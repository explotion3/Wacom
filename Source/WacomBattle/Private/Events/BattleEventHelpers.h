// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

struct FBattleEventBus;
struct FDeckDrawResult;

namespace WacomBattleEvents
{
	void EmitCardsDrawn(
		FBattleEventBus& Events,
		const TArray<FGuid>& DrawnCardIds,
		int32 DrawPileCountAfter = INDEX_NONE,
		int32 DiscardPileCountAfter = INDEX_NONE);

	void EmitDeckDrawResult(
		FBattleEventBus& Events,
		const FDeckDrawResult& Result);

	void EmitCardsRetained(
		FBattleEventBus& Events,
		const TArray<FGuid>& RetainedCardIds);

	void EmitHandLimitDiscardedEvents(
		FBattleEventBus& Events,
		const TArray<FGuid>& DiscardedCardIds,
		EHandLimitDiscardSource Source,
		const FGuid& SourceCardId = FGuid());
}
