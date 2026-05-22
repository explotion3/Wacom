// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

struct FBattleEventBus;
struct FBattleState;

/**
 * 击倒事件请求流服务。
 *
 * 统一构造 KnockdownChoiceRequested 事件，保留旧 Count 位掩码兼容日志/UI 过渡路径。
 */
struct FKnockdownFlowService
{
	static bool RequestCurrentChoiceIfPending(FBattleState& State, FBattleEventBus& Events);
	static FBattleEvent BuildCurrentChoiceRequestedEvent(const FBattleState& State);
};
