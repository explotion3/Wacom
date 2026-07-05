// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "GameFramework/PlayerController.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleRetainedFeedbackPresentationSpec
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

	FHandCardSnapshot MakeHandCard(bool bIsHandAnchor = false)
	{
		FHandCardSnapshot Snapshot;
		Snapshot.InstanceId = FGuid::NewGuid();
		Snapshot.RuntimeCost = 1;
		Snapshot.Zone = EHandZone::Both;
		Snapshot.bIsPlayable = true;
		Snapshot.bIsHandAnchor = bIsHandAnchor;
		return Snapshot;
	}

	FBattleSnapshot MakeSnapshotWithHand(const TArray<FHandCardSnapshot>& Cards)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.Hand.Cards = Cards;
		for (const FHandCardSnapshot& Card : Cards)
		{
			if (!Card.bIsHandAnchor)
			{
				++Snapshot.Hand.NormalCardCount;
			}
		}
		return Snapshot;
	}

	FBattleEvent MakeCardsRetainedEvent(const TArray<FGuid>& CardInstanceIds)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardsRetained;
		Event.CardInstanceIds = CardInstanceIds;
		Event.Count = Event.CardInstanceIds.Num();
		return Event;
	}

	const FWacomFirstPersonCardLayerFeedbackHint* FindFeedbackHint(
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& Hints,
		const FGuid& CardInstanceId)
	{
		return Hints.FindByPredicate(
			[&CardInstanceId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == CardInstanceId;
			});
	}

	const FWacomFirstPersonCardLayerTransitionHint* FindTransitionHint(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints,
		const FGuid& CardInstanceId)
	{
		return Hints.FindByPredicate(
			[&CardInstanceId](const FWacomFirstPersonCardLayerTransitionHint& Hint)
			{
				return Hint.CardInstanceId == CardInstanceId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardsRetainedFeedbackHintsTest,
	"Wacom.UI.Battle.RetainedFeedback.CardsRetainedBuildsNormalHandFeedbackHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardsRetainedFeedbackHintsTest::RunTest(const FString& Parameters)
{
	using namespace WacomBattleRetainedFeedbackPresentationSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->SetWorldForTest(World);
	HUD->SetOwningPlayerForTest(PC);

	const FHandCardSnapshot RetainedCard = MakeHandCard();
	const FHandCardSnapshot OtherRetainedCard = MakeHandCard();
	const FHandCardSnapshot HandAnchor = MakeHandCard(true);
	const FHandCardSnapshot RemovedCard = MakeHandCard();
	const FBattleSnapshot Previous = MakeSnapshotWithHand({ RetainedCard, OtherRetainedCard, HandAnchor });
	const FBattleSnapshot Next = MakeSnapshotWithHand({ RetainedCard, OtherRetainedCard, HandAnchor });
	const FGuid InvalidId;

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		MakeCardsRetainedEvent({
			RetainedCard.InstanceId,
			HandAnchor.InstanceId,
			InvalidId,
			RetainedCard.InstanceId,
			RemovedCard.InstanceId,
			OtherRetainedCard.InstanceId }) });

	const TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints =
		HUD->BuildFirstPersonCardFeedbackHintsForTest(Next);
	TestEqual(TEXT("Only normal retained cards receive feedback hints"), FeedbackHints.Num(), 2);

	const FWacomFirstPersonCardLayerFeedbackHint* FirstHint =
		FindFeedbackHint(FeedbackHints, RetainedCard.InstanceId);
	const FWacomFirstPersonCardLayerFeedbackHint* SecondHint =
		FindFeedbackHint(FeedbackHints, OtherRetainedCard.InstanceId);
	if (TestNotNull(TEXT("First retained feedback hint"), FirstHint))
	{
		TestEqual(TEXT("First retained feedback kind"), FirstHint->FeedbackKind, EWacomFirstPersonCardLayerFeedbackKind::Retained);
		TestEqual(TEXT("First retained sequence index"), FirstHint->SequenceIndex, 0);
		TestEqual(TEXT("First retained sequence count"), FirstHint->SequenceCount, 2);
	}
	if (TestNotNull(TEXT("Second retained feedback hint"), SecondHint))
	{
		TestEqual(TEXT("Second retained feedback kind"), SecondHint->FeedbackKind, EWacomFirstPersonCardLayerFeedbackKind::Retained);
		TestEqual(TEXT("Second retained sequence index"), SecondHint->SequenceIndex, 1);
		TestEqual(TEXT("Second retained sequence count"), SecondHint->SequenceCount, 2);
	}
	TestNull(
		TEXT("Hand anchor does not receive retained feedback"),
		FindFeedbackHint(FeedbackHints, HandAnchor.InstanceId));
	TestNull(
		TEXT("Removed card does not receive retained feedback"),
		FindFeedbackHint(FeedbackHints, RemovedCard.InstanceId));

	const TArray<FWacomFirstPersonCardLayerTransitionHint> TransitionHints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);
	TestEqual(TEXT("CardsRetained does not create transition hints"), TransitionHints.Num(), 0);
	TestNull(
		TEXT("Retained card has no transition hint"),
		FindTransitionHint(TransitionHints, RetainedCard.InstanceId));

	PC->Destroy();
	return true;
}

#endif
