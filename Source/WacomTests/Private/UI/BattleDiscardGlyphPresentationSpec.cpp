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
#include "UI/PileCountViewTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleDiscardGlyphPresentationSpec
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

	FBattleSnapshot MakeSnapshot(const TArray<FGuid>& HandIds, int32 DrawCount, int32 DiscardCount)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.PileCounts.DrawCount = DrawCount;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleDiscardGlyphPresentationSequenceSpec,
	"Wacom.UI.Battle.PresentationPlan.DiscardGlyphPrecedesReshuffleAndDraw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleDiscardGlyphPresentationSequenceSpec::RunTest(const FString&)
{
	using namespace WacomBattleDiscardGlyphPresentationSpec;
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

	const FGuid FirstDiscard = FGuid::NewGuid();
	const FGuid SecondDiscard = FGuid::NewGuid();
	const FGuid Drawn = FGuid::NewGuid();
	const FBattleSnapshot PreSnapshot = MakeSnapshot({ FirstDiscard, SecondDiscard }, 0, 0);
	const FBattleSnapshot PostSnapshot = MakeSnapshot({ Drawn }, 1, 0);
	const TArray<FGuid> DiscardIds = { FirstDiscard, SecondDiscard };

	FBattleEvent FirstDiscardEvent;
	FirstDiscardEvent.Type = EBattleEventType::CardDiscarded;
	FirstDiscardEvent.Sequence = 10;
	FirstDiscardEvent.CardInstanceId = FirstDiscard;
	FirstDiscardEvent.CardInstanceIds = DiscardIds;
	FirstDiscardEvent.HandCardZoneMoveBatchSequence = 10;
	FirstDiscardEvent.DiscardPileCountAfter = 2;
	FBattleEvent SecondDiscardEvent = FirstDiscardEvent;
	SecondDiscardEvent.Sequence = 11;
	SecondDiscardEvent.CardInstanceId = SecondDiscard;
	FBattleEvent HandLimitCompatibilityEvent;
	HandLimitCompatibilityEvent.Type = EBattleEventType::HandLimitDiscarded;
	HandLimitCompatibilityEvent.Sequence = 9;
	HandLimitCompatibilityEvent.CardInstanceIds = DiscardIds;

	FBattleEvent ReshuffleEvent;
	ReshuffleEvent.Type = EBattleEventType::DiscardPileReshuffledIntoDraw;
	ReshuffleEvent.Sequence = 12;
	ReshuffleEvent.CardInstanceIds = DiscardIds;
	ReshuffleEvent.DrawPileCountAfter = 2;
	ReshuffleEvent.DiscardPileCountAfter = 0;
	FBattleEvent DrawEvent;
	DrawEvent.Type = EBattleEventType::CardsDrawn;
	DrawEvent.Sequence = 13;
	DrawEvent.CardInstanceIds = { Drawn };
	DrawEvent.DrawPileCountAfter = 1;
	DrawEvent.DiscardPileCountAfter = 0;
	const TArray<FBattleEvent> Events = {
		HandLimitCompatibilityEvent, FirstDiscardEvent, SecondDiscardEvent, ReshuffleEvent, DrawEvent };
	FBattlePresentationJournal Journal;
	Journal.AppendDeckStepsFromEvents(Events);

	TestTrue(TEXT("ordinary discard command enqueues the generic deck presentation plan"),
		HUD->EnqueueCommandPresentationPlanForTest(
			Journal,
			Events,
			PreSnapshot,
			PostSnapshot));
	for (int32 Iteration = 0; HUD->IsBattlePresentationBusy() && Iteration < 24; ++Iteration)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}
	const TArray<FName> StartedPhases = HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const int32 DiscardIndex = StartedPhases.IndexOfByKey(FName(TEXT("HandDiscardGlyphTransfer")));
	const int32 ReshuffleIndex = StartedPhases.IndexOfByKey(FName(TEXT("DeckReshuffle")));
	const int32 DrawIndex = StartedPhases.IndexOfByKey(FName(TEXT("TurnStartDraw")));
	TestTrue(TEXT("discard glyph phase exists"), DiscardIndex != INDEX_NONE);
	TestTrue(TEXT("reshuffle phase exists"), ReshuffleIndex != INDEX_NONE);
	TestTrue(TEXT("draw phase exists"), DrawIndex != INDEX_NONE);
	TestTrue(TEXT("ordinary discard precedes reshuffle"), DiscardIndex < ReshuffleIndex);
	TestTrue(TEXT("reshuffle precedes the following draw batch"), ReshuffleIndex < DrawIndex);
	int32 DiscardPhaseCount = 0;
	for (const FName PhaseName : StartedPhases)
	{
		DiscardPhaseCount += PhaseName == FName(TEXT("HandDiscardGlyphTransfer")) ? 1 : 0;
	}
	TestEqual(TEXT("HandLimit compatibility fact does not duplicate the discard glyph phase"),
		DiscardPhaseCount,
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleDiscardGlyphReceiveFeedbackSpec,
	"Wacom.UI.Battle.PresentationPlan.DiscardGlyphArrivalUpdatesCountAndFeedbackOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleDiscardGlyphReceiveFeedbackSpec::RunTest(const FString&)
{
	using namespace WacomBattleDiscardGlyphPresentationSpec;
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
	HUD->CreatePileViewsForTest();
	UPileCountView* DiscardPileView = HUD->GetDiscardPileViewForTest();
	if (!TestNotNull(TEXT("DiscardPileView"), DiscardPileView))
	{
		return false;
	}

	HUD->PrimeDiscardPileReceiveFeedbackForTest(
		20,
		2,
		0);

	FWacomFirstPersonCardPileTransferProgressView Progress;
	Progress.EventSequence = 20;
	Progress.TransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
	Progress.TotalCount = 2;
	Progress.ArrivedCount = 1;
	HUD->HandlePileTransferProgressForTest(Progress);
	HUD->HandlePileTransferProgressForTest(Progress);
	TestEqual(TEXT("first arrival updates the discard count once"),
		DiscardPileView->GetCount(),
		1);
	FWacomPileCountViewTestAccess::Tick(*DiscardPileView, 0.04f);
	TestTrue(TEXT("duplicate progress does not add a second receive pulse"),
		FMath::IsNearlyEqual(
			DiscardPileView->GetRenderTransform().Scale.Y,
			0.94f,
			0.001f));

	DiscardPileView->ResetReceiveFeedback();
	Progress.ArrivedCount = 2;
	Progress.bCompleted = true;
	HUD->HandlePileTransferProgressForTest(Progress);
	TestEqual(TEXT("final arrival updates the discard count once"),
		DiscardPileView->GetCount(),
		2);
	FWacomPileCountViewTestAccess::Tick(*DiscardPileView, 0.04f);
	TestTrue(TEXT("final arrival uses the stronger receive pulse"),
		FMath::IsNearlyEqual(
			DiscardPileView->GetRenderTransform().Scale.Y,
			0.928f,
			0.001f));

	Progress.bWasForceCompleted = true;
	HUD->HandlePileTransferProgressForTest(Progress);
	FWacomPileCountViewTestAccess::Tick(*DiscardPileView, 0.04f);
	TestTrue(TEXT("force-complete clears feedback without replaying the final pulse"),
		DiscardPileView->GetRenderTransform().Scale.Equals(FVector2D(1.0f, 1.0f), 0.001f));
	return true;
}

#endif
