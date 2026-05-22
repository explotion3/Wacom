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
 * 校验费用、目标和手牌状态后，执行卡牌效果、先机命中、抵抗、完美释放、
 * 卡牌去向、被动触发、击倒事件和战斗结束判定。
 */
class FPlayCardResolver
{
public:
	static FWacomStatus Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command);
};
