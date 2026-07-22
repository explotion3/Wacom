// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

namespace
{
	UCharacterDefinition* MakeJournalCharacter(FWacomBattleFixture& Fixture, int32 CardCost)
	{
		UCardDefinition* Left = Fixture.MakeNoopCard(CardCost);
		UCardDefinition* Right = Fixture.MakeNoopCard(CardCost);
		TArray<UCardDefinition*> Deck;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Deck.Add(Fixture.MakeNoopCard(CardCost));
		}
		return Fixture.MakeCharacter(Left, Right, Deck);
	}

	bool VerifySingleEnemyActionStep(
		FAutomationTestBase& Test,
		const FBattleResolution& Resolution,
		int32 ExpectedDamage)
	{
		Test.TestTrue(TEXT("Command succeeds"), Resolution.IsOk());
		Test.TestEqual(
			TEXT("Journal records exactly one enemy action"),
			Resolution.PresentationJournal.EnemyActionSteps.Num(),
			1);
		if (Resolution.PresentationJournal.EnemyActionSteps.Num() != 1)
		{
			return false;
		}

		const FBattlePresentationEnemyActionStep& Step =
			Resolution.PresentationJournal.EnemyActionSteps[0];
		Test.TestTrue(TEXT("Action step is valid"), Step.IsValid());
		Test.TestEqual(
			TEXT("Action snapshot captures player damage"),
			Step.SnapshotAfter.Player.CurrentHp,
			Step.SnapshotBefore.Player.CurrentHp - ExpectedDamage);
		Test.TestTrue(
			TEXT("Action event sequence range is ordered"),
			Step.LastEventSequence >= Step.FirstEventSequence);

		const FBattleEvent* FirstEvent = Resolution.Events.FindByPredicate(
			[Sequence = Step.FirstEventSequence](const FBattleEvent& Event)
			{
				return Event.Sequence == Sequence;
			});
		Test.TestNotNull(TEXT("First journal sequence resolves to an event"), FirstEvent);
		if (FirstEvent)
		{
			Test.TestEqual(
				TEXT("First journal event is EnemyPartActed"),
				FirstEvent->Type,
				EBattleEventType::EnemyPartActed);
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyActionJournalCommandsSpec,
	"Wacom.Battle.PresentationJournal.EnemyAction.Commands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyActionJournalCommandsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = Fixture.CreateSession(
			MakeJournalCharacter(Fixture, /*CardCost*/1),
			Fixture.MakeSinglePartEnemyWithIntentDamage(50, /*Initiative*/1, /*Damage*/5),
			/*Seed*/7);
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FHandCardSnapshot* Card = Before.Hand.Cards.FindByPredicate(
			[](const FHandCardSnapshot& Candidate)
			{
				return !Candidate.bIsHandAnchor && Candidate.InstanceId.IsValid();
			});
		const FGuid CardId = Card ? Card->InstanceId : FGuid();
		TestTrue(TEXT("PlayCard fixture provides a card"), CardId.IsValid());
		if (CardId.IsValid())
		{
			VerifySingleEnemyActionStep(
				*this,
				Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId)),
				/*ExpectedDamage*/5);
		}
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = Fixture.CreateSession(
			MakeJournalCharacter(Fixture, /*CardCost*/0),
			Fixture.MakeSinglePartEnemyWithIntentDamage(50, /*Initiative*/2, /*Damage*/4),
			/*Seed*/8);
		VerifySingleEnemyActionStep(
			*this,
			Session->ResolveCommand(FBattleCommand::MakeWait()),
			/*ExpectedDamage*/4);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = Fixture.CreateSession(
			MakeJournalCharacter(Fixture, /*CardCost*/0),
			Fixture.MakeSinglePartEnemyWithIntentDamage(50, /*Initiative*/20, /*Damage*/3),
			/*Seed*/9);
		VerifySingleEnemyActionStep(
			*this,
			Session->ResolveCommand(FBattleCommand::MakeEndTurn()),
			/*ExpectedDamage*/3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyActionJournalContinuitySpec,
	"Wacom.Battle.PresentationJournal.EnemyAction.MultipleStepsRemainContinuous",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyActionJournalContinuitySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UBattleSession* Session = Fixture.CreateSession(
		MakeJournalCharacter(Fixture, /*CardCost*/0),
		Fixture.MakeThreePartEnemy(
			/*HeadHp*/30,
			/*BodyHp*/30,
			/*TailHp*/30,
			/*HeadInitiative*/10,
			/*BodyInitiative*/11,
			/*TailInitiative*/12),
		/*Seed*/10);

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("EndTurn succeeds"), Resolution.IsOk());
	const TArray<FBattlePresentationEnemyActionStep>& Steps =
		Resolution.PresentationJournal.EnemyActionSteps;
	TestEqual(TEXT("Every living part records one action"), Steps.Num(), 3);
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		TestTrue(TEXT("Every action step is valid"), Steps[Index].IsValid());
		if (Index > 0)
		{
			TestEqual(
				TEXT("Adjacent action steps preserve player HP continuity"),
				Steps[Index].SnapshotBefore.Player.CurrentHp,
				Steps[Index - 1].SnapshotAfter.Player.CurrentHp);
			TestTrue(
				TEXT("Adjacent action sequence ranges remain ordered"),
				Steps[Index].FirstEventSequence > Steps[Index - 1].LastEventSequence);
		}
	}
	return true;
}
