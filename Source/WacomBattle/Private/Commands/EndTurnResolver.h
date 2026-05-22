// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleCommand;
struct FBattleEventBus;

/** 结束回合命令解析，执行回合结束弃牌、敌方行动和下一回合起始流程。 */
class FEndTurnResolver
{
public:
	static FWacomStatus Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command);
};
