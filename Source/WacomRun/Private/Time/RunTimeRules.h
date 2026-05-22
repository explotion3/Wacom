// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

struct FRunState;

/**
 * Run 时间 / 节点推进规则的私有 helper。
 *
 * 只操作 FRunState；不依赖 UObject、不广播、不访问 UI。
 * URunSession / RunEventExecutor 等外层入口负责通知与结果包装。
 */
struct FRunTimeRules
{
	static bool ConsumeNode(FRunState& State, int32 Count, int32* OutConsumedNodeCount = nullptr);
	static void AdvanceToNextPhase(FRunState& State);
	static void ResetRemainingNodeForPhase(FRunState& State);

private:
	static void ApplyPhaseEntryEffects(FRunState& State, ETimePhase NewPhase, ETimePhase PrevPhase);
};
