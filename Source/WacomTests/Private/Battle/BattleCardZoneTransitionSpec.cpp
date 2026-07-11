// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	bool IsCardInHand(const FBattleSnapshot& Snapshot, const FGuid& CardInstanceId)
	{
		return Snapshot.Hand.Cards.ContainsByPredicate(
			[&CardInstanceId](const FHandCardSnapshot& Card)
			{
				return Card.InstanceId == CardInstanceId;
			});
	}

	void AddOnDiscardPlayerPoison(UCardDefinition& Card)
	{
		FCardEffect PoisonEffect;
		PoisonEffect.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		PoisonEffect.Magnitude = 1;
		PoisonEffect.Target = WacomTags::Target_Player;

		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnDiscard;
		Passive.Effects.Add(PoisonEffect);
		Card.Passives.Add(Passive);
	}

	FName FindInitialCardId(const FBattleSnapshot& Snapshot, const FGuid& CardInstanceId)
	{
		const FHandCardSnapshot* Card = Snapshot.Hand.Cards.FindByPredicate(
			[&CardInstanceId](const FHandCardSnapshot& Candidate)
			{
				return Candidate.InstanceId == CardInstanceId;
			});
		return Card && Card->Definition ? Card->Definition->CardId : NAME_None;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRandomDiscardBatchTransitionSpec,
	"Wacom.Battle.CardZoneTransition.RandomDiscardBatchPublishesOnlyMovedCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRandomDiscardBatchTransitionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* DiscardSource = Fixture.MakeRandomDiscardCard(/*Cost*/0, /*Count*/2);
	DiscardSource->CardId = TEXT("CardZoneTransition.DiscardSource");
	AddOnDiscardPlayerPoison(*DiscardSource);
	TArray<UCardDefinition*> Deck = { DiscardSource };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UCardDefinition* Filler = Fixture.MakeNoopCard(0);
		Filler->CardId = FName(*FString::Printf(TEXT("CardZoneTransition.Filler%d"), Index));
		AddOnDiscardPlayerPoison(*Filler);
		Deck.Add(Filler);
	}

	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		/*Seed*/19);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Before, DiscardSource->CardId);
	if (!TestTrue(TEXT("Random discard source exists"), SourceId.IsValid()))
	{
		return false;
	}

	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakePlayCard(SourceId));
	if (!TestTrue(TEXT("Submit two-card random discard"), Resolution.IsOk()))
	{
		return false;
	}

	const FBattleSnapshot After = Session->BuildSnapshot();
	const TArray<FBattleEvent>& Events = Resolution.Events;
	TArray<const FBattleEvent*> DiscardEvents;
	TArray<const FBattleEvent*> HandZoneEvents;
	TArray<const FBattleEvent*> OnDiscardStatusEvents;
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type == EBattleEventType::CardDiscarded)
		{
			DiscardEvents.Add(&Event);
		}
		else if (Event.Type == EBattleEventType::HandZoneChanged)
		{
			HandZoneEvents.Add(&Event);
		}
		else if (Event.Type == EBattleEventType::StatusApplied && Event.Tag == WacomTags::Status_Poison)
		{
			OnDiscardStatusEvents.Add(&Event);
		}
		else if (Event.Type == EBattleEventType::CardExhausted)
		{
			AddError(TEXT("Random discard must not publish CardExhausted"));
		}
	}

	TestEqual(TEXT("Two successful moves publish two CardDiscarded events"), DiscardEvents.Num(), 2);
	TestEqual(TEXT("The discard batch publishes one HandZoneChanged event"), HandZoneEvents.Num(), 1);
	TestEqual(TEXT("Each discarded card runs its OnDiscard passive"), OnDiscardStatusEvents.Num(), 2);
	TestEqual(
		TEXT("Discard pile receives exactly the successfully moved cards"),
		After.PileCounts.DiscardCount - Before.PileCounts.DiscardCount,
		2);

	if (HandZoneEvents.Num() == 1)
	{
		const FBattleEvent& HandZoneEvent = *HandZoneEvents[0];
		TestEqual(TEXT("HandZoneChanged reports the batch size"), HandZoneEvent.Count, 2);
		TestFalse(TEXT("A multi-card HandZoneChanged has no single card id"), HandZoneEvent.CardInstanceId.IsValid());

		TSet<FGuid> PublishedCardIds;
		TArray<FName> PublishedDefinitionIds;
		for (const FBattleEvent* DiscardEvent : DiscardEvents)
		{
			TestTrue(TEXT("CardDiscarded identifies a moved card"), DiscardEvent->CardInstanceId.IsValid());
			TestFalse(TEXT("A published discarded card is no longer in hand"),
				IsCardInHand(After, DiscardEvent->CardInstanceId));
			TestEqual(TEXT("Discard event source is the resolving card"), DiscardEvent->ActorInstanceId, SourceId);
			TestTrue(TEXT("Discard event keeps the effect tag"), DiscardEvent->Tag == WacomTags::Effect_Discard);
			TestTrue(TEXT("CardDiscarded precedes the batch HandZoneChanged"),
				DiscardEvent->Sequence < HandZoneEvent.Sequence);
			PublishedCardIds.Add(DiscardEvent->CardInstanceId);
			PublishedDefinitionIds.Add(FindInitialCardId(Before, DiscardEvent->CardInstanceId));
		}
		TestEqual(TEXT("Each moved card is published once"), PublishedCardIds.Num(), 2);
		if (PublishedDefinitionIds.Num() == 2)
		{
			TestEqual(TEXT("Fixed seed keeps the first discarded card"),
				PublishedDefinitionIds[0], FName(TEXT("CardZoneTransition.Filler1")));
			TestEqual(TEXT("Fixed seed keeps the second discarded card"),
				PublishedDefinitionIds[1], FName(TEXT("CardZoneTransition.Filler0")));
		}

		if (DiscardEvents.Num() == 2 && OnDiscardStatusEvents.Num() == 2)
		{
			TestTrue(TEXT("First CardDiscarded precedes its OnDiscard event"),
				DiscardEvents[0]->Sequence < OnDiscardStatusEvents[0]->Sequence);
			TestTrue(TEXT("First OnDiscard completes before the second CardDiscarded"),
				OnDiscardStatusEvents[0]->Sequence < DiscardEvents[1]->Sequence);
			TestTrue(TEXT("Second CardDiscarded precedes its OnDiscard event"),
				DiscardEvents[1]->Sequence < OnDiscardStatusEvents[1]->Sequence);
			TestTrue(TEXT("All OnDiscard events precede the batch HandZoneChanged"),
				OnDiscardStatusEvents[1]->Sequence < HandZoneEvent.Sequence);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleTurnEndHandTransitionOrderSpec,
	"Wacom.Battle.CardZoneTransition.TurnEndUsesStableRetainAndDiscardFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleTurnEndHandTransitionOrderSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
	UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
	UCardDefinition* PlayedCard = Fixture.MakeNoopCard(0);
	PlayedCard->CardId = TEXT("CardZoneTransition.TurnEnd.Played");
	AddOnDiscardPlayerPoison(*PlayedCard);

	UCardDefinition* FirstDiscard = Fixture.MakeNoopCard(0);
	FirstDiscard->CardId = TEXT("CardZoneTransition.TurnEnd.FirstDiscard");
	AddOnDiscardPlayerPoison(*FirstDiscard);
	UCardDefinition* SecondDiscard = Fixture.MakeNoopCard(0);
	SecondDiscard->CardId = TEXT("CardZoneTransition.TurnEnd.SecondDiscard");
	AddOnDiscardPlayerPoison(*SecondDiscard);
	UCardDefinition* RetainedCard = Fixture.MakeNoopCard(0);
	RetainedCard->CardId = TEXT("CardZoneTransition.TurnEnd.Retained");
	RetainedCard->Keywords.AddTag(WacomTags::Card_Keyword_Retain);
	UCardDefinition* PlainDiscard = Fixture.MakeNoopCard(0);
	PlainDiscard->CardId = TEXT("CardZoneTransition.TurnEnd.PlainDiscard");

	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			LeftHand,
			RightHand,
			{ PlayedCard, FirstDiscard, SecondDiscard, RetainedCard, PlainDiscard }),
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/100,
			/*Initiative*/50,
			/*IntentResist*/0,
			/*Damage*/0),
		/*Seed*/7);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid PlayedCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PlayedCard->CardId);
	const FGuid LeftHandId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftHand->CardId);
	const FGuid RightHandId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, RightHand->CardId);
	const FGuid RetainedCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, RetainedCard->CardId);
	if (!TestTrue(TEXT("Played card starts in hand"), PlayedCardId.IsValid())
		|| !TestTrue(TEXT("Left anchor starts in hand"), LeftHandId.IsValid())
		|| !TestTrue(TEXT("Right anchor starts in hand"), RightHandId.IsValid())
		|| !TestTrue(TEXT("Retained card starts in hand"), RetainedCardId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Move normal card to PlayedPile"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(PlayedCardId)).IsOk());
	TestTrue(TEXT("Remove one anchor so Both-zone does not retain plain cards"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(LeftHandId)).IsOk());

	const FBattleSnapshot BeforeEndTurn = Session->BuildSnapshot();
	TArray<FGuid> ExpectedDiscardOrder;
	for (int32 HandIndex = BeforeEndTurn.Hand.Cards.Num() - 1; HandIndex >= 0; --HandIndex)
	{
		const FHandCardSnapshot& Card = BeforeEndTurn.Hand.Cards[HandIndex];
		if (!Card.bIsHandAnchor && Card.InstanceId != RetainedCardId)
		{
			ExpectedDiscardOrder.Add(Card.InstanceId);
		}
	}

	const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("EndTurn succeeds"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;
	TArray<const FBattleEvent*> TurnEndDiscardEvents;
	const FBattleEvent* RetainedEvent = nullptr;
	const FBattleEvent* DiscardBatchEvent = nullptr;
	int32 OnDiscardPoisonEvents = 0;
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type == EBattleEventType::CardDiscarded
			&& Event.HandCardZoneMoveReason == EHandCardZoneMoveReason::TurnEnd)
		{
			TurnEndDiscardEvents.Add(&Event);
		}
		else if (Event.Type == EBattleEventType::CardsRetained)
		{
			RetainedEvent = &Event;
		}
		else if (Event.Type == EBattleEventType::HandZoneChanged
			&& Event.Count == ExpectedDiscardOrder.Num())
		{
			DiscardBatchEvent = &Event;
		}
		else if (Event.Type == EBattleEventType::StatusApplied
			&& Event.Tag == WacomTags::Status_Poison)
		{
			++OnDiscardPoisonEvents;
		}
	}

	TestEqual(TEXT("Only non-retained hand cards publish turn-end discard facts"),
		TurnEndDiscardEvents.Num(), ExpectedDiscardOrder.Num());
	for (int32 Index = 0; Index < FMath::Min(TurnEndDiscardEvents.Num(), ExpectedDiscardOrder.Num()); ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("Turn-end discard order[%d]"), Index),
			TurnEndDiscardEvents[Index]->CardInstanceId,
			ExpectedDiscardOrder[Index]);
		TestFalse(TEXT("Turn-end discard has no effect source actor"),
			TurnEndDiscardEvents[Index]->ActorInstanceId.IsValid());
	}
	TestNotNull(TEXT("Turn-end discard publishes one sized batch event"), DiscardBatchEvent);
	if (DiscardBatchEvent && !TurnEndDiscardEvents.IsEmpty())
	{
		TestTrue(TEXT("All CardDiscarded events precede the batch HandZoneChanged"),
			TurnEndDiscardEvents.Last()->Sequence < DiscardBatchEvent->Sequence);
	}
	TestEqual(TEXT("Only the two discarded passive cards run OnDiscard"), OnDiscardPoisonEvents, 2);
	TestTrue(TEXT("PlayedPile natural cleanup does not publish CardDiscarded"),
		!TurnEndDiscardEvents.ContainsByPredicate(
			[PlayedCardId](const FBattleEvent* Event)
			{
				return Event && Event->CardInstanceId == PlayedCardId;
			}));
	if (TestNotNull(TEXT("Explicit retained card publishes CardsRetained"), RetainedEvent))
	{
		TestTrue(TEXT("CardsRetained contains retained normal card"),
			RetainedEvent->CardInstanceIds.Contains(RetainedCardId));
		TestFalse(TEXT("CardsRetained excludes hand anchors"),
			RetainedEvent->CardInstanceIds.Contains(RightHandId));
	}

	return true;
}
