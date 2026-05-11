// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"

class UEnemyDefinition;
class UCharacterDefinition;

/**
 * 玩家运行时状态。
 *
 * 所有和"玩家本体"相关的数值、状态、累计计数都集中在这里。
 * 卡牌容器和敌人部位不放在这里（见 FCardContainers / FEnemyState）。
 */
struct FPlayerState
{
	int32 CurrentHp = 0;
	int32 MaxHp     = 0;
	int32 Shield    = 0;

	TObjectPtr<const UCharacterDefinition> CharacterDef = nullptr;

	/** 玩家持有的状态（P3.1 起支持 Poison，未来可扩展 Slow/Twilight/Freeze）。 */
	FGameplayTagContainer Statuses;
	TMap<FGameplayTag, int32> StatusStacks;

	/**
	 * 本场战斗内累计打出的 Companion 关键字卡牌数。
	 * 用于 Passive.Trigger.OnCompanionCount（拂晓飞蛾）。
	 * 任一 OnCompanionCount 被动触发后清零。
	 */
	int32 CompanionPlayedCount = 0;
};

/**
 * 卡牌容器。一场战斗内所有卡 + 六个定位容器。
 *
 * AllCards 持有所有卡的权威实例。DrawPile/Hand/... 只存 InstanceId，
 * 真实的 FRuntimeCardInstance::Location 字段和容器归属保持一致。
 *
 * **不变量**：AllCards 整场战斗只追加不删除（只改元素的 Location 字段），
 * 所以 CardIndexById 的索引一经建立就永远有效。若未来引入"战斗内销毁卡"，
 * 需要把被销毁的卡标记而非物理移除，或同步更新索引。
 */
struct FCardContainers
{
	TArray<FRuntimeCardInstance> AllCards;  // 一场战斗内所有卡的权威实例
	TMap<FGuid, int32> CardIndexById;       // InstanceId → AllCards 索引，O(1) 查找

	TArray<FGuid> DrawPile;                 // 抽牌堆
	TArray<FGuid> Hand;                     // 手牌队列（从左到右）
	TArray<FGuid> DiscardPile;              // 弃牌区
	TArray<FGuid> ExhaustPile;              // 消耗区
	TArray<FGuid> Limbo;                    // 本回合离手但不入任何区域的左右手

	/** 左手/右手运行时实例 ID。整场战斗不变。 */
	FGuid LeftHandInstanceId;
	FGuid RightHandInstanceId;
};

/**
 * 敌人运行时状态。
 *
 * **不变量**：Parts 整场战斗只追加不删除（部位破坏仅置 bDestroyed 标记）。
 */
struct FEnemyState
{
	TObjectPtr<const UEnemyDefinition> Definition = nullptr;
	TArray<FRuntimeEnemyPart> Parts;        // 按部位顺序
	TMap<FGuid, int32> PartIndexById;       // InstanceId → Parts 索引，O(1) 查找
};

/**
 * 战斗真相容器。
 *
 * 仅限 WacomBattle/Private 引用，外部模块编译期不可见。对外入口是
 * UBattleSession + FBattleSnapshot + FBattleEvent。
 *
 * 第一阶段设计为非反射 struct：
 * - 不需要暴露给蓝图或序列化。
 * - 嵌套持有 FRuntimeCardInstance 的 TArray 时，非反射下仍可正常复制。
 * - TObjectPtr 在非反射容器里需要在 GC 侧手动 AddReferencedObjects。第一阶段
 *   BattleState 由 UBattleSession 持有，所有对象引用通过 Session 间接追踪。
 *
 * 字段分组（优化阶段）：
 * - Meta（阶段、回合、随机源、版本号）
 * - Player（FPlayerState）
 * - Cards  （FCardContainers）
 * - Enemy  （FEnemyState）
 *
 * 后续若有存档/网络需求，再升级为 USTRUCT 或切换到 UObject 容器。
 */
struct FBattleState
{
	// ---- Meta ----
	EBattlePhase Phase = EBattlePhase::None;
	int32 TurnNumber = 0;
	int32 CurrentWaitValue = 2;
	EBattleOutcome Outcome = EBattleOutcome::Undetermined;

	/** 随机源。测试可注入 seed 复现。 */
	FRandomStream Rng;

	/** 每次状态变更递增，用于 Snapshot Version 字段。 */
	int32 StateVersion = 0;

	// ---- 分组字段 ----
	FPlayerState    Player;
	FCardContainers Cards;
	FEnemyState     Enemy;
};
