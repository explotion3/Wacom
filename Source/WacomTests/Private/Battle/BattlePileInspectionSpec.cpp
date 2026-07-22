// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "Snapshots/BattleSnapshot.h"

namespace
{
	bool IsDefinitionThenInstanceSorted(const TArray<FBattlePileCardSnapshot>& Cards)
	{
		for (int32 Index = 1; Index < Cards.Num(); ++Index)
		{
			const FBattlePileCardSnapshot& Previous = Cards[Index - 1];
			const FBattlePileCardSnapshot& Current = Cards[Index];
			const FString PreviousPath = GetPathNameSafe(Previous.Definition);
			const FString CurrentPath = GetPathNameSafe(Current.Definition);
			const int32 PathOrder = PreviousPath.Compare(CurrentPath, ESearchCase::CaseSensitive);
			if (PathOrder > 0
				|| (PathOrder == 0 && Previous.InstanceId.ToString() > Current.InstanceId.ToString()))
			{
				return false;
			}
		}
		return true;
	}

	bool SameCards(
		const TArray<FBattlePileCardSnapshot>& Left,
		const TArray<FBattlePileCardSnapshot>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].InstanceId != Right[Index].InstanceId
				|| Left[Index].Definition != Right[Index].Definition
				|| Left[Index].Location != Right[Index].Location
				|| Left[Index].RuntimeCost != Right[Index].RuntimeCost
				|| Left[Index].StatusStacks.OrderIndependentCompareEqual(
					Right[Index].StatusStacks) == false
				|| Left[Index].TemporaryKeywords != Right[Index].TemporaryKeywords)
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePileInspectionReadOnlyOrderHiddenSpec,
	"Wacom.Battle.PileInspection.ReadOnlySnapshotHidesDrawOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePileInspectionReadOnlyOrderHiddenSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Deck;
	for (int32 Index = 0; Index < 12; ++Index)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(Index % 4);
		Card->CardId = FName(*FString::Printf(TEXT("PileInspection.Card.%02d"), 11 - Index));
		Deck.Add(Card);
	}

	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50),
		/*Seed*/3721);
	if (!TestNotNull(TEXT("Battle session created"), Session))
	{
		return false;
	}

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FBattlePileInspectionSnapshot First = Session->BuildPileInspectionSnapshot();
	const FBattlePileInspectionSnapshot Second = Session->BuildPileInspectionSnapshot();
	const FBattleSnapshot After = Session->BuildSnapshot();

	TestEqual(TEXT("Inspection preserves battle version"), After.Version, Before.Version);
	TestEqual(TEXT("Inspection reports source battle version"), First.BattleVersion, Before.Version);
	TestEqual(TEXT("Inspection always exposes four semantic sections"), First.Sections.Num(), 4);

	const FBattlePileInspectionSectionSnapshot* Draw = First.FindSection(ECardLocation::Draw);
	const FBattlePileInspectionSectionSnapshot* Discard = First.FindSection(ECardLocation::Discard);
	const FBattlePileInspectionSectionSnapshot* Played = First.FindSection(ECardLocation::Played);
	const FBattlePileInspectionSectionSnapshot* Exhaust = First.FindSection(ECardLocation::Exhaust);
	if (!TestNotNull(TEXT("Draw section exists"), Draw)
		|| !TestNotNull(TEXT("Discard section exists"), Discard)
		|| !TestNotNull(TEXT("Played section exists"), Played)
		|| !TestNotNull(TEXT("Exhaust section exists"), Exhaust))
	{
		return false;
	}

	TestTrue(TEXT("Draw order is explicitly hidden"), Draw->bOrderHidden);
	TestFalse(TEXT("Discard order is not marked hidden"), Discard->bOrderHidden);
	TestFalse(TEXT("Played order is not marked hidden"), Played->bOrderHidden);
	TestFalse(TEXT("Exhaust order is not marked hidden"), Exhaust->bOrderHidden);
	TestEqual(TEXT("Draw inspection count matches authoritative count"), Draw->Count, Before.PileCounts.DrawCount);
	TestEqual(TEXT("Draw inspection includes every instance"), Draw->Cards.Num(), Draw->Count);
	TestTrue(TEXT("Draw output uses stable definition/id order"), IsDefinitionThenInstanceSorted(Draw->Cards));

	const FBattlePileInspectionSectionSnapshot* SecondDraw = Second.FindSection(ECardLocation::Draw);
	TestTrue(TEXT("Repeated inspection is deterministic"), SecondDraw && SameCards(Draw->Cards, SecondDraw->Cards));
	TestEqual(TEXT("Inspection does not change draw count"), After.PileCounts.DrawCount, Before.PileCounts.DrawCount);
	TestEqual(TEXT("Inspection does not change discard count"), After.PileCounts.DiscardCount, Before.PileCounts.DiscardCount);
	TestEqual(TEXT("Inspection does not change played count"), After.PileCounts.PlayedCount, Before.PileCounts.PlayedCount);
	TestEqual(TEXT("Inspection does not change exhaust count"), After.PileCounts.ExhaustCount, Before.PileCounts.ExhaustCount);
	return true;
}
