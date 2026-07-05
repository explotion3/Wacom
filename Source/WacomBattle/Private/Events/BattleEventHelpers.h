// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

struct FBattleEventBus;

namespace WacomBattleEvents
{
	void EmitCardsDrawn(
		FBattleEventBus& Events,
		const TArray<FGuid>& DrawnCardIds);

	void EmitCardsRetained(
		FBattleEventBus& Events,
		const TArray<FGuid>& RetainedCardIds);

	void EmitHandLimitDiscardedEvents(
		FBattleEventBus& Events,
		const TArray<FGuid>& DiscardedCardIds,
		EHandLimitDiscardSource Source,
		const FGuid& SourceCardId = FGuid());
}
