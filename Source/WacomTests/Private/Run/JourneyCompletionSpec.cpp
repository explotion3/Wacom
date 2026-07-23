// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "Session/BattleResultPacket.h"
#include "WacomSaveGame.h"

namespace WacomJourneyCompletionSpec
{
	struct FJourneyCompletionFixture
	{
		FWacomRunExplorationFixture Fixture;
		UWacomFloorMapDefinition* Floor = nullptr;
		UWacomJourneyDefinition* Journey = nullptr;
		FWacomInitializedRunExplorationSession Initialized;

		explicit FJourneyCompletionFixture(const int32 NodeCount = 1)
		{
			Floor = Fixture.MakeLinearFloor(TEXT("Floor.Completion"), NodeCount);
			for (FWacomMapNodeDefinition& Node : Floor->Nodes)
			{
				Node.NodeType = EWacomMapNodeType::Encounter;
				Node.Content.Encounter.EncounterDefinition =
					NewObject<UEncounterDefinition>(Floor);
				Node.Content.Encounter.bBoss = true;
			}
			Journey = Fixture.MakeJourney({ Floor }, TEXT("Journey.Completion"));
			Journey->DisplayName = FText::FromString(TEXT("Completion Journey"));
			Journey->SuccessTerminalNode = {
				Floor->FloorId,
				Floor->Nodes.Last().NodeId };
			Initialized = Fixture.CreateInitializedSession(nullptr, Journey);
		}
	};

	int32 CountJourneyCompletionOwnedCard(
		const FRunState& State,
		const UCardDefinition* Definition)
	{
		int32 Count = 0;
		auto CountPile = [&Count, Definition](const TArray<FCardInstance>& Pile)
		{
			for (const FCardInstance& Instance : Pile)
			{
				Count += Instance.Definition == Definition ? 1 : 0;
			}
		};
		CountPile(State.Backpack);
		CountPile(State.BattleDeck);
		CountPile(State.BurdenZone);
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			CountPile(Zone.Cards);
		}
		return Count;
	}

	FBattleResultPacket MakeJourneyCompletionVictory()
	{
		FBattleResultPacket Packet;
		Packet.Outcome = EBattleOutcome::Victory;
		return Packet;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomJourneyCompletionAtomicSuccessSpec,
	"Wacom.Run.JourneyCompletion.AtomicSuccessAndTerminalGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomJourneyCompletionAtomicSuccessSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneyCompletionSpec;

	FJourneyCompletionFixture Completion;
	URunSession* Run = Completion.Initialized.Session;
	if (!TestTrue(TEXT("Run initializes"), Completion.Initialized.Initialization.IsOk()))
	{
		return false;
	}

	Run->SetPressure(EWacomPressureType::Wound, 99);
	const FRunExplorationResolution Begin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Terminal Encounter begins"), Begin.IsOk())
		|| !TestTrue(TEXT("Terminal ticket exists"), Begin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	int32 NotificationCount = 0;
	const FDelegateHandle NotificationHandle = Run->OnRunStateChangedNative.AddLambda(
		[&NotificationCount]()
		{
			++NotificationCount;
		});

	FWacomBattleFixture BattleFixture;
	UCardDefinition* Reward = BattleFixture.MakeNoopCard(0);
	FBattleResultPacket Packet = MakeJourneyCompletionVictory();
	Packet.bMutualDestruction = true;
	Packet.bCrossedHighHpThreshold = true;
	Packet.KnockdownExpGains.AddDefaulted_GetRef().ExpAmount = 3;
	Packet.GainedCards.AddDefaulted_GetRef().Definition = Reward;

	const FRunExplorationResolution Settlement = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(),
		Packet);
	TestTrue(TEXT("Terminal Victory settles"), Settlement.IsOk());
	TestEqual(TEXT("Settlement increments exploration version once"),
		Settlement.VersionAfter,
		Settlement.VersionBefore + 1);
	TestEqual(TEXT("Settlement broadcasts once"), NotificationCount, 1);
	TestTrue(TEXT("Success emits at least one event"), !Settlement.Events.IsEmpty());
	if (!Settlement.Events.IsEmpty())
	{
		const FRunExplorationEvent& LastEvent = Settlement.Events.Last();
		TestEqual(TEXT("JourneySucceeded is the final event"),
			LastEvent.Type,
			ERunExplorationEventType::JourneySucceeded);
		TestTrue(TEXT("Success event uses terminal handle"),
			LastEvent.Node == Completion.Journey->SuccessTerminalNode);
		TestEqual(TEXT("Success event detail is JourneyId"),
			LastEvent.Detail,
			Completion.Journey->JourneyId);
	}
	int32 SuccessEventCount = 0;
	for (const FRunExplorationEvent& Event : Settlement.Events)
	{
		SuccessEventCount += Event.Type == ERunExplorationEventType::JourneySucceeded ? 1 : 0;
	}
	TestEqual(TEXT("Exactly one success event is emitted"), SuccessEventCount, 1);

	const FRunExplorationSnapshot Snapshot = Run->BuildExplorationSnapshot();
	TestEqual(TEXT("Outcome is Succeeded"), Snapshot.Outcome, ERunOutcome::Succeeded);
	TestTrue(TEXT("Completion summary is present"), Snapshot.bHasCompletionSummary);
	TestTrue(TEXT("Completion summary is valid"), Snapshot.CompletionSummary.IsValid());
	TestEqual(TEXT("Summary JourneyId"),
		Snapshot.CompletionSummary.JourneyId,
		Completion.Journey->JourneyId);
	TestTrue(TEXT("Summary terminal"),
		Snapshot.CompletionSummary.TerminalNode
			== Completion.Journey->SuccessTerminalNode);
	TestEqual(TEXT("Summary entered floors"), Snapshot.CompletionSummary.EnteredFloorCount, 1);
	TestEqual(TEXT("Summary total floors"), Snapshot.CompletionSummary.TotalFloorCount, 1);
	TestEqual(TEXT("Summary resolved nodes"), Snapshot.CompletionSummary.ResolvedNodeCount, 1);
	TestEqual(TEXT("Summary total nodes"), Snapshot.CompletionSummary.TotalNodeCount, 1);
	TestEqual(TEXT("Summary keeps final pressure"),
		Snapshot.CompletionSummary.FinalPressure,
		Run->GetTotalPressure());
	TestTrue(TEXT("Terminal Victory wins over pressure failure"),
		Snapshot.CompletionSummary.FinalPressure >= 100);
	TestFalse(TEXT("Succeeded Run is not active"), Run->IsRunActive());
	TestFalse(TEXT("Succeeded Run is not failed"), Run->IsRunFailed());
	TestEqual(TEXT("Battle experience is committed"), Run->GetExperienceCurrent(), 3);
	TestEqual(TEXT("Battle reward is committed"),
		CountJourneyCompletionOwnedCard(Run->GetRunState(), Reward),
		1);

	NotificationCount = 0;
	const int32 VersionAfterSuccess = Snapshot.StateVersion;
	const int32 GoldAfterSuccess = Run->GetGold();
	const int32 PressureAfterSuccess = Run->GetTotalPressure();
	const int32 FingerAfterSuccess = Run->GetFingerCount();
	const int32 RewardCountAfterSuccess =
		CountJourneyCompletionOwnedCard(Run->GetRunState(), Reward);

	const FRunExplorationResolution Duplicate = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(),
		Packet);
	TestFalse(TEXT("Duplicate result is rejected"), Duplicate.IsOk());
	TestEqual(TEXT("Duplicate reports terminal state"),
		Duplicate.Status.Detail,
		FName(TEXT("RunAlreadySucceeded")));
	Run->AddGold(5);
	Run->RemoveGold(1);
	Run->AddPressure(EWacomPressureType::Fatigue, 5);
	Run->RemoveFinger(1);
	Run->AcquireCardToRun(Reward);
	FBattleInitParams RejectedBattleParams;
	TestFalse(TEXT("Battle initialization is rejected after success"),
		Run->BuildInitParamsForBattle(
			Run->BuildExplorationSnapshot().CurrentNode,
			TEXT("Terminal"),
			RejectedBattleParams));
	const FRunTreasureSettlementResult Pickup =
		Run->CollectGoldPickup(TEXT("Pickup.AfterSuccess"), 2);
	TestFalse(TEXT("Pickup is rejected after success"), Pickup.bSucceeded);
	TestEqual(TEXT("Pickup reports terminal state"),
		Pickup.DisabledReason,
		FName(TEXT("RunAlreadySucceeded")));
	const FRunExplorationResolution Command = Run->ResolveExplorationCommand(
		FRunExplorationCommand::ChooseNightExploration(VersionAfterSuccess));
	TestFalse(TEXT("Exploration command is rejected after success"), Command.IsOk());
	TestFalse(TEXT("Shop is rejected after success"),
		Run->BeginShopVisit(TEXT("Shop.AfterSuccess"), TArray<FRunShopOfferInput>()));
	TestFalse(TEXT("RunEvent is rejected after success"),
		Run->BeginRunEvent(TEXT("Event.AfterSuccess"), nullptr));
	TestNotNull(TEXT("Saving a successful summary remains allowed"),
		Run->BuildSaveGameFromRunState());

	const FRunExplorationSnapshot AfterRejectedWrites = Run->BuildExplorationSnapshot();
	TestEqual(TEXT("Rejected writes preserve version"),
		AfterRejectedWrites.StateVersion,
		VersionAfterSuccess);
	TestEqual(TEXT("Rejected writes preserve gold"), Run->GetGold(), GoldAfterSuccess);
	TestEqual(TEXT("Rejected writes preserve pressure"),
		Run->GetTotalPressure(),
		PressureAfterSuccess);
	TestEqual(TEXT("Rejected writes preserve fingers"),
		Run->GetFingerCount(),
		FingerAfterSuccess);
	TestEqual(TEXT("Rejected writes preserve cards"),
		CountJourneyCompletionOwnedCard(Run->GetRunState(), Reward),
		RewardCountAfterSuccess);
	TestEqual(TEXT("Rejected writes do not broadcast"), NotificationCount, 0);

	Run->OnRunStateChangedNative.Remove(NotificationHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomJourneyCompletionExclusionSpec,
	"Wacom.Run.JourneyCompletion.ExclusionsAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomJourneyCompletionExclusionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneyCompletionSpec;

	FJourneyCompletionFixture NonTerminal(2);
	URunSession* NonTerminalRun = NonTerminal.Initialized.Session;
	const FRunExplorationResolution NonTerminalBegin =
		NonTerminalRun->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	const FRunExplorationResolution NonTerminalSettlement =
		NonTerminalRun->SettleEncounterNodeActivity(
			NonTerminalBegin.NodeActivityTicket.GetValue(),
			MakeJourneyCompletionVictory());
	TestTrue(TEXT("Ordinary Boss victory settles"), NonTerminalSettlement.IsOk());
	TestEqual(TEXT("Ordinary Boss does not complete Journey"),
		NonTerminalRun->GetRunState().Outcome,
		ERunOutcome::InProgress);

	FJourneyCompletionFixture Withdraw;
	URunSession* WithdrawRun = Withdraw.Initialized.Session;
	const FRunExplorationResolution WithdrawBegin =
		WithdrawRun->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	FBattleResultPacket WithdrawPacket = MakeJourneyCompletionVictory();
	WithdrawPacket.bWithdrawn = true;
	const FRunExplorationResolution WithdrawSettlement =
		WithdrawRun->SettleEncounterNodeActivity(
			WithdrawBegin.NodeActivityTicket.GetValue(),
			WithdrawPacket);
	TestTrue(TEXT("Withdraw settles existing battle semantics"), WithdrawSettlement.IsOk());
	TestEqual(TEXT("Withdraw does not complete Journey"),
		WithdrawRun->GetRunState().Outcome,
		ERunOutcome::InProgress);

	FJourneyCompletionFixture Defeat;
	URunSession* DefeatRun = Defeat.Initialized.Session;
	const FRunExplorationResolution DefeatBegin =
		DefeatRun->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	FBattleResultPacket DefeatPacket;
	DefeatPacket.Outcome = EBattleOutcome::Defeat;
	const FRunExplorationResolution DefeatSettlement = DefeatRun->SettleEncounterNodeActivity(
		DefeatBegin.NodeActivityTicket.GetValue(),
		DefeatPacket);
	TestTrue(TEXT("Defeat settles"), DefeatSettlement.IsOk());
	TestEqual(TEXT("Defeat produces Failed"),
		DefeatRun->GetRunState().Outcome,
		ERunOutcome::Failed);
	TestFalse(TEXT("Defeat emits no success event"),
		DefeatSettlement.Events.ContainsByPredicate(
			[](const FRunExplorationEvent& Event)
			{
				return Event.Type == ERunExplorationEventType::JourneySucceeded;
			}));

	FJourneyCompletionFixture Undetermined;
	URunSession* UndeterminedRun = Undetermined.Initialized.Session;
	const FRunExplorationResolution UndeterminedBegin =
		UndeterminedRun->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	FBattleResultPacket UndeterminedPacket;
	UndeterminedPacket.Outcome = EBattleOutcome::Undetermined;
	const FRunExplorationResolution UndeterminedSettlement =
		UndeterminedRun->SettleEncounterNodeActivity(
			UndeterminedBegin.NodeActivityTicket.GetValue(),
			UndeterminedPacket);
	TestFalse(TEXT("Undetermined is rejected"), UndeterminedSettlement.IsOk());
	TestEqual(TEXT("Undetermined preserves InProgress"),
		UndeterminedRun->GetRunState().Outcome,
		ERunOutcome::InProgress);

	FJourneyCompletionFixture StaleA;
	FJourneyCompletionFixture StaleB;
	const FRunExplorationResolution StaleBeginA =
		StaleA.Initialized.Session->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	const FRunExplorationResolution StaleBeginB =
		StaleB.Initialized.Session->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	const FRunExplorationResolution StaleSettlement =
		StaleA.Initialized.Session->SettleEncounterNodeActivity(
			StaleBeginB.NodeActivityTicket.GetValue(),
			MakeJourneyCompletionVictory());
	TestFalse(TEXT("Foreign/stale ticket is rejected"), StaleSettlement.IsOk());
	TestEqual(TEXT("Stale ticket preserves version"),
		StaleA.Initialized.Session->BuildExplorationSnapshot().StateVersion,
		StaleBeginA.PostSnapshot.StateVersion);

	FJourneyCompletionFixture Rollback;
	URunSession* RollbackRun = Rollback.Initialized.Session;
	const FRunExplorationResolution RollbackBegin =
		RollbackRun->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	const int32 VersionBeforeRollback = RollbackBegin.PostSnapshot.StateVersion;
	Rollback.Journey->JourneyId = NAME_None;
	const FRunExplorationResolution RollbackSettlement =
		RollbackRun->SettleEncounterNodeActivity(
			RollbackBegin.NodeActivityTicket.GetValue(),
			MakeJourneyCompletionVictory());
	TestFalse(TEXT("Invalid completion summary rejects settlement"), RollbackSettlement.IsOk());
	TestEqual(TEXT("Invalid summary rolls back exploration version"),
		RollbackRun->BuildExplorationSnapshot().StateVersion,
		VersionBeforeRollback);
	TestEqual(TEXT("Invalid summary preserves InProgress"),
		RollbackRun->GetRunState().Outcome,
		ERunOutcome::InProgress);

	return true;
}

#endif
