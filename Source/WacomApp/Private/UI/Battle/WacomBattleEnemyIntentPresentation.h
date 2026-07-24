// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationTypes.h"

struct FWacomBattleEnemyPartEntryViewData;
class UWacomBattleEnemyIntentPresentationStyle;

/** Localizes authoritative Battle Intent facts without reading Behavior assets. */
struct WACOMAPP_API FWacomBattleIntentPresentationBuilder final
{
	static FWacomBattleIntentPresentationViewData Build(
		const FWacomBattleEnemyPartEntryViewData& PartView,
		const UWacomBattleEnemyIntentPresentationStyle* Style,
		int32 MaximumVisibleRows = 0);
};
