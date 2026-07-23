// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace WacomBattleViewportLayerPolicy
{
	/**
	 * Scene-projected compact panels must sit above the full-screen BattleHUD so their
	 * explicit inspection hotspot can receive Slate input. When interaction is gated
	 * off the panel content is HitTestInvisible, allowing target selection to continue
	 * through to BattleHUD's world-target route.
	 */
	inline const FName CompactPanelSharedLayerName(TEXT("WacomBattleEnemyPanelScreenLayer"));
	constexpr int32 CompactPanelZOrder = 8000;

	/** Minimum viewport depth for the non-modal two-sided inspection surface. */
	constexpr int32 InspectionPanelZOrder = 8500;

	/** Minimum viewport depth for full-screen Battle secondary panels. */
	constexpr int32 SecondaryPanelZOrder = 8501;

	inline int32 AddViewportZOrderOffsetSaturated(
		const int32 BaseZOrder,
		const int32 Offset)
	{
		if (Offset <= 0)
		{
			return BaseZOrder;
		}
		return BaseZOrder > MAX_int32 - Offset
			? MAX_int32
			: BaseZOrder + Offset;
	}

	/** Enemy inspection renders one viewport step above the active first-person hand. */
	inline int32 ResolveInspectionPanelZOrder(const int32 CardLayerZOrder)
	{
		return FMath::Max(
			InspectionPanelZOrder,
			AddViewportZOrderOffsetSaturated(CardLayerZOrder, 1));
	}

	/** Full-screen secondary panels remain above both the hand and enemy inspection. */
	inline int32 ResolveSecondaryPanelZOrder(const int32 CardLayerZOrder)
	{
		return FMath::Max(
			SecondaryPanelZOrder,
			AddViewportZOrderOffsetSaturated(CardLayerZOrder, 2));
	}

	static_assert(CompactPanelZOrder < InspectionPanelZOrder);
	static_assert(InspectionPanelZOrder < SecondaryPanelZOrder);
}
