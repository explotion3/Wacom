// Copyright Wacom. All Rights Reserved.

#include "Events/BattleEventHelpers.h"

#include "Events/BattleEventBus.h"

namespace WacomBattleEvents
{
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
