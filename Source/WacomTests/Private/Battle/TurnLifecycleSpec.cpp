// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Misc/AutomationTest.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	int32 FindEventIndex(const TArray<FBattleEvent>& Events, EBattleEventType Type, int32 StartIndex = 0)
	{
		for (int32 Index = FMath::Max(0, StartIndex); Index < Events.Num(); ++Index)
		{
			if (Events[Index].Type == Type)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	int32 CountEvents(const TArray<FBattleEvent>& Events, EBattleEventType Type)
	{
		int32 Count = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == Type)
			{
				++Count;
			}
		}
		return Count;
	}

	const FBattleEvent* FindEventBySequence(const TArray<FBattleEvent>& Events, int32 Sequence)
	{
		return Events.FindByPredicate(
			[Sequence](const FBattleEvent& Event)
			{
				return Event.Sequence == Sequence;
			});
	}

	const FBattlePresentationCheckpoint* FindCheckpoint(
		const FBattlePresentationJournal& Journal,
		EBattlePresentationCheckpointType Type)
	{
		return Journal.Checkpoints.FindByPredicate(
			[Type](const FBattlePresentationCheckpoint& Checkpoint)
			{
				return Checkpoint.Type == Type;
			});
	}

	void AddShieldPassive(UCardDefinition& Card, FGameplayTag Trigger, int32 Amount)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Status_Shield;
		Effect.Magnitude = Amount;
		Effect.Target = WacomTags::Target_Player;

		FCardPassive Passive;
		Passive.Trigger = Trigger;
		Passive.Effects.Add(Effect);
		Card.Passives.Add(Passive);
	}

	void AddLethalOnDiscard(UCardDefinition& Card)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = 1000;
		Effect.Target = WacomTags::Target_Player;

		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnDiscard;
		Passive.Effects.Add(Effect);
		Card.Passives.Add(Passive);
	}

	UBattleSession* CreateLifecycleSession(
		FWacomBattleFixture& Fixture,
		UEnemyDefinition* Enemy,
		TArray<UCardDefinition*> Deck,
		UCardDefinition*& OutLeftHand,
		UCardDefinition*& OutRightHand,
		int32 Seed = 1)
	{
		OutLeftHand = Fixture.MakeNoopCard(0);
		OutRightHand = Fixture.MakeNoopCard(0);
		return Fixture.CreateSession(
			Fixture.MakeCharacter(OutLeftHand, OutRightHand, Deck),
			Enemy,
			Seed);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnLifecycleInitialStartSpec,
	"Wacom.Battle.TurnLifecycle.InitialStartKeepsEventAndStateContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnLifecycleInitialStartSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Deck;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(0));
	}

	UCardDefinition* LeftHand = nullptr;
	UCardDefinition* RightHand = nullptr;
	UBattleSession* Session = CreateLifecycleSession(
		Fixture,
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 0),
		Deck,
		LeftHand,
		RightHand,
		/*Seed*/3);

	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const int32 BattleStartedIndex = FindEventIndex(Events, EBattleEventType::BattleStarted);
	const int32 TurnStartedIndex = FindEventIndex(Events, EBattleEventType::TurnStarted);
	const int32 CardsDrawnIndex = FindEventIndex(Events, EBattleEventType::CardsDrawn);
	const int32 HandZoneChangedIndex = FindEventIndex(Events, EBattleEventType::HandZoneChanged);
	int32 LastInitialEnemyFactIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		if (Events[Index].Type == EBattleEventType::EnemyPhaseChanged
			|| Events[Index].Type == EBattleEventType::EnemyIntentSelected)
		{
			LastInitialEnemyFactIndex = Index;
		}
	}

	TestTrue(TEXT("BattleStarted precedes initial enemy facts"),
		BattleStartedIndex != INDEX_NONE && BattleStartedIndex < LastInitialEnemyFactIndex);
	TestTrue(TEXT("TurnStarted follows all initial enemy facts"),
		LastInitialEnemyFactIndex != INDEX_NONE && LastInitialEnemyFactIndex < TurnStartedIndex);
	TestTrue(TEXT("TurnStarted precedes CardsDrawn"),
		TurnStartedIndex != INDEX_NONE && TurnStartedIndex < CardsDrawnIndex);
	TestTrue(TEXT("CardsDrawn precedes HandZoneChanged"),
		CardsDrawnIndex != INDEX_NONE && CardsDrawnIndex < HandZoneChangedIndex);
	TestEqual(TEXT("Initial lifecycle emits TurnStarted exactly once"),
		CountEvents(Events, EBattleEventType::TurnStarted), 1);
	if (CardsDrawnIndex != INDEX_NONE)
	{
		TestEqual(TEXT("Initial lifecycle emits five drawn-card facts"),
			Events[CardsDrawnIndex].Count, 5);
	}
	TestEqual(TEXT("Initial phase reaches PlayerAction"), Snapshot.Phase, EBattlePhase::PlayerAction);
	TestEqual(TEXT("Initial turn number"), Snapshot.TurnNumber, 1);
	TestEqual(TEXT("Initial wait value"), Snapshot.CurrentWaitValue, 2);
	TestEqual(TEXT("Initial StateVersion increments once"), Snapshot.Version, 1);
	TestTrue(TEXT("Initial lifecycle does not write command presentation journal"),
		Session->ConsumePresentationJournal().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnLifecycleNormalCompletionSpec,
	"Wacom.Battle.TurnLifecycle.NormalCompletionOwnsOrderAndCheckpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnLifecycleNormalCompletionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* PlayedCard = Fixture.MakeNoopCard(0);
	PlayedCard->CardId = TEXT("TurnLifecycle.PlayedCard");
	TArray<UCardDefinition*> Deck = { PlayedCard };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(0));
	}

	UCardDefinition* LeftHand = nullptr;
	UCardDefinition* RightHand = nullptr;
	UBattleSession* Session = CreateLifecycleSession(
		Fixture,
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 0),
		Deck,
		LeftHand,
		RightHand,
		/*Seed*/11);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid PlayedCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PlayedCard->CardId);
	const FGuid LeftHandId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftHand->CardId);
	if (!TestTrue(TEXT("Named played card appears in opening hand"), PlayedCardId.IsValid())
		|| !TestTrue(TEXT("Left anchor appears in opening hand"), LeftHandId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Play normal card into PlayedPile"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(PlayedCardId)).IsOk());
	TestTrue(TEXT("Play left anchor to disable Both-zone retention"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(LeftHandId)).IsOk());
	const FBattleSnapshot BeforeEndTurn = Session->BuildSnapshot();
	Session->ConsumeEvents();
	Session->ConsumePresentationJournal();

	TestTrue(TEXT("EndTurn succeeds"),
		Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	const FBattlePresentationJournal Journal = Session->ConsumePresentationJournal();
	const FBattleSnapshot AfterEndTurn = Session->BuildSnapshot();

	const int32 TurnEndedIndex = FindEventIndex(Events, EBattleEventType::TurnEnded);
	const int32 FirstDiscardIndex = FindEventIndex(Events, EBattleEventType::CardDiscarded);
	const int32 EnemyActedIndex = FindEventIndex(Events, EBattleEventType::EnemyPartActed);
	const int32 CardsDrawnIndex = FindEventIndex(Events, EBattleEventType::CardsDrawn);
	const int32 FinalHandZoneIndex = FindEventIndex(Events, EBattleEventType::HandZoneChanged, CardsDrawnIndex);
	TestTrue(TEXT("TurnEnded precedes turn-end hand discard"),
		TurnEndedIndex != INDEX_NONE && TurnEndedIndex < FirstDiscardIndex);
	TestTrue(TEXT("Discard finishes before enemy action"),
		FirstDiscardIndex != INDEX_NONE && FirstDiscardIndex < EnemyActedIndex);
	TestTrue(TEXT("Enemy action finishes before next-turn draw"),
		EnemyActedIndex != INDEX_NONE && EnemyActedIndex < CardsDrawnIndex);
	TestTrue(TEXT("CardsDrawn precedes final HandZoneChanged"),
		CardsDrawnIndex != INDEX_NONE && CardsDrawnIndex < FinalHandZoneIndex);
	TestEqual(TEXT("Following turn does not publish TurnStarted"),
		CountEvents(Events, EBattleEventType::TurnStarted), 0);
	TestFalse(TEXT("PlayedPile natural cleanup does not publish CardDiscarded"),
		Events.ContainsByPredicate(
			[PlayedCardId](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::CardDiscarded
					&& Event.CardInstanceId == PlayedCardId;
			}));

	const FBattlePresentationCheckpoint* DiscardCheckpoint = FindCheckpoint(
		Journal,
		EBattlePresentationCheckpointType::TurnEndDiscardResolved);
	const FBattlePresentationCheckpoint* RetainCheckpoint = FindCheckpoint(
		Journal,
		EBattlePresentationCheckpointType::TurnEndRetainResolved);
	const FBattlePresentationCheckpoint* DrawCheckpoint = FindCheckpoint(
		Journal,
		EBattlePresentationCheckpointType::TurnStartDrawResolved);
	TestNotNull(TEXT("Discard checkpoint exists"), DiscardCheckpoint);
	TestNull(TEXT("No explicit retain checkpoint with one anchor"), RetainCheckpoint);
	TestNotNull(TEXT("Draw checkpoint exists"), DrawCheckpoint);
	if (DiscardCheckpoint)
	{
		const FBattleEvent* FirstRangeEvent = FindEventBySequence(Events, DiscardCheckpoint->FirstEventSequence);
		const FBattleEvent* LastRangeEvent = FindEventBySequence(Events, DiscardCheckpoint->LastEventSequence);
		TestTrue(TEXT("Discard range starts at CardDiscarded"),
			FirstRangeEvent && FirstRangeEvent->Type == EBattleEventType::CardDiscarded);
		TestTrue(TEXT("Discard range ends at batch HandZoneChanged"),
			LastRangeEvent && LastRangeEvent->Type == EBattleEventType::HandZoneChanged);
		TestEqual(TEXT("Discard checkpoint stays in TurnEnd"),
			DiscardCheckpoint->Snapshot.Phase, EBattlePhase::TurnEnd);
		TestEqual(TEXT("PlayedPile is already naturally cleaned at discard checkpoint"),
			DiscardCheckpoint->Snapshot.PileCounts.PlayedCount, 0);
	}
	if (DrawCheckpoint)
	{
		const FBattleEvent* FirstRangeEvent = FindEventBySequence(Events, DrawCheckpoint->FirstEventSequence);
		const FBattleEvent* LastRangeEvent = FindEventBySequence(Events, DrawCheckpoint->LastEventSequence);
		TestTrue(TEXT("Draw range starts at CardsDrawn"),
			FirstRangeEvent && FirstRangeEvent->Type == EBattleEventType::CardsDrawn);
		TestTrue(TEXT("Draw range ends at HandZoneChanged"),
			LastRangeEvent && LastRangeEvent->Type == EBattleEventType::HandZoneChanged);
		TestEqual(TEXT("Draw checkpoint is final PlayerAction state"),
			DrawCheckpoint->Snapshot.Phase, EBattlePhase::PlayerAction);
		TestEqual(TEXT("Draw checkpoint belongs to next turn"),
			DrawCheckpoint->Snapshot.TurnNumber, 2);
	}

	TestEqual(TEXT("Normal completion advances one turn"), AfterEndTurn.TurnNumber, 2);
	TestEqual(TEXT("Normal completion returns to PlayerAction"),
		AfterEndTurn.Phase, EBattlePhase::PlayerAction);
	TestEqual(TEXT("Normal completion increments StateVersion exactly once"),
		AfterEndTurn.Version, BeforeEndTurn.Version + 1);
	TestEqual(TEXT("Normal completion resets wait value"), AfterEndTurn.CurrentWaitValue, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnLifecyclePreEnemyBattleEndSpec,
	"Wacom.Battle.TurnLifecycle.OnDiscardCanEndBattleBeforeEnemyActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnLifecyclePreEnemyBattleEndSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* LethalDiscard = Fixture.MakeNoopCard(0);
	AddLethalOnDiscard(*LethalDiscard);
	TArray<UCardDefinition*> Deck = { LethalDiscard };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(0));
	}

	UCardDefinition* LeftHand = nullptr;
	UCardDefinition* RightHand = nullptr;
	UBattleSession* Session = CreateLifecycleSession(
		Fixture,
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 1),
		Deck,
		LeftHand,
		RightHand);
	const FGuid LeftHandId = FWacomBattleFixture::FindHandInstanceByCardId(
		Session->BuildSnapshot(), LeftHand->CardId);
	TestTrue(TEXT("Play left anchor"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(LeftHandId)).IsOk());
	const FBattleSnapshot BeforeEndTurn = Session->BuildSnapshot();
	Session->ConsumeEvents();
	Session->ConsumePresentationJournal();

	TestTrue(TEXT("EndTurn succeeds even when OnDiscard ends battle"),
		Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	const FBattlePresentationJournal Journal = Session->ConsumePresentationJournal();
	const FBattleSnapshot AfterEndTurn = Session->BuildSnapshot();
	TestTrue(TEXT("OnDiscard damage is published"),
		FindEventIndex(Events, EBattleEventType::DamageDealt) != INDEX_NONE);
	TestTrue(TEXT("First BattleEnd gate publishes BattleEnded"),
		FindEventIndex(Events, EBattleEventType::BattleEnded) != INDEX_NONE);
	TestEqual(TEXT("Enemy actions are skipped"), CountEvents(Events, EBattleEventType::EnemyPartActed), 0);
	TestEqual(TEXT("Next-turn draw is skipped"), CountEvents(Events, EBattleEventType::CardsDrawn), 0);
	TestNotNull(TEXT("Resolved discard checkpoint is preserved"),
		FindCheckpoint(Journal, EBattlePresentationCheckpointType::TurnEndDiscardResolved));
	TestNull(TEXT("Early BattleEnd has no draw checkpoint"),
		FindCheckpoint(Journal, EBattlePresentationCheckpointType::TurnStartDrawResolved));
	TestEqual(TEXT("Pre-enemy BattleEnd does not advance turn"), AfterEndTurn.TurnNumber, 1);
	TestEqual(TEXT("Pre-enemy BattleEnd phase"), AfterEndTurn.Phase, EBattlePhase::BattleEnd);
	TestEqual(TEXT("BattleEnd increments StateVersion exactly once"),
		AfterEndTurn.Version, BeforeEndTurn.Version + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnLifecyclePostEnemyBattleEndSpec,
	"Wacom.Battle.TurnLifecycle.EnemyActionCanEndBattleBeforeNextTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnLifecyclePostEnemyBattleEndSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Deck;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(0));
	}

	UCardDefinition* LeftHand = nullptr;
	UCardDefinition* RightHand = nullptr;
	UBattleSession* Session = CreateLifecycleSession(
		Fixture,
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 1000),
		Deck,
		LeftHand,
		RightHand);
	const FGuid LeftHandId = FWacomBattleFixture::FindHandInstanceByCardId(
		Session->BuildSnapshot(), LeftHand->CardId);
	TestTrue(TEXT("Play left anchor"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(LeftHandId)).IsOk());
	const FBattleSnapshot BeforeEndTurn = Session->BuildSnapshot();
	Session->ConsumeEvents();
	Session->ConsumePresentationJournal();

	TestTrue(TEXT("EndTurn succeeds when enemy action ends battle"),
		Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	const FBattlePresentationJournal Journal = Session->ConsumePresentationJournal();
	const FBattleSnapshot AfterEndTurn = Session->BuildSnapshot();
	const int32 EnemyActedIndex = FindEventIndex(Events, EBattleEventType::EnemyPartActed);
	const int32 BattleEndedIndex = FindEventIndex(Events, EBattleEventType::BattleEnded);
	TestTrue(TEXT("Enemy action precedes second BattleEnd gate"),
		EnemyActedIndex != INDEX_NONE && EnemyActedIndex < BattleEndedIndex);
	TestEqual(TEXT("Post-enemy BattleEnd skips next-turn draw"),
		CountEvents(Events, EBattleEventType::CardsDrawn), 0);
	TestNull(TEXT("Post-enemy BattleEnd has no draw checkpoint"),
		FindCheckpoint(Journal, EBattlePresentationCheckpointType::TurnStartDrawResolved));
	TestEqual(TEXT("Post-enemy BattleEnd does not advance turn"), AfterEndTurn.TurnNumber, 1);
	TestEqual(TEXT("Post-enemy BattleEnd phase"), AfterEndTurn.Phase, EBattlePhase::BattleEnd);
	TestEqual(TEXT("Post-enemy BattleEnd increments StateVersion exactly once"),
		AfterEndTurn.Version, BeforeEndTurn.Version + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnLifecycleReservedTriggersSpec,
	"Wacom.Battle.TurnLifecycle.ReservedTriggersRemainInactive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnLifecycleReservedTriggersSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* ReservedCard = Fixture.MakeNoopCard(0);
	AddShieldPassive(*ReservedCard, WacomTags::Passive_Trigger_OnDraw, 3);
	AddShieldPassive(*ReservedCard, WacomTags::Passive_Trigger_OnTurnEnd, 5);
	AddShieldPassive(*ReservedCard, WacomTags::Passive_Trigger_OnTurnStart, 7);
	TArray<UCardDefinition*> Deck = { ReservedCard };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Deck.Add(Fixture.MakeNoopCard(0));
	}

	UCardDefinition* LeftHand = nullptr;
	UCardDefinition* RightHand = nullptr;
	UBattleSession* Session = CreateLifecycleSession(
		Fixture,
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 0),
		Deck,
		LeftHand,
		RightHand);
	TestEqual(TEXT("Reserved OnDraw/OnTurnStart do not apply shield during initialization"),
		Session->BuildSnapshot().Player.Shield, 0);

	const FGuid LeftHandId = FWacomBattleFixture::FindHandInstanceByCardId(
		Session->BuildSnapshot(), LeftHand->CardId);
	TestTrue(TEXT("Play left anchor"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(LeftHandId)).IsOk());
	Session->ConsumeEvents();
	TestTrue(TEXT("EndTurn succeeds"),
		Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	TestEqual(TEXT("Reserved turn triggers do not apply shield"),
		Session->BuildSnapshot().Player.Shield, 0);
	TestEqual(TEXT("Reserved turn triggers do not publish PassiveTriggered"),
		CountEvents(Events, EBattleEventType::PassiveTriggered), 0);
	TestFalse(TEXT("Reserved turn triggers do not publish shield StatusApplied"),
		Events.ContainsByPredicate(
			[](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::StatusApplied
					&& Event.Tag == WacomTags::Status_Shield;
			}));
	return true;
}
