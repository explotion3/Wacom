// Copyright Wacom. All Rights Reserved.

#include "Events/BattleEventHelpers.h"

#include "Events/BattleEventBus.h"

namespace WacomBattleEvents
{
	void EmitCardsDrawn(
		FBattleEventBus& Events,
		const TArray<FGuid>& DrawnCardIds)
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
