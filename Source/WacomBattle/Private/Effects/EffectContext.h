// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FBattleState;
struct FBattleEventBus;

/**
 * 效果来源类型。决定效果计算口径（例如"卡牌的 Runtime Cost 影响伤害"）。
 */
enum class EEffectSourceKind : uint8
{
	None,
	Card,          // 玩家打出的一张卡
	EnemyPartIntent,// 敌方部位执行意图
	System,        // 状态回合结算、被动触发等系统调度
};

/**
 * 效果目标类型。
 */
enum class EEffectTargetKind : uint8
{
	None,
	Player,
	EnemyPart,
	HandCard,       // 腾挪类效果会作用于另一张手牌
};

/**
 * 一次效果执行的上下文。
 *
 * Source / Target 用 FGuid 或特殊标记（FGuid 无效 + Kind = Player）指定。
 * EffectTag、Magnitude、Duration、MetaTag 由调用方根据 FCardEffect / FIntentEffect 填入。
 *
 * 本结构生命周期不超过一次 Execute 调用，Lightweight。
 */
struct FEffectContext
{
	FBattleState* State = nullptr;
	FBattleEventBus* Events = nullptr;

	EEffectSourceKind SourceKind = EEffectSourceKind::None;
	FGuid SourceInstanceId;        // 若 SourceKind == Card，这是卡实例；== EnemyPartIntent，这是部位实例

	EEffectTargetKind TargetKind = EEffectTargetKind::None;
	FGuid TargetInstanceId;        // Player 时可为 invalid

	FGameplayTag EffectTag;        // Effect.*
	int32 Magnitude = 0;
	int32 Duration  = 0;
	FGameplayTag MetaTag;          // 备用：区域、特殊标识等

	/**
	 * 腾挪类效果下用于排除的手牌 ID。
	 *
	 * 语义："这次腾挪不能选中这张卡"。典型用例：
	 * - 赤腹工蚁从双手区腾挪一张到其他区域。赤腹工蚁本身也在双手区，
	 *   如果不排除会把自己腾走，和"腾挪另一张卡"的规则意图不符。
	 *
	 * 为空（invalid FGuid）表示不排除任何卡。
	 */
	FGuid ExcludeHandCardId;
};
