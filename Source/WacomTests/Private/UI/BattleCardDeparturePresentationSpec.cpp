// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCardDeparturePresentationSpec
{
	FHandCardSnapshot MakeHandCard(const FGuid& CardInstanceId)
	{
		FHandCardSnapshot Card;
		Card.InstanceId = CardInstanceId;
		return Card;
	}

	FBattleSnapshot MakeSnapshot(std::initializer_list<FGuid> CardIds)
	{
		FBattleSnapshot Snapshot;
		for (const FGuid& CardId : CardIds)
		{
			Snapshot.Hand.Cards.Add(MakeHandCard(CardId));
		}
		Snapshot.Hand.NormalCardCount = Snapshot.Hand.Cards.Num();
		return Snapshot;
	}

	FBattleEvent MakeEvent(EBattleEventType Type, const FGuid& CardId)
	{
		FBattleEvent Event;
		Event.Type = Type;
		Event.CardInstanceId = CardId;
		return Event;
	}

	const FWacomFirstPersonCardLayerTransitionHint* FindHint(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints,
		const FGuid& CardId)
	{
		return Hints.FindByPredicate(
			[&CardId](const FWacomFirstPersonCardLayerTransitionHint& Hint)
			{
				return Hint.CardInstanceId == CardId;
			});
	}

	const FWacomFirstPersonCardLayerFeedbackHint* FindFeedbackHint(
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& Hints,
		const FGuid& CardId)
	{
		return Hints.FindByPredicate(
			[&CardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == CardId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardDeparturePresentationMappingTest,
	"Wacom.UI.FirstPersonCardLayer.DeparturePresentation.MapsPlayedAndExhausted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardDeparturePresentationMappingTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCardDeparturePresentationSpec;
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>();
	if (!TestNotNull(TEXT("Battle HUD presentation fixture"), HUD))
	{
		return false;
	}

	const FGuid NormalPlayedId = FGuid::NewGuid();
	const FGuid ExhaustedPlayedId = FGuid::NewGuid();
	const FGuid DirectExhaustedId = FGuid::NewGuid();
	const FGuid LegacyPlayedId = FGuid::NewGuid();
	const FBattleSnapshot Previous = MakeSnapshot({
		NormalPlayedId,
		ExhaustedPlayedId,
		DirectExhaustedId,
		LegacyPlayedId });
	const FBattleSnapshot Next = MakeSnapshot({});

	FBattleEvent NormalDestination = MakeEvent(
		EBattleEventType::CardPlayDestinationResolved,
		NormalPlayedId);
	NormalDestination.CardDestination = ECardLocation::Played;
	FBattleEvent ExhaustDestination = MakeEvent(
		EBattleEventType::CardPlayDestinationResolved,
		ExhaustedPlayedId);
	ExhaustDestination.CardDestination = ECardLocation::Exhaust;
	HUD->StoreFirstPersonCardTransitionEventsForTest({
		MakeEvent(EBattleEventType::CardPlayed, NormalPlayedId),
		NormalDestination,
		MakeEvent(EBattleEventType::CardPlayed, ExhaustedPlayedId),
		MakeEvent(EBattleEventType::CardExhausted, ExhaustedPlayedId),
		ExhaustDestination,
		MakeEvent(EBattleEventType::CardExhausted, DirectExhaustedId),
		MakeEvent(EBattleEventType::CardPlayed, LegacyPlayedId) });

	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);
	TestEqual(TEXT("Each removed card receives one departure hint"), Hints.Num(), 4);
	const FWacomFirstPersonCardLayerTransitionHint* NormalHint = FindHint(Hints, NormalPlayedId);
	const FWacomFirstPersonCardLayerTransitionHint* ExhaustedHint = FindHint(Hints, ExhaustedPlayedId);
	const FWacomFirstPersonCardLayerTransitionHint* DirectExhaustedHint = FindHint(Hints, DirectExhaustedId);
	const FWacomFirstPersonCardLayerTransitionHint* LegacyHint = FindHint(Hints, LegacyPlayedId);
	if (TestNotNull(TEXT("Normal Played hint"), NormalHint))
	{
		TestEqual(TEXT("Normal destination maps to Played"), NormalHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Played);
	}
	if (TestNotNull(TEXT("Exhausted Played hint"), ExhaustedHint))
	{
		TestEqual(TEXT("Final Exhaust destination maps to Exhausted"), ExhaustedHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	}
	if (TestNotNull(TEXT("Direct Exhausted hint"), DirectExhaustedHint))
	{
		TestEqual(TEXT("Direct CardExhausted maps to Exhausted"), DirectExhaustedHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Exhausted);
	}
	if (TestNotNull(TEXT("Legacy Played hint"), LegacyHint))
	{
		TestEqual(TEXT("Missing destination fact falls back to Played"), LegacyHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Played);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardUseReformPresentationMappingTest,
	"Wacom.UI.FirstPersonCardLayer.DeparturePresentation.RetainedPlayedUsesReform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardUseReformPresentationMappingTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCardDeparturePresentationSpec;
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>();
	if (!TestNotNull(TEXT("Battle HUD presentation fixture"), HUD))
	{
		return false;
	}

	const FGuid AcceptedRetainedPlayedId = FGuid::NewGuid();
	const FGuid EventOnlyRetainedPlayedId = FGuid::NewGuid();
	const FGuid RemovedPlayedId = FGuid::NewGuid();
	const FBattleSnapshot Next = MakeSnapshot({
		AcceptedRetainedPlayedId,
		EventOnlyRetainedPlayedId });
	HUD->StoreFirstPersonCardTransitionEventsForTest({
		MakeEvent(EBattleEventType::CardPlayed, AcceptedRetainedPlayedId),
		MakeEvent(EBattleEventType::CardPlayed, EventOnlyRetainedPlayedId),
		MakeEvent(EBattleEventType::CardPlayed, RemovedPlayedId) });
	HUD->RecordFirstPersonPlayCommitForTest(AcceptedRetainedPlayedId);
	HUD->RecordFirstPersonPlayCommitForTest(RemovedPlayedId);

	const TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints =
		HUD->BuildFirstPersonCardFeedbackHintsForTest(Next);
	TestEqual(TEXT("Only accepted Played card still in hand reforms"), Hints.Num(), 1);
	const FWacomFirstPersonCardLayerFeedbackHint* ReformHint =
		FindFeedbackHint(Hints, AcceptedRetainedPlayedId);
	if (TestNotNull(TEXT("Retained Played reform hint"), ReformHint))
	{
		TestEqual(
			TEXT("Retained Played maps to CardUseReform"),
			ReformHint->FeedbackKind,
			EWacomFirstPersonCardLayerFeedbackKind::CardUseReform);
	}
	TestNull(
		TEXT("Unaccepted event-only card does not reform"),
		FindFeedbackHint(Hints, EventOnlyRetainedPlayedId));
	TestNull(
		TEXT("Removed Played card stays on departure path"),
		FindFeedbackHint(Hints, RemovedPlayedId));
	return true;
}

#endif
