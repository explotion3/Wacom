// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomResult.h"

struct FBattleState;
struct FBattleCommand;
struct FBattleEventBus;

/**
 * 等待命令解析。
 *
 * 等待会按当前等待值推进敌方先机，触发先机归零部位行动，然后递增等待值。
 */
class FWaitResolver
{
public:
	static FWacomStatus Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command);
};
