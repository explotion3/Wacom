// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace WacomBattleEnemyUILayerPolicy
{
	/**
	 * Scene-projected compact panels must sit above the full-screen BattleHUD so their
	 * explicit inspection hotspot can receive Slate input. When interaction is gated
	 * off the panel content is HitTestInvisible, allowing target selection to continue
	 * through to BattleHUD's world-target route.
	 */
	inline const FName CompactPanelSharedLayerName(TEXT("WacomBattleEnemyPanelScreenLayer"));
	constexpr int32 CompactPanelZOrder = 8000;

	/** The non-modal two-sided inspection surface remains above compact panels. */
	constexpr int32 InspectionPanelZOrder = 8500;

	static_assert(CompactPanelZOrder < InspectionPanelZOrder);
}
