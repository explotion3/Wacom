// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Exploration/RunFloorMapSnapshot.h"

struct FRunState;

/** 将当前 Floor working state 投影为低频、完整且不含场景对象的地图事实。 */
class FRunFloorMapSnapshotBuilder
{
public:
	static FRunFloorMapSnapshot Build(const FRunState& State);
};
