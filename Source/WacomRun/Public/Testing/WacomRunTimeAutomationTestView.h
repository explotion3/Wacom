// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Exploration/RunExplorationTypes.h"
#include "RunState.h"
#include "Types/WacomResult.h"

/** Narrow non-reflected automation seam for private Run time/exposure modules. */
struct WACOMRUN_API FWacomRunTimeAutomationTestView
{
	static FWacomStatus TrySpendActionPoints(
		FRunState& State,
		int32 Cost,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus AdvanceToNextPhase(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus ChooseNightExploration(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus CompleteCampAndAdvanceToMorning(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus ApplyDailyDecay(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
};

#endif
