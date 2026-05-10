// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleCommand;
struct FBattleEventBus;

/**
 * 结束回合命令解析。对齐 Battle_Rules.md §7 / §12。
 * S2 骨架：只把阶段切到 TurnEnd。具体 TurnEnd 流程在 S5/S6 补完。
 */
class FEndTurnResolver
{
public:
	static FWacomStatus Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command);
};
