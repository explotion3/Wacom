// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Exploration/RunFloorMapSnapshot.h"
#include "UI/Map/WacomRunMapScreenTypes.h"

/** 将 WacomRun 地图事实映射为可丢弃的被动 Screen ViewData。 */
class FWacomRunMapViewDataBuilder
{
public:
	static FWacomRunMapScreenViewData Build(
		const FRunFloorMapSnapshot& Snapshot,
		TOptional<FWacomMapNodeHandle> RequestedSelection = {},
		bool bPreferRecommendedTarget = false,
		const FText& StatusOverride = FText::GetEmpty());
};
