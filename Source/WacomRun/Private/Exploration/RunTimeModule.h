// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FRunExplorationEvent;
struct FRunState;

/** Private owner of Action Point spending, phase gates, and phase-entry effects. */
class FRunTimeModule
{
public:
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

private:
	static FWacomStatus AdvanceWorkingState(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus EnterNewMorning(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
};
