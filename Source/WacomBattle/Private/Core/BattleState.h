// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"

class UEnemyDefinition;
class UCharacterDefinition;

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
 * 后续若有存档/网络需求，再升级为 USTRUCT 或切换到 UObject 容器。
 */
struct FBattleState
{
	/** 战斗阶段。 */
	EBattlePhase Phase = EBattlePhase::None;

	int32 TurnNumber = 0;
	int32 CurrentWaitValue = 2;

	EBattleOutcome Outcome = EBattleOutcome::Undetermined;

	/** 随机源。测试可注入 seed 复现。 */
	FRandomStream Rng;

	/** 每次状态变更递增，用于 Snapshot Version 字段。 */
	int32 StateVersion = 0;

	// ---- 玩家 ----
	int32 PlayerCurrentHp = 0;
	int32 PlayerMaxHp = 0;
	int32 PlayerShield = 0;
	TObjectPtr<const UCharacterDefinition> CharacterDef = nullptr;

	// ---- 卡牌容器 ----
	TArray<FRuntimeCardInstance> AllCards;          // 一场战斗内所有卡的权威实例
	TArray<FGuid> DrawPile;                         // 抽牌堆
	TArray<FGuid> Hand;                             // 手牌队列（从左到右）
	TArray<FGuid> DiscardPile;                      // 弃牌区
	TArray<FGuid> ExhaustPile;                      // 消耗区
	TArray<FGuid> Limbo;                            // 本回合离手但不入任何区域的左右手

	/** 左手/右手运行时实例 ID。整场战斗不变。 */
	FGuid LeftHandInstanceId;
	FGuid RightHandInstanceId;

	// ---- 敌人 ----
	TObjectPtr<const UEnemyDefinition> EnemyDef = nullptr;
	TArray<FRuntimeEnemyPart> EnemyParts;           // 按部位顺序
};
