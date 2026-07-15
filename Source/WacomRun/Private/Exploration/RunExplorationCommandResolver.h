// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationCommand.h"
#include "Exploration/RunExplorationResolution.h"

struct FRunState;

/** 探索命令的唯一 private resolver；调用方负责 working-state commit。 */
class FRunExplorationCommandResolver
{
public:
	static FRunExplorationResolution Resolve(
		FRunState& State,
		TOptional<FRunTraversalTicket>& PendingTraversal,
		TOptional<FRunCampTicket>& PendingCamp,
		TOptional<FRunFloorTransitionConfirmation>& PendingFloorTransition,
		const FRunExplorationCommand& Command);
};
