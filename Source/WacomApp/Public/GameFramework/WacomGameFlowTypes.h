// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomGameFlowTypes.generated.h"

/**
 * 战斗外的游戏流程状态。
 *
 * - Exploration：玩家在场景中自由移动。IMC_Exploration 激活，可触发战斗。
 * - Battle     ：战斗进行中。IMC_Battle 激活，移动禁用，战斗 UI 在 Game 层。
 * - JourneySummary：Journey 成功后的只读总结与主菜单交接阶段。
 *
 * GameMode 是状态机的唯一拥有者；其他系统只读 State 或通过 EnterBattle/ExitBattle 请求切换。
 */
UENUM(BlueprintType)
enum class EGameFlowState : uint8
{
	Exploration UMETA(DisplayName = "Exploration"),
	Battle      UMETA(DisplayName = "Battle"),
	JourneySummary UMETA(DisplayName = "Journey Summary"),
};
