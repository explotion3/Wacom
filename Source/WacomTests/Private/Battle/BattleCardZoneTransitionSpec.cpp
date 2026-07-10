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

	Session->ConsumeEvents();
	if (!TestTrue(
		TEXT("Submit two-card random discard"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(SourceId)).IsOk()))
	{
		return false;
	}

	const FBattleSnapshot After = Session->BuildSnapshot();
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
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
