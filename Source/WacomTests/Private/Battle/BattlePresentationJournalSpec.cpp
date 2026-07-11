// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/HandSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	const FBattleEvent* FindEventByType(
		const TArray<FBattleEvent>& Events,
		EBattleEventType Type)
	{
		return Events.FindByPredicate(
			[Type](const FBattleEvent& Event)
			{
				return Event.Type == Type;
			});
	}

	bool ContainsGuid(const TArray<FGuid>& Ids, const FGuid& Id)
	{
		return Ids.Contains(Id);
	}

	bool SnapshotHandContainsId(const FBattleSnapshot& Snapshot, const FGuid& Id)
	{
		return Snapshot.Hand.Cards.ContainsByPredicate(
			[Id](const FHandCardSnapshot& Card)
			{
				return Card.InstanceId == Id;
			});
	}

	bool IsRetainedTestSetupReady(
		const FBattleSnapshot& Snapshot,
		const UCardDefinition* RetainCard,
		FGuid& OutRetainedId,
		bool& bOutHasDiscardCandidate)
	{
		OutRetainedId.Invalidate();
		bOutHasDiscardCandidate = false;

		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.bIsHandAnchor)
			{
				continue;
			}

			if (Card.Definition == RetainCard)
			{
				OutRetainedId = Card.InstanceId;
			}
			else
			{
				bOutHasDiscardCandidate = true;
			}
		}

		return OutRetainedId.IsValid()
			&& bOutHasDiscardCandidate
			&& Snapshot.PileCounts.DrawCount > 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEndTurnPresentationJournalSpec,
	"Wacom.Battle.PresentationJournal.EndTurnRecordsDiscardRetainDrawCheckpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEndTurnPresentationJournalSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* RetainCard = Fx.MakeNoopCard(/*Cost*/0);
	RetainCard->Keywords.AddTag(WacomTags::Card_Keyword_Retain);

	TArray<UCardDefinition*> Deck;
	Deck.Add(RetainCard);
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
	}

	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(/*Cost*/0),
		nullptr,
		Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemyWithIntentDamage(
		/*Hp*/40,
		/*Initiative*/1,
		/*IntentResist*/0,
		/*Damage*/0);

	UBattleSession* Session = nullptr;
	FBattleSnapshot PreEndTurnSnapshot;
	FGuid RetainedId;
	bool bHasDiscardCandidate = false;
	for (int32 Seed = 1; Seed <= 64; ++Seed)
	{
		Session = Fx.CreateSession(Character, Enemy, Seed);
		PreEndTurnSnapshot = Session->BuildSnapshot();
		if (IsRetainedTestSetupReady(
			PreEndTurnSnapshot,
			RetainCard,
			RetainedId,
			bHasDiscardCandidate))
		{
			break;
		}
	}

	if (!TestNotNull(TEXT("Session"), Session)
		|| !TestTrue(TEXT("Retained card appears in first hand"), RetainedId.IsValid())
		|| !TestTrue(TEXT("Hand has at least one discard candidate"), bHasDiscardCandidate))
	{
		return false;
	}

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("EndTurn command succeeds"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;
	const FBattlePresentationJournal& Journal = Resolution.PresentationJournal;
	TestEqual(TEXT("EndTurn journal has three hand checkpoints"), Journal.Checkpoints.Num(), 3);
	if (Journal.Checkpoints.Num() == 3)
	{
		TestEqual(
			TEXT("First checkpoint is discard"),
			Journal.Checkpoints[0].Type,
			EBattlePresentationCheckpointType::TurnEndDiscardResolved);
		TestEqual(
			TEXT("Second checkpoint is retain"),
			Journal.Checkpoints[1].Type,
			EBattlePresentationCheckpointType::TurnEndRetainResolved);
		TestEqual(
			TEXT("Third checkpoint is draw"),
			Journal.Checkpoints[2].Type,
			EBattlePresentationCheckpointType::TurnStartDrawResolved);

		TArray<FGuid> AnchorIds;
		for (const FHandCardSnapshot& Card : PreEndTurnSnapshot.Hand.Cards)
		{
			if (Card.bIsHandAnchor)
			{
				AnchorIds.Add(Card.InstanceId);
			}
		}

		TestTrue(TEXT("Discard checkpoint records discarded hand cards"), Journal.Checkpoints[0].CardInstanceIds.Num() > 0);
		TestTrue(TEXT("Retain checkpoint records retained card"), ContainsGuid(Journal.Checkpoints[1].CardInstanceIds, RetainedId));
		for (const FGuid& AnchorId : AnchorIds)
		{
			TestFalse(
				TEXT("Retain checkpoint excludes hand anchors"),
				ContainsGuid(Journal.Checkpoints[1].CardInstanceIds, AnchorId));
		}
		TestTrue(TEXT("Draw checkpoint records drawn cards"), Journal.Checkpoints[2].CardInstanceIds.Num() > 0);
		for (const FGuid& DrawnId : Journal.Checkpoints[2].CardInstanceIds)
		{
			TestTrue(TEXT("Draw checkpoint id is visible in draw snapshot"), SnapshotHandContainsId(Journal.Checkpoints[2].Snapshot, DrawnId));
		}
	}

	const FBattleEvent* RetainEvent = FindEventByType(Events, EBattleEventType::CardsRetained);
	TestNotNull(TEXT("CardsRetained event emitted"), RetainEvent);
	if (RetainEvent)
	{
		TestEqual(TEXT("CardsRetained count matches ids"), RetainEvent->Count, RetainEvent->CardInstanceIds.Num());
		TestTrue(TEXT("CardsRetained includes retained id"), RetainEvent->CardInstanceIds.Contains(RetainedId));
	}

	const FBattleEvent* DrawEvent = FindEventByType(Events, EBattleEventType::CardsDrawn);
	TestNotNull(TEXT("CardsDrawn event emitted"), DrawEvent);
	if (DrawEvent && Journal.Checkpoints.Num() == 3)
	{
		TestEqual(
			TEXT("Draw checkpoint id count matches CardsDrawn count"),
			Journal.Checkpoints[2].CardInstanceIds.Num(),
			DrawEvent->CardInstanceIds.Num());
		TestEqual(TEXT("CardsDrawn count matches ids"), DrawEvent->Count, DrawEvent->CardInstanceIds.Num());
	}

	return true;
}
