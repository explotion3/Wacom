// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleSession.h"
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
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 50, 0);
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
	TestEqual(TEXT("Draw phase remains pending after enemy phase starts"), HUD->GetPresentationPlanPendingPhaseCountForTest(), 1);
	TestFalse(TEXT("Presentation plan blocks player commands"), HUD->CanSubmitPlayerActionCommand());
	TestTrue(TEXT("Enemy phase reuses event queue busy state"), HUD->IsBattlePresentationBusy());

	for (int32 Iteration = 0; HUD->IsBattlePresentationBusy() && Iteration < 16; ++Iteration)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	TestFalse(TEXT("Presentation plan finishes"), HUD->IsPresentationPlanActiveForTest());
	TestFalse(TEXT("Presentation busy clears after plan"), HUD->IsBattlePresentationBusy());

	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const TArray<FName> ExpectedPhases = {
		FName(TEXT("TurnEndDiscard")),
		FName(TEXT("TurnEndRetain")),
		FName(TEXT("EnemyAction")),
		FName(TEXT("TurnStartDraw"))
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
		FName(TEXT("TurnStartHandAnchorEnter"))
	};
	TestEqual(TEXT("Plan starts retained, draw, then hand-anchor enter"), StartedPhases.Num(), ExpectedPhases.Num());
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

#endif
