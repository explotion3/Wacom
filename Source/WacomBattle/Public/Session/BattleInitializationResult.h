// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomResult.h"

/**
 * 一次战斗初始化的原子输出。
 *
 * 成功时 Events 与 PostSnapshot 来自同一次新战斗 commit；失败时 Events 为空，
 * PostSnapshot 是 Session 当前仍然有效的旧战斗快照。
 */
struct WACOMBATTLE_API FBattleInitializationResult
{
	FWacomStatus Status;
	TArray<FBattleEvent> Events;
	FBattleSnapshot PostSnapshot;

	bool IsOk() const { return Status.IsOk(); }
};
