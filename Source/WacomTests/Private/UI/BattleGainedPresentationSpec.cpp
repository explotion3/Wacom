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

namespace WacomBattleGainedPresentationSpec
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
			{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
		return Fixture.CreateSession(Character, Fixture.MakeSinglePartEnemy(20, 50), 1);
	}

	FBattleSnapshot MakeSnapshot(const TArray<FGuid>& HandIds, int32 DiscardCount = 0)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.PileCounts.DiscardCount = DiscardCount;
		for (const FGuid& CardId : HandIds)
		{
			FHandCardSnapshot& Card = Snapshot.Hand.Cards.AddDefaulted_GetRef();
			Card.InstanceId = CardId;
			Card.Zone = EHandZone::Both;
			Card.bIsPlayable = true;
		}
		Snapshot.Hand.NormalCardCount = Snapshot.Hand.Cards.Num();
		return Snapshot;
	}

	TUniquePtr<FWacomBattleHUDTestHarness> MakeHarness(
		FAutomationTestBase& Test,
		UWorld& World,
		UBattleSession& Session)
	{
		TUniquePtr<FWacomBattleHUDTestHarness> Harness =
			FWacomBattleHUDTestHarness::CreateHUDWithPlayer(&World);
		if (!Test.TestNotNull(TEXT("HUD harness"), Harness.Get()))
		{
			return nullptr;
		}
		Harness->SetSession(&Session);
		return Harness;
	}

	void DrainPlan(UWacomBattleHUDDetailTest& HUD)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < 24; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleGainedPresentationSinglePhaseSpec,
	"Wacom.UI.Battle.PresentationPlan.GainedCardPlaysOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleGainedPresentationSinglePhaseSpec::RunTest(const FString&)
{
	using namespace WacomBattleGainedPresentationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreatePlayerActionSession(Fixture);
	if (!TestNotNull(TEXT("Battle session"), Session))
	{
		return false;
	}
	TUniquePtr<FWacomBattleHUDTestHarness> Harness = MakeHarness(*this, *World, *Session);
	if (!Harness)
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FGuid ExistingId = FGuid::NewGuid();
	const FGuid GainedId = FGuid::NewGuid();
	const FBattleSnapshot PreSnapshot = MakeSnapshot({ ExistingId });
	const FBattleSnapshot GainedSnapshot = MakeSnapshot({ ExistingId, GainedId });
	FBattleEvent GainedEvent;
	GainedEvent.Type = EBattleEventType::CardGained;
	GainedEvent.Sequence = 40;
	GainedEvent.CardInstanceId = GainedId;
	FBattlePresentationJournal Journal;
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::CardGainedResolved,
		GainedSnapshot,
		{ GainedId },
		40,
		40);

	TestTrue(TEXT("Gained checkpoint enqueues the resolved command plan"),
		HUD->EnqueueCommandPresentationPlanForTest(
			Journal,
			{ GainedEvent },
			PreSnapshot,
			GainedSnapshot));
	DrainPlan(*HUD);
	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	int32 GainedPhaseCount = 0;
	for (const FName PhaseName : StartedPhases)
	{
		GainedPhaseCount += PhaseName == FName(TEXT("CommandCardGained")) ? 1 : 0;
	}
	TestEqual(TEXT("The explicit Gained phase plays once"), GainedPhaseCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleGainedThenDiscardPresentationSpec,
	"Wacom.UI.Battle.PresentationPlan.GainedPrecedesImmediateLimitDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleGainedThenDiscardPresentationSpec::RunTest(const FString&)
{
	using namespace WacomBattleGainedPresentationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreatePlayerActionSession(Fixture);
	if (!TestNotNull(TEXT("Battle session"), Session))
	{
		return false;
	}
	TUniquePtr<FWacomBattleHUDTestHarness> Harness = MakeHarness(*this, *World, *Session);
	if (!Harness)
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FGuid ExistingId = FGuid::NewGuid();
	const FGuid GainedId = FGuid::NewGuid();
	const FBattleSnapshot PreSnapshot = MakeSnapshot({ ExistingId });
	const FBattleSnapshot GainedSnapshot = MakeSnapshot({ ExistingId, GainedId });
	const FBattleSnapshot PostSnapshot = MakeSnapshot({ ExistingId }, 1);
	FBattleEvent GainedEvent;
	GainedEvent.Type = EBattleEventType::CardGained;
	GainedEvent.Sequence = 50;
	GainedEvent.CardInstanceId = GainedId;
	FBattleEvent DiscardEvent;
	DiscardEvent.Type = EBattleEventType::CardDiscarded;
	DiscardEvent.Sequence = 51;
	DiscardEvent.CardInstanceId = GainedId;
	DiscardEvent.CardInstanceIds = { GainedId };
	DiscardEvent.HandCardZoneMoveBatchSequence = 51;
	DiscardEvent.DiscardPileCountAfter = 1;
	FBattleEvent HandLimitEvent;
	HandLimitEvent.Type = EBattleEventType::HandLimitDiscarded;
	HandLimitEvent.Sequence = 52;
	HandLimitEvent.CardInstanceId = GainedId;
	HandLimitEvent.CardInstanceIds = { GainedId };
	FBattlePresentationJournal Journal;
	Journal.AddCheckpoint(
		EBattlePresentationCheckpointType::CardGainedResolved,
		GainedSnapshot,
		{ GainedId },
		50,
		50);

	TestTrue(TEXT("Immediate limit discard enqueues the resolved command plan"),
		HUD->EnqueueCommandPresentationPlanForTest(
			Journal,
			{ GainedEvent, DiscardEvent, HandLimitEvent },
			PreSnapshot,
			PostSnapshot));
	DrainPlan(*HUD);
	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const int32 GainedIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandCardGained")));
	const int32 DiscardIndex = StartedPhases.IndexOfByKey(FName(TEXT("HandDiscardGlyphTransfer")));
	TestTrue(TEXT("Gained phase exists"), GainedIndex != INDEX_NONE);
	TestTrue(TEXT("Discard glyph phase exists"), DiscardIndex != INDEX_NONE);
	TestTrue(TEXT("Gain reveal completes before immediate discard"), GainedIndex < DiscardIndex);
	return true;
}

#endif
