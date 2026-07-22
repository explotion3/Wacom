// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattlePresentationPlanSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	UBattleSession* CreatePlayerActionSession(FWacomBattleFixture& Fixture)
	{
		UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
		UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			LeftHand,
			RightHand,
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50);
		return Fixture.CreateSession(Character, Enemy, 1);
	}

	FHandCardSnapshot MakeHandCard(const FGuid& CardInstanceId, bool bIsHandAnchor = false)
	{
		FHandCardSnapshot Card;
		Card.InstanceId = CardInstanceId;
		Card.RuntimeCost = 1;
		Card.Zone = EHandZone::Both;
		Card.bIsPlayable = true;
		Card.bIsHandAnchor = bIsHandAnchor;
		return Card;
	}

	FBattleSnapshot MakeSnapshotWithHand(const TArray<FGuid>& CardInstanceIds)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.TurnNumber = 2;
		for (const FGuid& CardInstanceId : CardInstanceIds)
		{
			Snapshot.Hand.Cards.Add(MakeHandCard(CardInstanceId));
		}
		Snapshot.Hand.NormalCardCount = Snapshot.Hand.Cards.Num();
		return Snapshot;
	}

	FBattleSnapshot MakeSnapshotWithHandCards(const TArray<FHandCardSnapshot>& Cards)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.TurnNumber = 2;
		Snapshot.Hand.Cards = Cards;
		Snapshot.Hand.NormalCardCount = 0;
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (!Card.bIsHandAnchor)
			{
				++Snapshot.Hand.NormalCardCount;
			}
		}
		return Snapshot;
	}

	FBattleEvent MakeCardEvent(
		EBattleEventType EventType,
		int32 Sequence,
		const FGuid& CardInstanceId)
	{
		FBattleEvent Event;
		Event.Type = EventType;
		Event.Sequence = Sequence;
		Event.CardInstanceId = CardInstanceId;
		Event.CardInstanceIds = { CardInstanceId };
		Event.Count = 1;
		return Event;
	}

	FBattleEvent MakeEnemyDamageEvent(int32 Sequence)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.Sequence = Sequence;
		Event.ActorEnemyPartKey = FBattleEnemyPartKey::Make(
			TEXT("Encounter"),
			TEXT("Enemy"),
			TEXT("Head"));
		Event.Amount = 3;
		return Event;
	}

	UCardDefinition* MakePoisonCardOnPart(
		FWacomBattleFixture& Fixture,
		int32 Cost,
		int32 Stacks)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(Cost);
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;

		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Effect.Magnitude = Stacks;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		Card->Effects.Add(Effect);
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEndTurnPresentationPlanSequencesPhasesTest,
	"Wacom.UI.Battle.PresentationPlan.EndTurnJournalSequencesDiscardRetainEnemyDraw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEndTurnPresentationPlanSequencesPhasesTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePresentationPlanSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	TestTrue(TEXT("HUD can submit commands before plan starts"), HUD->CanSubmitPlayerActionCommand());
	TestTrue(
		TEXT("First-person hand interaction is enabled before the plan starts"),
		HUD->ShouldEnableFirstPersonBattleHandInteractionForTest());

	const FGuid DiscardedId = FGuid::NewGuid();
	const FGuid RetainedId = FGuid::NewGuid();
	const FGuid DrawnId = FGuid::NewGuid();
	const FBattleSnapshot DiscardSnapshot = MakeSnapshotWithHand({ RetainedId });
	const FBattleSnapshot RetainSnapshot = MakeSnapshotWithHand({ RetainedId });
	const FBattleSnapshot DrawSnapshot = MakeSnapshotWithHand({ RetainedId, DrawnId });

	FBattlePresentationJournal Journal;
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnEndDiscardResolved,
		DiscardSnapshot,
		{ DiscardedId },
		10,
		10);
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnEndRetainResolved,
		RetainSnapshot,
		{ RetainedId },
		12,
		12);
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnStartDrawResolved,
		DrawSnapshot,
		{ DrawnId },
		30,
		30);

	const TArray<FBattleEvent> Events = {
		MakeCardEvent(EBattleEventType::CardDiscarded, 10, DiscardedId),
		MakeCardEvent(EBattleEventType::CardsRetained, 12, RetainedId),
		MakeEnemyDamageEvent(20),
		MakeCardEvent(EBattleEventType::CardsDrawn, 30, DrawnId)
	};

	TestTrue(
		TEXT("EndTurn journal enqueues a presentation plan"),
		HUD->EnqueueEndTurnPresentationPlanForTest(Journal, Events, DrawSnapshot));
	TestTrue(TEXT("Presentation plan is active"), HUD->IsPresentationPlanActiveForTest());
	TestEqual(
		TEXT("Hand phases skip without card layer and enemy phase becomes active"),
		HUD->GetActivePresentationPlanPhaseNameForTest(),
		FName(TEXT("EnemyAction")));
	TestEqual(TEXT("Draw and retain release remain pending after enemy phase starts"), HUD->GetPresentationPlanPendingPhaseCountForTest(), 2);
	TestFalse(TEXT("Presentation plan blocks player commands"), HUD->CanSubmitPlayerActionCommand());
	TestFalse(
		TEXT("Presentation plan locks first-person hand interaction"),
		HUD->ShouldEnableFirstPersonBattleHandInteractionForTest());
	TestTrue(TEXT("Enemy phase reuses event queue busy state"), HUD->IsBattlePresentationBusy());

	for (int32 Iteration = 0; HUD->IsBattlePresentationBusy() && Iteration < 16; ++Iteration)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	TestFalse(TEXT("Presentation plan finishes"), HUD->IsPresentationPlanActiveForTest());
	TestFalse(TEXT("Presentation busy clears after plan"), HUD->IsBattlePresentationBusy());
	TestTrue(
		TEXT("First-person hand interaction unlocks after the plan finishes"),
		HUD->ShouldEnableFirstPersonBattleHandInteractionForTest());

	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const TArray<FName> ExpectedPhases = {
		FName(TEXT("TurnEndDiscard")),
		FName(TEXT("TurnEndRetain")),
		FName(TEXT("EnemyAction")),
		FName(TEXT("TurnStartDraw")),
		FName(TEXT("TurnStartRetainRelease"))
	};
	TestEqual(TEXT("Plan starts expected phase count"), StartedPhases.Num(), ExpectedPhases.Num());
	for (int32 Index = 0; Index < FMath::Min(StartedPhases.Num(), ExpectedPhases.Num()); ++Index)
	{
		TestEqual(
			FString::Printf(TEXT("Plan phase %d starts in order"), Index),
			StartedPhases[Index],
			ExpectedPhases[Index]);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEndTurnPresentationPlanAddsHandAnchorEnterAfterDrawTest,
	"Wacom.UI.Battle.PresentationPlan.EndTurnJournalAddsHandAnchorEnterAfterDraw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEndTurnPresentationPlanAddsHandAnchorEnterAfterDrawTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePresentationPlanSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FGuid RetainedId = FGuid::NewGuid();
	const FGuid DrawnId = FGuid::NewGuid();
	const FGuid HandAnchorId = FGuid::NewGuid();
	const FHandCardSnapshot Retained = MakeHandCard(RetainedId);
	const FHandCardSnapshot Drawn = MakeHandCard(DrawnId);
	const FHandCardSnapshot HandAnchor = MakeHandCard(HandAnchorId, true);
	const FBattleSnapshot RetainSnapshot = MakeSnapshotWithHandCards({ Retained });
	const FBattleSnapshot DrawSnapshot = MakeSnapshotWithHandCards({ Retained, Drawn, HandAnchor });

	FBattlePresentationJournal Journal;
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnEndRetainResolved,
		RetainSnapshot,
		{ RetainedId },
		12,
		12);
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnStartDrawResolved,
		DrawSnapshot,
		{ DrawnId },
		30,
		30);

	const TArray<FBattleEvent> Events = {
		MakeCardEvent(EBattleEventType::CardsRetained, 12, RetainedId),
		MakeCardEvent(EBattleEventType::CardsDrawn, 30, DrawnId)
	};

	TestTrue(
		TEXT("EndTurn journal enqueues a presentation plan"),
		HUD->EnqueueEndTurnPresentationPlanForTest(Journal, Events, DrawSnapshot));
	TestFalse(TEXT("Presentation plan finishes without a card layer"), HUD->IsPresentationPlanActiveForTest());

	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const TArray<FName> ExpectedPhases = {
		FName(TEXT("TurnEndRetain")),
		FName(TEXT("TurnStartDraw")),
		FName(TEXT("TurnStartHandAnchorEnter")),
		FName(TEXT("TurnStartRetainRelease"))
	};
	TestEqual(TEXT("Plan starts retained, draw, hand-anchor enter, then retained release"), StartedPhases.Num(), ExpectedPhases.Num());
	for (int32 Index = 0; Index < FMath::Min(StartedPhases.Num(), ExpectedPhases.Num()); ++Index)
	{
		TestEqual(
			FString::Printf(TEXT("Plan phase %d starts in order"), Index),
			StartedPhases[Index],
			ExpectedPhases[Index]);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEndTurnPresentationPlanSkipsExistingHandAnchorEnterTest,
	"Wacom.UI.Battle.PresentationPlan.EndTurnJournalSkipsExistingHandAnchorEnter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEndTurnPresentationPlanSkipsExistingHandAnchorEnterTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattlePresentationPlanSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreatePlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FGuid RetainedId = FGuid::NewGuid();
	const FGuid DrawnId = FGuid::NewGuid();
	const FGuid HandAnchorId = FGuid::NewGuid();
	const FHandCardSnapshot Retained = MakeHandCard(RetainedId);
	const FHandCardSnapshot Drawn = MakeHandCard(DrawnId);
	const FHandCardSnapshot HandAnchor = MakeHandCard(HandAnchorId, true);
	const FBattleSnapshot RetainSnapshot = MakeSnapshotWithHandCards({ Retained, HandAnchor });
	const FBattleSnapshot DrawSnapshot = MakeSnapshotWithHandCards({ Retained, Drawn, HandAnchor });

	FBattlePresentationJournal Journal;
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnEndRetainResolved,
		RetainSnapshot,
		{ RetainedId },
		12,
		12);
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::TurnStartDrawResolved,
		DrawSnapshot,
		{ DrawnId },
		30,
		30);

	const TArray<FBattleEvent> Events = {
		MakeCardEvent(EBattleEventType::CardsRetained, 12, RetainedId),
		MakeCardEvent(EBattleEventType::CardsDrawn, 30, DrawnId)
	};

	TestTrue(
		TEXT("EndTurn journal enqueues a presentation plan"),
		HUD->EnqueueEndTurnPresentationPlanForTest(Journal, Events, DrawSnapshot));

	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	TestFalse(
		TEXT("Existing hand anchor does not get a generated enter phase"),
		StartedPhases.Contains(FName(TEXT("TurnStartHandAnchorEnter"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEndTurnPresentationPlanKeepsPostDrawKnockdownDialogTest,
	"Wacom.UI.Battle.PresentationPlan.EndTurnKeepsPostDrawKnockdownDialog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEndTurnPresentationPlanKeepsPostDrawKnockdownDialogTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattlePresentationPlanSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* PoisonCard = MakePoisonCardOnPart(Fixture, /*Cost*/1, /*Stacks*/1);
	TArray<UCardDefinition*> Deck = { PoisonCard };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(0));
	}
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(2),
		Fixture.MakeNoopCard(2),
		Deck);
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(/*Hp*/16, /*Initiative*/20);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, /*Seed*/41);
	if (!TestNotNull(TEXT("Battle session"), Session))
	{
		return false;
	}

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid PartInstanceId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	const FGuid PoisonCardInstanceId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonCard->CardId);
	TestTrue(TEXT("Poison card is in hand"), PoisonCardInstanceId.IsValid());
	TestTrue(
		TEXT("Poison setup command resolves"),
		Session->ResolveCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(
			Snapshot,
			PoisonCardInstanceId,
			PartInstanceId)).IsOk());

	const FBattleResolution EndTurnResolution =
		Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("Lethal poison EndTurn resolves"), EndTurnResolution.IsOk());
	TestEqual(
		TEXT("Lethal poison leaves the session waiting for knockdown choice"),
		EndTurnResolution.PostSnapshot.Phase,
		EBattlePhase::PendingKnockdownChoice);
	const FBattlePresentationCheckpoint* DrawCheckpoint =
		EndTurnResolution.PresentationJournal.Checkpoints.FindByPredicate(
			[](const FBattlePresentationCheckpoint& Checkpoint)
			{
				return Checkpoint.Type ==
					EBattlePresentationCheckpointType::TurnStartDrawResolved;
			});
	const FBattleEvent* KnockdownRequest = EndTurnResolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::KnockdownChoiceRequested;
		});
	TestNotNull(TEXT("EndTurn records its draw checkpoint"), DrawCheckpoint);
	TestNotNull(TEXT("EndTurn emits a knockdown request"), KnockdownRequest);
	TestTrue(
		TEXT("Command pipeline appends the knockdown request after the draw checkpoint"),
		DrawCheckpoint
			&& KnockdownRequest
			&& KnockdownRequest->Sequence > DrawCheckpoint->LastEventSequence);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	TestTrue(
		TEXT("EndTurn journal enqueues a presentation plan"),
		HUD->EnqueueEndTurnPresentationPlanForTest(
			EndTurnResolution.PresentationJournal,
			EndTurnResolution.Events,
			EndTurnResolution.PostSnapshot));
	for (int32 Iteration = 0; HUD->IsBattlePresentationBusy() && Iteration < 128; ++Iteration)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const int32 EnemyActionIndex = StartedPhases.IndexOfByKey(FName(TEXT("EnemyAction")));
	const int32 DrawIndex = StartedPhases.IndexOfByKey(FName(TEXT("TurnStartDraw")));
	const int32 DialogIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandBlockingDialog")));
	TestTrue(TEXT("Enemy action phase is presented"), EnemyActionIndex != INDEX_NONE);
	TestTrue(TEXT("Turn-start draw phase is presented"), DrawIndex != INDEX_NONE);
	TestTrue(TEXT("Post-draw knockdown request becomes a blocking dialog phase"),
		DialogIndex != INDEX_NONE);
	TestTrue(TEXT("Knockdown dialog follows enemy action and draw presentation"),
		EnemyActionIndex < DrawIndex && DrawIndex < DialogIndex);
	TestTrue(TEXT("Pending choice remains available for the dialog"),
		Session->BuildPendingKnockdownChoiceView().bHasPendingChoice);

	return true;
}

#endif
