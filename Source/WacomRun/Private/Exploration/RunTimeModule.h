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
	/**
	 * 扣除行动点但在归零时保留当前阶段。调用方必须在其拥有的长生命周期
	 * activity 结束后显式调用 AdvanceToNextPhase。
	 */
	static FWacomStatus TrySpendActionPointsDeferredAdvance(
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
	static FWacomStatus TrySpendActionPointsInternal(
		FRunState& State,
		int32 Cost,
		bool bAdvanceWhenDepleted,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus AdvanceWorkingState(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus EnterNewMorning(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);
};
