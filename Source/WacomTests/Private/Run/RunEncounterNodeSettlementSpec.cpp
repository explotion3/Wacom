// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "Session/BattleResultPacket.h"

namespace
{
	FWacomInitializedRunExplorationSession MakeEncounterSession(
		FWacomRunExplorationFixture& Fixture)
	{
		UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Encounter.Floor"), 1);
		Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
		return Fixture.CreateInitializedSession(nullptr, Fixture.MakeJourney({ Floor }));
	}

	int32 CountOwnedCard(const FRunState& State, const UCardDefinition* Definition)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEncounterVictoryAtomicSettlementTest,
	"Wacom.Run.NodeActivity.Encounter.VictoryCommitsReservationRewardsAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEncounterVictoryAtomicSettlementTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = MakeEncounterSession(Fixture);
	URunSession* Run = Initialized.Session;
	const FRunExplorationResolution Begin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!TestTrue(TEXT("Encounter begin succeeds"), Begin.IsOk())
		|| !TestTrue(TEXT("Encounter ticket is returned"), Begin.NodeActivityTicket.IsSet()))
	{
		return false;
	}

	FWacomBattleFixture BattleFixture;
	UCardDefinition* Reward = BattleFixture.MakeNoopCard(0);
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bCrossedHighHpThreshold = true;
	FKnockdownExpGain& Gain = Packet.KnockdownExpGains.AddDefaulted_GetRef();
	Gain.PartId = TEXT("Part.Reward");
	Gain.ExpAmount = 3;
	FBattleGainedCard& GainedCard = Packet.GainedCards.AddDefaulted_GetRef();
	GainedCard.Definition = Reward;

	const FRunExplorationResolution Settlement = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(),
		Packet);
	TestTrue(TEXT("Victory settlement succeeds"), Settlement.IsOk());
	TestEqual(TEXT("Last Morning AP advances to Day"),
		Settlement.PostSnapshot.Time.CurrentTimePhase, ETimePhase::Day);
	TestEqual(TEXT("Encounter is resolved"),
		Settlement.PostSnapshot.Nodes[0].Lifecycle, ERunMapNodeLifecycle::Resolved);
	TestEqual(TEXT("Fatigue is settled"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 1);
	TestEqual(TEXT("Wound threshold is settled"),
		Run->GetPressureValue(EWacomPressureType::Wound), 1);
	TestEqual(TEXT("Experience is settled"), Run->GetExperienceCurrent(), 3);
	TestEqual(TEXT("Reward card is settled"), CountOwnedCard(Run->GetRunState(), Reward), 1);

	const FRunExplorationSnapshot BeforeDuplicate = Run->BuildExplorationSnapshot();
	const FRunExplorationResolution Duplicate = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(),
		Packet);
	TestFalse(TEXT("Duplicate result is rejected"), Duplicate.IsOk());
	TestEqual(TEXT("Duplicate preserves version"),
		Run->BuildExplorationSnapshot().StateVersion, BeforeDuplicate.StateVersion);
	TestEqual(TEXT("Duplicate does not grant another card"),
		CountOwnedCard(Run->GetRunState(), Reward), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEncounterWithdrawSettlementTest,
	"Wacom.Run.NodeActivity.Encounter.WithdrawReleasesReservationAndKeepsProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEncounterWithdrawSettlementTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = MakeEncounterSession(Fixture);
	URunSession* Run = Initialized.Session;
	const int32 ActionPointsBefore = Run->BuildExplorationSnapshot().Time.RemainingActionPoints;
	const FRunExplorationResolution Begin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);

	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Victory;
	Packet.bWithdrawn = true;
	Packet.bCrossedLowHpThreshold = true;
	Packet.DestroyedPartKeys.Add(
		FBattleEnemyPartKey::Make(TEXT("Encounter"), TEXT("Enemy"), TEXT("Part.Broken")));
	FKnockdownExpGain& Gain = Packet.KnockdownExpGains.AddDefaulted_GetRef();
	Gain.ExpAmount = 2;
	const FRunExplorationResolution Settlement = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(),
		Packet);
	TestTrue(TEXT("Withdraw settlement succeeds"), Settlement.IsOk());
	TestEqual(TEXT("Withdraw consumes no AP"),
		Settlement.PostSnapshot.Time.RemainingActionPoints, ActionPointsBefore);
	TestEqual(TEXT("Encounter remains visited"),
		Settlement.PostSnapshot.Nodes[0].Lifecycle, ERunMapNodeLifecycle::Visited);
	TestTrue(TEXT("Destroyed-part progress is retained"),
		Run->GetRunState().BattleProgress.Contains(
			FWacomMapNodeHandle{ TEXT("Encounter.Floor"), TEXT("Node.01") }));
	TestEqual(TEXT("Withdraw still settles fatigue"),
		Run->GetPressureValue(EWacomPressureType::Fatigue), 1);
	TestEqual(TEXT("Withdraw still settles wound threshold"),
		Run->GetPressureValue(EWacomPressureType::Wound), 5);
	TestEqual(TEXT("Withdraw retains earned experience"), Run->GetExperienceCurrent(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEncounterDefeatSettlementTest,
	"Wacom.Run.NodeActivity.Encounter.DefeatEndsRunWithoutAdvancingTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEncounterDefeatSettlementTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = MakeEncounterSession(Fixture);
	URunSession* Run = Initialized.Session;
	const int32 ActionPointsBefore = Run->BuildExplorationSnapshot().Time.RemainingActionPoints;
	const FRunExplorationResolution Begin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	FBattleResultPacket Packet;
	Packet.Outcome = EBattleOutcome::Defeat;
	const FRunExplorationResolution Settlement = Run->SettleEncounterNodeActivity(
		Begin.NodeActivityTicket.GetValue(),
		Packet);
	TestTrue(TEXT("Defeat settlement succeeds"), Settlement.IsOk());
	TestFalse(TEXT("Defeat ends the Run"), Run->IsRunActive());
	TestEqual(TEXT("Defeat does not spend reserved AP"),
		Settlement.PostSnapshot.Time.RemainingActionPoints, ActionPointsBefore);
	TestEqual(TEXT("Defeat does not resolve Encounter"),
		Settlement.PostSnapshot.Nodes[0].Lifecycle, ERunMapNodeLifecycle::Visited);
	return true;
}

#endif
