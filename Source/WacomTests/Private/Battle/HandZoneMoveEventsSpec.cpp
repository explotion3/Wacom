// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	const FHandCardSnapshot* FindHandZoneMoveEventHandCard(const FBattleSnapshot& Snapshot, const FGuid& CardId)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.InstanceId == CardId)
			{
				return &Card;
			}
		}
		return nullptr;
	}

	int32 CountEvents(const TArray<FBattleEvent>& Events, EBattleEventType Type, const FGuid& CardId = FGuid())
	{
		int32 Count = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == Type && (!CardId.IsValid() || Event.CardInstanceId == CardId))
			{
				++Count;
			}
		}
		return Count;
	}

	const FBattleEvent* FindEvent(const TArray<FBattleEvent>& Events, EBattleEventType Type, const FGuid& CardId)
	{
		return Events.FindByPredicate(
			[Type, &CardId](const FBattleEvent& Event)
			{
				return Event.Type == Type && Event.CardInstanceId == CardId;
			});
	}

	UBattleSession* CreateSelectedMoveSession(
		FWacomBattleFixture& Fixture,
		bool bExhaust,
		UCardDefinition*& OutSourceCard,
		UCardDefinition*& OutTargetCard)
	{
		OutSourceCard = Fixture.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, bExhaust);
		OutTargetCard = Fixture.MakeOnDiscardShieldCard(/*Cost*/3, /*ShieldAmount*/7);

		TArray<UCardDefinition*> Deck = { OutSourceCard, OutTargetCard };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}

		return Fixture.CreateSession(
			Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
			Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
			1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSelectedDiscardEmitsCardDiscardedAndRunsOnDiscardSpec,
	"Wacom.Battle.HandZoneMoveEvents.SelectedDiscardEmitsCardDiscardedAndRunsOnDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSelectedDiscardEmitsCardDiscardedAndRunsOnDiscardSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateSelectedMoveSession(Fx, /*bExhaust*/false, SourceDef, TargetDef);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	TestTrue(TEXT("Source exists"), SourceId.IsValid());
	TestTrue(TEXT("Target exists"), TargetId.IsValid());

	Session->ConsumeEvents();
	TestTrue(TEXT("Submit selected discard"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());

	Snapshot = Session->BuildSnapshot();
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	TestFalse(TEXT("Target left hand"), FindHandZoneMoveEventHandCard(Snapshot, TargetId) != nullptr);
	TestEqual(TEXT("OnDiscard passive added shield"), Snapshot.Player.Shield, 7);
	if (const FBattleEvent* DiscardEvent = FindEvent(Events, EBattleEventType::CardDiscarded, TargetId))
	{
		TestEqual(TEXT("Discard reason is effect"),
			DiscardEvent->HandCardZoneMoveReason, EHandCardZoneMoveReason::Effect);
		TestEqual(TEXT("Discard source card"), DiscardEvent->ActorInstanceId, SourceId);
		TestTrue(TEXT("Discard effect tag"), DiscardEvent->Tag == WacomTags::Effect_Card_DiscardSelected);
	}
	else
	{
		AddError(TEXT("Missing CardDiscarded event for selected target"));
	}
	TestEqual(TEXT("Selected discard emits one HandZoneChanged for target move"),
		CountEvents(Events, EBattleEventType::HandZoneChanged, FGuid()), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRandomDiscardEmitsCardDiscardedAndRunsOnDiscardSpec,
	"Wacom.Battle.HandZoneMoveEvents.RandomDiscardEmitsCardDiscardedAndRunsOnDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRandomDiscardEmitsCardDiscardedAndRunsOnDiscardSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 80; ++Seed)
	{
		FWacomBattleFixture Fx;
		UCardDefinition* DiscardSource = Fx.MakeRandomDiscardCard(/*Cost*/0, /*Count*/1);
		UCardDefinition* Target = Fx.MakeOnDiscardShieldCard(/*Cost*/0, /*ShieldAmount*/5);
		TArray<UCardDefinition*> Deck = { DiscardSource, Target };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Deck.Add(Fx.MakeNoopCard(0));
		}

		UBattleSession* Session = Fx.CreateSession(
			Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Deck),
			Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
			Seed);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DiscardSource->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Target->CardId);
		if (!SourceId.IsValid() || !TargetId.IsValid())
		{
			continue;
		}

		Session->ConsumeEvents();
		TestTrue(TEXT("Submit random discard"), Session->SubmitCommand(FBattleCommand::MakePlayCard(SourceId)).IsOk());
		Snapshot = Session->BuildSnapshot();
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		if (FindEvent(Events, EBattleEventType::CardDiscarded, TargetId))
		{
			TestEqual(TEXT("Random discard target OnDiscard passive added shield"), Snapshot.Player.Shield, 5);
			return true;
		}
	}

	AddError(TEXT("No deterministic seed randomly discarded the OnDiscard target"));
	return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandLimitDiscardEmitsCardDiscardedAndLegacyHandLimitEventSpec,
	"Wacom.Battle.HandZoneMoveEvents.HandLimitDiscardEmitsCardDiscardedAndLegacyHandLimitEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandLimitDiscardEmitsCardDiscardedAndLegacyHandLimitEventSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = nullptr;
	FGuid SourceId;
	FGuid TargetId;

	for (int32 Seed = 1; Seed <= 80 && !Session; ++Seed)
	{
		SourceDef = nullptr;
		TargetDef = Fx.MakeOnDiscardShieldCard(/*Cost*/0, /*ShieldAmount*/3);
		UCardDefinition* DrawCard = Fx.MakeNoopCard(/*Cost*/0);
		SourceDef = DrawCard;
		FCardEffect DrawEffect;
		DrawEffect.EffectType = WacomTags::Effect_Draw;
		DrawEffect.Magnitude = 7;
		DrawEffect.TargetZone = WacomTags::CardLocation_Draw;
		DrawCard->Effects.Add(DrawEffect);

		TArray<UCardDefinition*> Deck = { DrawCard, TargetDef };
		for (int32 Index = 0; Index < 14; ++Index)
		{
			Deck.Add(Fx.MakeNoopCard(0));
		}

		UBattleSession* Candidate = Fx.CreateSession(
			Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Deck),
			Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/100, /*IntentResist*/0),
			Seed);
		const FBattleSnapshot Snapshot = Candidate->BuildSnapshot();
		SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
		TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
		if (SourceId.IsValid() && TargetId.IsValid())
		{
			Session = Candidate;
		}
	}

	TestTrue(TEXT("Found source in opening hand"), SourceId.IsValid());
	TestTrue(TEXT("Found target in opening hand"), TargetId.IsValid());
	if (!Session)
	{
		return false;
	}

	Session->ConsumeEvents();
	TestTrue(TEXT("Submit draw that enforces hand limit"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(SourceId)).IsOk());
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();

	int32 LimitDiscardEvents = 0;
	int32 CardDiscardedEvents = 0;
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type == EBattleEventType::HandLimitDiscarded)
		{
			++LimitDiscardEvents;
			TestEqual(TEXT("Legacy hand limit source"), Event.HandLimitDiscardSource, EHandLimitDiscardSource::EffectDraw);
			TestEqual(TEXT("Legacy hand limit actor"), Event.ActorInstanceId, SourceId);
		}
		if (Event.Type == EBattleEventType::CardDiscarded
			&& Event.HandCardZoneMoveReason == EHandCardZoneMoveReason::HandLimit)
		{
			++CardDiscardedEvents;
			TestEqual(TEXT("CardDiscarded hand limit source"), Event.HandLimitDiscardSource, EHandLimitDiscardSource::EffectDraw);
			TestEqual(TEXT("CardDiscarded actor"), Event.ActorInstanceId, SourceId);
		}
	}
	TestTrue(TEXT("Hand limit emits legacy event"), LimitDiscardEvents > 0);
	TestEqual(TEXT("Hand limit emits matching CardDiscarded events"), CardDiscardedEvents, LimitDiscardEvents);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnEndDiscardRunsOnDiscardWithoutHandLimitEventSpec,
	"Wacom.Battle.HandZoneMoveEvents.TurnEndDiscardRunsOnDiscardWithoutHandLimitEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnEndDiscardRunsOnDiscardWithoutHandLimitEventSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Left = Fx.MakeNoopCard(1);
	UCardDefinition* Right = Fx.MakeNoopCard(1);
	UCardDefinition* Target = Fx.MakeOnDiscardShieldCard(/*Cost*/0, /*ShieldAmount*/4);
	TArray<UCardDefinition*> Deck = { Target };
	for (int32 Index = 0; Index < 14; ++Index)
	{
		Deck.Add(Fx.MakeNoopCard(0));
	}

	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(Left, Right, Deck),
		Fx.MakeSinglePartEnemyWithIntentDamage(/*Hp*/500, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0),
		1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Left->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Target->CardId);
	TestTrue(TEXT("Left anchor exists"), LeftId.IsValid());
	TestTrue(TEXT("Target exists"), TargetId.IsValid());

	TestTrue(TEXT("Play left anchor so normal cards do not retain"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(LeftId)).IsOk());
	Session->ConsumeEvents();
	TestTrue(TEXT("End turn"), Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());

	Snapshot = Session->BuildSnapshot();
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	TestFalse(TEXT("Target left hand after turn-end discard"), FindHandZoneMoveEventHandCard(Snapshot, TargetId) != nullptr);
	TestEqual(TEXT("Turn-end OnDiscard passive added shield"), Snapshot.Player.Shield, 4);
	if (const FBattleEvent* DiscardEvent = FindEvent(Events, EBattleEventType::CardDiscarded, TargetId))
	{
		TestEqual(TEXT("Turn-end discard reason"),
			DiscardEvent->HandCardZoneMoveReason, EHandCardZoneMoveReason::TurnEnd);
	}
	else
	{
		AddError(TEXT("Missing CardDiscarded event for turn-end discard"));
	}
	TestEqual(TEXT("Turn-end discard does not emit HandLimitDiscarded"),
		CountEvents(Events, EBattleEventType::HandLimitDiscarded), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleExhaustSelectedEmitsCardExhaustedWithoutOnDiscardSpec,
	"Wacom.Battle.HandZoneMoveEvents.ExhaustSelectedEmitsCardExhaustedWithoutOnDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleExhaustSelectedEmitsCardExhaustedWithoutOnDiscardSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateSelectedMoveSession(Fx, /*bExhaust*/true, SourceDef, TargetDef);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetDef->CardId);
	Session->ConsumeEvents();

	TestTrue(TEXT("Submit selected exhaust"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();

	TestFalse(TEXT("Target left hand"), FindHandZoneMoveEventHandCard(Snapshot, TargetId) != nullptr);
	TestEqual(TEXT("OnDiscard passive did not run for exhaust"), Snapshot.Player.Shield, 0);
	TestEqual(TEXT("No CardDiscarded for exhaust target"),
		CountEvents(Events, EBattleEventType::CardDiscarded, TargetId), 0);
	if (const FBattleEvent* ExhaustEvent = FindEvent(Events, EBattleEventType::CardExhausted, TargetId))
	{
		TestEqual(TEXT("Exhaust reason is effect"),
			ExhaustEvent->HandCardZoneMoveReason, EHandCardZoneMoveReason::Effect);
		TestEqual(TEXT("Exhaust source card"), ExhaustEvent->ActorInstanceId, SourceId);
		TestTrue(TEXT("Exhaust effect tag"), ExhaustEvent->Tag == WacomTags::Effect_Card_ExhaustSelected);
	}
	else
	{
		AddError(TEXT("Missing CardExhausted event for selected target"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePlayedCardDiscardDestinationDoesNotRunOnDiscardSpec,
	"Wacom.Battle.HandZoneMoveEvents.PlayedCardDiscardDestinationDoesNotRunOnDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePlayedCardDiscardDestinationDoesNotRunOnDiscardSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* PlayedCard = Fx.MakeOnDiscardShieldCard(/*Cost*/0, /*ShieldAmount*/9);
	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { PlayedCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) }),
		Fx.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid PlayedId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PlayedCard->CardId);
	TestTrue(TEXT("Played card exists"), PlayedId.IsValid());
	Session->ConsumeEvents();

	TestTrue(TEXT("Play OnDiscard card"), Session->SubmitCommand(FBattleCommand::MakePlayCard(PlayedId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();

	TestFalse(TEXT("Played card left hand"), FindHandZoneMoveEventHandCard(Snapshot, PlayedId) != nullptr);
	TestEqual(TEXT("Played card destination does not trigger OnDiscard"), Snapshot.Player.Shield, 0);
	TestEqual(TEXT("Played card destination does not emit CardDiscarded"),
		CountEvents(Events, EBattleEventType::CardDiscarded, PlayedId), 0);
	TestEqual(TEXT("CardPlayed still emitted"),
		CountEvents(Events, EBattleEventType::CardPlayed, PlayedId), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleDiscardHooksRunAfterCardIsAlreadyInDiscardPileSpec,
	"Wacom.Battle.HandZoneMoveEvents.DiscardHooksRunAfterCardIsAlreadyInDiscardPile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleDiscardHooksRunAfterCardIsAlreadyInDiscardPileSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* SourceDef = nullptr;
	UCardDefinition* TargetDef = nullptr;
	UBattleSession* Session = CreateSelectedMoveSession(Fx, /*bExhaust*/false, SourceDef, TargetDef);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Before, SourceDef->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Before, TargetDef->CardId);
	const int32 DiscardBefore = Before.PileCounts.DiscardCount;
	Session->ConsumeEvents();

	TestTrue(TEXT("Submit selected discard"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();

	TestEqual(TEXT("Discard pile contains source and target"), After.PileCounts.DiscardCount, DiscardBefore + 2);
	TestEqual(TEXT("OnDiscard passive observed post-move state"), After.Player.Shield, 7);
	if (const FBattleEvent* DiscardEvent = FindEvent(Events, EBattleEventType::CardDiscarded, TargetId))
	{
		if (const FBattleEvent* HandZoneEvent = Events.FindByPredicate(
			[](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::HandZoneChanged;
			}))
		{
			TestTrue(TEXT("CardDiscarded is emitted before batch HandZoneChanged"),
				DiscardEvent->Sequence < HandZoneEvent->Sequence);
		}
		else
		{
			AddError(TEXT("Missing HandZoneChanged event"));
		}
	}
	else
	{
		AddError(TEXT("Missing CardDiscarded event"));
	}
	return true;
}
