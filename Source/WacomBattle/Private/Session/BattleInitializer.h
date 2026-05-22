// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

class UObject;
struct FBattleEventBus;
struct FBattleInitParams;
struct FBattleState;

/**
 * BattleSession 的私有初始化规则中心。
 *
 * UBattleSession::Initialize 负责 public 参数 guard 与生命周期容器重置；
 * 本类型负责把合法入参灌入一场新的 FBattleState，并发出首批战斗事件。
 */
struct FBattleInitializer
{
	static FWacomStatus Initialize(
		FBattleState& State,
		FBattleEventBus& EventBus,
		const FBattleInitParams& Params,
		TArray<TObjectPtr<const UObject>>& ReferencedAssets);
};
