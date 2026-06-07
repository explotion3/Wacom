// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomEnums.generated.h"

/**
 * 战斗阶段。
 *
 * `Setup` 代表敌人初始化阶段，战斗开始后唯一一次。
 */
UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
	None        UMETA(DisplayName = "None"),
	Setup       UMETA(DisplayName = "Setup"),       // 敌人初始化
	TurnStart   UMETA(DisplayName = "TurnStart"),   // 起始阶段
	PlayerAction UMETA(DisplayName = "PlayerAction"),// 执行阶段
	TurnEnd     UMETA(DisplayName = "TurnEnd"),     // 结束阶段
	PendingKnockdownChoice UMETA(DisplayName = "PendingKnockdownChoice"), // 等玩家三选一
	BattleEnd   UMETA(DisplayName = "BattleEnd"),   // 战斗结束
};

/**
 * 击倒事件玩家选择。
 *
 * 部位被击倒时玩家三选一：
 *   - Aid（援助）：左手分支，不消耗左手牌
 *   - Destroy（破坏）：右手分支，不消耗右手牌
 *   - Withdraw（撤离）：直接结束战斗。Outcome 设 Victory，不销毁触发战斗的场景 Trigger
 *
 * 具体奖励、撤离与战后包规则见 Docs/WacomBattle.md / Docs/WacomRun.md。
 */
UENUM(BlueprintType)
enum class EKnockdownChoice : uint8
{
	None      UMETA(DisplayName = "None"),
	Aid       UMETA(DisplayName = "援助"),
	Destroy   UMETA(DisplayName = "破坏"),
	Withdraw  UMETA(DisplayName = "撤离"),
};

/**
 * 卡牌目标模式。
 *
 * HandCard 目标当前用于腾挪类效果，不用于主动打牌合法性。
 */
UENUM(BlueprintType)
enum class ECardTargetMode : uint8
{
	None            UMETA(DisplayName = "None"),             // 无目标（打出即生效）
	Self            UMETA(DisplayName = "Self"),             // 作用于玩家自身
	SingleEnemyPart UMETA(DisplayName = "SingleEnemyPart"),  // 单个敌方部位
	AllEnemyParts   UMETA(DisplayName = "AllEnemyParts"),    // 所有存活敌方部位
	HandCard        UMETA(DisplayName = "HandCard"),         // 作用于手牌中的其它卡
};

/**
 * 手牌区域。
 *
 * None 表示卡牌不在手牌队列中，或左右手锚点缺失导致该区域不存在。
 */
UENUM(BlueprintType)
enum class EHandZone : uint8
{
	None  UMETA(DisplayName = "None"),
	Left  UMETA(DisplayName = "Left"),
	Both  UMETA(DisplayName = "Both"),
	Right UMETA(DisplayName = "Right"),
};

/**
 * 战斗结果。BattleEnd 阶段时填充。
 */
UENUM(BlueprintType)
enum class EBattleOutcome : uint8
{
	Undetermined UMETA(DisplayName = "Undetermined"),
	Victory      UMETA(DisplayName = "Victory"),
	Defeat       UMETA(DisplayName = "Defeat"),
};

/**
 * 卡牌在战斗中的位置容器。
 *
 * `Limbo` 用于左手牌/右手牌被打出、但本回合不进入任何卡牌区域的过渡状态。
 */
UENUM(BlueprintType)
enum class ECardLocation : uint8
{
	Unknown  UMETA(DisplayName = "Unknown"),
	Draw     UMETA(DisplayName = "Draw"),      // 抽牌堆
	Hand     UMETA(DisplayName = "Hand"),      // 手牌队列
	Played   UMETA(DisplayName = "Played"),    // 本回合使用牌堆
	Discard  UMETA(DisplayName = "Discard"),   // 弃牌堆
	Exhaust  UMETA(DisplayName = "Exhaust"),   // 消耗牌堆
	Limbo    UMETA(DisplayName = "Limbo"),     // 左右手本回合离开手牌但不入任何区域
};
