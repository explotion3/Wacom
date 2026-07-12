// Copyright Wacom. All Rights Reserved.

#include "Events/BattleEventHelpers.h"

#include "Events/BattleEventBus.h"
#include "Deck/DeckService.h"

namespace WacomBattleEvents
{
	void EmitCardsDrawn(
		FBattleEventBus& Events,
		const TArray<FGuid>& DrawnCardIds,
		int32 DrawPileCountAfter,
		int32 DiscardPileCountAfter)
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::CardsDrawn;
		Ev.CardInstanceIds.Reserve(DrawnCardIds.Num());
		for (const FGuid& DrawnCardId : DrawnCardIds)
		{
			if (DrawnCardId.IsValid())
			{
				Ev.CardInstanceIds.Add(DrawnCardId);
			}
		}
		Ev.Count = Ev.CardInstanceIds.Num();
		Ev.DrawPileCountAfter = DrawPileCountAfter;
		Ev.DiscardPileCountAfter = DiscardPileCountAfter;
		Events.Emit(Ev);
	}

	void EmitDeckDrawResult(
		FBattleEventBus& Events,
		const FDeckDrawResult& Result)
	{
		for (const FDeckDrawStepFact& Step : Result.Steps)
		{
			if (Step.Kind == EDeckDrawStepKind::DrawBatch)
			{
				EmitCardsDrawn(
					Events,
					Step.CardInstanceIds,
					Step.DrawPileCountAfter,
					Step.DiscardPileCountAfter);
				continue;
			}

			FBattleEvent Event;
			Event.Type = EBattleEventType::DiscardPileReshuffledIntoDraw;
			Event.CardInstanceIds = Step.CardInstanceIds;
			Event.Count = Event.CardInstanceIds.Num();
			Event.DrawPileCountAfter = Step.DrawPileCountAfter;
			Event.DiscardPileCountAfter = Step.DiscardPileCountAfter;
			Events.Emit(MoveTemp(Event));
		}
	}

	void EmitCardsRetained(
		FBattleEventBus& Events,
		const TArray<FGuid>& RetainedCardIds)
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::CardsRetained;
		Ev.CardInstanceIds.Reserve(RetainedCardIds.Num());
		for (const FGuid& RetainedCardId : RetainedCardIds)
		{
			if (RetainedCardId.IsValid())
			{
				Ev.CardInstanceIds.Add(RetainedCardId);
			}
		}
		Ev.Count = Ev.CardInstanceIds.Num();
		Events.Emit(Ev);
	}

	void EmitHandLimitDiscardedEvents(
		FBattleEventBus& Events,
		const TArray<FGuid>& DiscardedCardIds,
		EHandLimitDiscardSource Source,
		const FGuid& SourceCardId)
	{
		for (const FGuid& DiscardedCardId : DiscardedCardIds)
		{
			if (!DiscardedCardId.IsValid())
			{
				continue;
			}

			FBattleEvent Ev;
			Ev.Type = EBattleEventType::HandLimitDiscarded;
			Ev.CardInstanceId = DiscardedCardId;
			Ev.ActorInstanceId = SourceCardId;
			Ev.HandLimitDiscardSource = Source;
			Events.Emit(Ev);
		}
	}
}
