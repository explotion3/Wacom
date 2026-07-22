// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Commands/BattleCommand.h"
#include "Cards/CardDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleDiscardToDrawReshuffleFactSpec,
	"Wacom.Battle.Deck.Reshuffle.EmitsOrderedExactFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleDiscardToDrawReshuffleFactSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Deck;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(0);
		Card->CardId = FName(*FString::Printf(TEXT("Reshuffle.Card.%d"), Index));
		Deck.Add(Card);
	}
	UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
	UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(LeftHand, RightHand, Deck),
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0),
		17);

	const FBattleSnapshot Before = Session->BuildSnapshot();
	TestEqual(TEXT("Opening draw exhausts draw pile"), Before.PileCounts.DrawCount, 0);
	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	if (!TestTrue(TEXT("End turn succeeds"), Resolution.IsOk()))
	{
		return false;
	}

	const int32 ReshuffleEventIndex = Resolution.Events.IndexOfByPredicate([](const FBattleEvent& Event)
	{
		return Event.Type == EBattleEventType::DiscardPileReshuffledIntoDraw;
	});
	const int32 DrawEventIndex = Resolution.Events.IndexOfByPredicate([](const FBattleEvent& Event)
	{
		return Event.Type == EBattleEventType::CardsDrawn;
	});
	TestTrue(TEXT("Reshuffle fact precedes following draw batch"),
		ReshuffleEventIndex != INDEX_NONE && DrawEventIndex > ReshuffleEventIndex);
	if (ReshuffleEventIndex != INDEX_NONE)
	{
		const FBattleEvent& Event = Resolution.Events[ReshuffleEventIndex];
		TestTrue(TEXT("At least one discarded card is reshuffled"), Event.Count > 0);
		TestEqual(TEXT("Reshuffle fact carries every exact id"), Event.CardInstanceIds.Num(), Event.Count);
		TestEqual(TEXT("Reshuffle post draw count matches moved cards"), Event.DrawPileCountAfter, Event.Count);
		TestEqual(TEXT("Reshuffle post discard count"), Event.DiscardPileCountAfter, 0);
	}

	TestEqual(TEXT("Journal records reshuffle then draw"), Resolution.PresentationJournal.DeckSteps.Num(), 2);
	if (Resolution.PresentationJournal.DeckSteps.Num() == 2)
	{
		TestEqual(TEXT("First deck step is reshuffle"),
			Resolution.PresentationJournal.DeckSteps[0].Kind,
			EBattlePresentationDeckStepKind::DiscardPileReshuffledIntoDraw);
		TestEqual(TEXT("Second deck step is draw"),
			Resolution.PresentationJournal.DeckSteps[1].Kind,
			EBattlePresentationDeckStepKind::DrawBatch);
	}
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Five normal cards returned to hand"), After.Hand.NormalCardCount, 5);
	TestEqual(TEXT("Discard is empty after refill draw"), After.PileCounts.DiscardCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePartialDrawBeforeReshuffleSpec,
	"Wacom.Battle.Deck.Reshuffle.PartialDrawPreservesStepOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePartialDrawBeforeReshuffleSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Deck;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(0);
		Card->CardId = FName(*FString::Printf(TEXT("PartialReshuffle.Card.%d"), Index));
		Deck.Add(Card);
	}
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0),
		23);
	TestEqual(TEXT("Two cards remain before end-turn draw"),
		Session->BuildSnapshot().PileCounts.DrawCount,
		2);

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	if (!TestTrue(TEXT("End turn succeeds"), Resolution.IsOk()))
	{
		return false;
	}
	const TArray<FBattlePresentationDeckStep>& Steps = Resolution.PresentationJournal.DeckSteps;
	TestEqual(TEXT("Partial refill has three ordered steps"), Steps.Num(), 3);
	if (Steps.Num() == 3)
	{
		TestEqual(TEXT("Step 0 draws current pile"), Steps[0].Kind, EBattlePresentationDeckStepKind::DrawBatch);
		TestEqual(TEXT("Step 0 draws two"), Steps[0].CardInstanceIds.Num(), 2);
		TestEqual(TEXT("Step 1 reshuffles discard"), Steps[1].Kind, EBattlePresentationDeckStepKind::DiscardPileReshuffledIntoDraw);
		TestEqual(TEXT("Step 2 draws remaining request"), Steps[2].Kind, EBattlePresentationDeckStepKind::DrawBatch);
		TestEqual(TEXT("Step 2 draws three"), Steps[2].CardInstanceIds.Num(), 3);
		TestTrue(TEXT("Reshuffle contains enough cards for final batch"),
			Steps[1].CardInstanceIds.Num() >= Steps[2].CardInstanceIds.Num());
		TestTrue(TEXT("Event sequence remains strictly increasing"),
			Steps[0].EventSequence < Steps[1].EventSequence
			&& Steps[1].EventSequence < Steps[2].EventSequence);
	}
	return true;
}

#endif
