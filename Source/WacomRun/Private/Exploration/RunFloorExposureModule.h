// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FRunExplorationEvent;
struct FRunState;

/** Applies the Journey/Floor daily pressure contract exactly once per new Morning. */
class FRunFloorExposureModule
{
public:
	static FWacomStatus ApplyDailyDecay(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
};
