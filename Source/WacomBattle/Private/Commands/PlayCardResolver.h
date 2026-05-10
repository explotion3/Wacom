// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleCommand;
struct FBattleEventBus;

/**
 * 打牌命令解析。
 *
 * S2 骨架：仅检查最基本的入参，不改变 BattleState。
 * 完整流程在 S5 按 Battle_Rules.md §5 实现。
 */
class FPlayCardResolver
{
public:
	static FWacomStatus Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command);
};
