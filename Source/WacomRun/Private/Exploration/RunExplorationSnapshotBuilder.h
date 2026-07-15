// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"

struct FRunState;

/** 将探索 working state 投影为 UI/App 只读事实，不持有 Session 输出。 */
class FRunExplorationSnapshotBuilder
{
public:
	static FRunExplorationSnapshot Build(const FRunState& State);
};
