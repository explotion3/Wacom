// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BattleEvent.generated.h"

class UCardDefinition;

/**
 * 战斗事件类型。
 *
 * 对齐 Architecture.md §7。
 * 事件是结算过程的记录流，给 UI 播表现、给日志写历史、给测试验证用。
 * 事件本身不作为战斗真相；断线重连或存档恢复都以 BattleSnapshot 为准。
 */
UENUM(BlueprintType)
enum class EBattleEventType : uint8
{
	None                  UMETA(DisplayName = "None"),
	BattleStarted         UMETA(DisplayName = "BattleStarted"),
	TurnStarted           UMETA(DisplayName = "TurnStarted"),
	CardsDrawn            UMETA(DisplayName = "CardsDrawn"),
	HandZoneChanged       UMETA(DisplayName = "HandZoneChanged"),      // 区域重新判定/腾挪后触发
	CardPlayed            UMETA(DisplayName = "CardPlayed"),
	InitiativeHit         UMETA(DisplayName = "InitiativeHit"),        // 先机命中（完美释放窗口成立）
	ResistanceResolved    UMETA(DisplayName = "ResistanceResolved"),
	PerfectReleaseResolved UMETA(DisplayName = "PerfectReleaseResolved"),
	DamageDealt           UMETA(DisplayName = "DamageDealt"),
	StatusApplied         UMETA(DisplayName = "StatusApplied"),
	InitiativePushed      UMETA(DisplayName = "InitiativePushed"),     // 打牌推进先机后的统一扣减
	WaitPerformed         UMETA(DisplayName = "WaitPerformed"),
	EnemyPartActed        UMETA(DisplayName = "EnemyPartActed"),
	EnemyPartHpEmptied    UMETA(DisplayName = "EnemyPartHpEmptied"),   // 部位被破坏
	EnemyKnockdown        UMETA(DisplayName = "EnemyKnockdown"),       // 击倒事件（第一阶段仅记录）
	KnockdownChoiceRequested UMETA(DisplayName = "KnockdownChoiceRequested"), // 等待玩家三选一
	KnockdownChoiceMade   UMETA(DisplayName = "KnockdownChoiceMade"),  // 玩家选完一项
	TurnEnded             UMETA(DisplayName = "TurnEnded"),
	PassiveTriggered      UMETA(DisplayName = "PassiveTriggered"),    // P3.5 占位：被动触发通知
	HandLimitDiscarded    UMETA(DisplayName = "HandLimitDiscarded"),  // 普通手牌上限导致弃牌
	CardGained            UMETA(DisplayName = "CardGained"),          // 战斗中获得一张新卡
	BattleEnded           UMETA(DisplayName = "BattleEnded"),
};

/**
 * 普通手牌上限弃牌来源。用于表现层区分是回合开始抽牌、中途抽牌，还是被动回手触发。
 */
UENUM(BlueprintType)
enum class EHandLimitDiscardSource : uint8
{
	None                    UMETA(DisplayName = "None"),
	TurnStart               UMETA(DisplayName = "TurnStart"),
	EffectDraw              UMETA(DisplayName = "EffectDraw"),
	PassiveOnCompanionCount UMETA(DisplayName = "PassiveOnCompanionCount"),
};

/**
 * 战斗事件。
 *
 * 第一阶段使用扁平字段变体：不同事件类型填不同字段，未使用的留默认值。
 * 后续事件种类过多时再切换到更严格的 polymorphic USTRUCT。
 *
 * 字段使用约定（非穷举）：
 * - CardsDrawn          ：Count = 抽牌数
 * - CardPlayed          ：CardInstanceId、ActorInstanceId = 目标部位 InstanceId
 * - InitiativeHit       ：ActorInstanceId = 被命中部位、Amount = 本次 RuntimeCost
 * - DamageDealt         ：ActorInstanceId = 受伤害单位、Amount = 实际扣血量
 * - StatusApplied       ：ActorInstanceId、Tag = Status.*、Amount = 层数
 * - InitiativePushed    ：Amount = 扣减量（RuntimeCost）
 * - WaitPerformed       ：Amount = 本次等待值
 * - EnemyPartActed      ：ActorInstanceId = 行动部位、Tag = Intent id（用 tag 承载方便扩展；第一阶段也可留空）
 * - EnemyPartHpEmptied  ：ActorInstanceId = 被破坏部位
 * - HandLimitDiscarded  ：CardInstanceId = 被弃掉的卡；ActorInstanceId = 触发源卡（仅 EffectDraw）
 * - CardGained          ：CardInstanceId = 战斗内新卡实例；ActorInstanceId = 来源部位；CardDefinition = 新卡定义；Count = EKnockdownChoice
 * - BattleEnded         ：Count = 1 表示胜利、0 表示失败（后续换专用字段）
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	EBattleEventType Type = EBattleEventType::None;

	/** 事件序号，单场战斗内唯一递增。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 Sequence = 0;

	/** 与事件相关的主体实例 ID（打出的卡、行动的部位、受伤单位等）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FGuid ActorInstanceId;

	/** 若事件涉及一张卡，这是该卡的运行时实例 ID。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FGuid CardInstanceId;

	/** 通用标签字段。按 Type 语义使用（状态 tag、意图 id、效果 tag 等）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	FGameplayTag Tag;

	/** 通用数值字段：伤害、层数、等待值、RuntimeCost 等。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 Amount = 0;

	/** 通用计数字段：抽牌数、连击次数等。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	int32 Count = 0;

	/** 普通手牌上限弃牌的来源，仅 HandLimitDiscarded 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	EHandLimitDiscardSource HandLimitDiscardSource = EHandLimitDiscardSource::None;

	/** 事件涉及的新卡定义。第一版仅 CardGained 使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Event")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;
};
