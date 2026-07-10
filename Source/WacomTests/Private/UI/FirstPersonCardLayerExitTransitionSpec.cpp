// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerExitTransitionSpec
{
	FHandCardSnapshot MakeHandCard(const FGuid& CardInstanceId)
	{
		FHandCardSnapshot Card;
		Card.InstanceId = CardInstanceId;
		return Card;
	}

	FBattleSnapshot MakeSnapshotWithHand(const TArray<FHandCardSnapshot>& Cards)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Hand.Cards = Cards;
		Snapshot.Hand.NormalCardCount = Cards.Num();
		return Snapshot;
	}

	FBattleEvent MakeDiscardEvent(const FGuid& CardInstanceId)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardDiscarded;
		Event.CardInstanceId = CardInstanceId;
		return Event;
	}

	FBattleEvent MakePlayedEvent(const FGuid& CardInstanceId)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardPlayed;
		Event.CardInstanceId = CardInstanceId;
		return Event;
	}

	const FWacomFirstPersonCardLayerTransitionHint* FindHint(
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
	FWacomFirstPersonCardLayerPlayedHintKeepsCommittedTargetTest,
	"Wacom.UI.FirstPersonCardLayer.ExitTransition.PlayedHintKeepsCommittedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPlayedHintKeepsCommittedTargetTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerExitTransitionSpec;

	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FGuid PlayedCardId = FGuid::NewGuid();
	const FVector2D TargetWidgetPosition(840.0f, 260.0f);
	const FBattleSnapshot Previous = MakeSnapshotWithHand({ MakeHandCard(PlayedCardId) });
	const FBattleSnapshot Next = MakeSnapshotWithHand({});
	HUD->StoreFirstPersonCardTransitionEventsForTest({ MakePlayedEvent(PlayedCardId) });
	HUD->RecordFirstPersonPlayCommitForTest(
		PlayedCardId,
		FBattlePartSlotIdentity(),
		TargetWidgetPosition);

	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);
	const FWacomFirstPersonCardLayerTransitionHint* PlayedHint = FindHint(Hints, PlayedCardId);
	if (TestNotNull(TEXT("Played transition hint"), PlayedHint))
	{
		TestTrue(TEXT("Played hint carries a target widget position"), PlayedHint->bHasPlayedExitTargetWidgetPosition);
		TestEqual(
			TEXT("Played hint preserves the committed target position"),
			PlayedHint->PlayedExitTargetWidgetPosition,
			TargetWidgetPosition);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDiscardHintsCarryStableSequenceTest,
	"Wacom.UI.FirstPersonCardLayer.ExitTransition.DiscardHintsCarryStableSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDiscardHintsCarryStableSequenceTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerExitTransitionSpec;

	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FGuid FirstDiscardedId = FGuid::NewGuid();
	const FGuid SecondDiscardedId = FGuid::NewGuid();
	const FBattleSnapshot Previous = MakeSnapshotWithHand({
		MakeHandCard(FirstDiscardedId),
		MakeHandCard(SecondDiscardedId)
	});
	const FBattleSnapshot Next = MakeSnapshotWithHand({});

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		MakeDiscardEvent(FirstDiscardedId),
		MakeDiscardEvent(SecondDiscardedId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* FirstHint = FindHint(Hints, FirstDiscardedId);
	const FWacomFirstPersonCardLayerTransitionHint* SecondHint = FindHint(Hints, SecondDiscardedId);
	if (TestNotNull(TEXT("First discard hint"), FirstHint)
		&& TestNotNull(TEXT("Second discard hint"), SecondHint))
	{
		TestEqual(TEXT("First discard sequence index"), FirstHint->SequenceIndex, 0);
		TestEqual(TEXT("Second discard sequence index"), SecondHint->SequenceIndex, 1);
		TestEqual(TEXT("First discard sequence count"), FirstHint->SequenceCount, 2);
		TestEqual(TEXT("Second discard sequence count"), SecondHint->SequenceCount, 2);
	}
	return true;
}

#endif
