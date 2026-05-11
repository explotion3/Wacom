// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Session/BattleSession.h"  // FBattleInitParams
#include "RunState.generated.h"

class UCharacterDefinition;
class UEnemyDefinition;

/**
 * 一次冒险（Run）的持久状态。
 *
 * 第一阶段字段：
 *   - Character / BattleSeed / DefeatedEnemies / bRunActive（R5 骨架）
 *   - DestroyedTriggerIds / PlayerTransform（S1 存档骨架）
 *
 * 后续扩展（未实现）：
 *   - 跨战斗 HP 传递
 *   - 当前卡组 / 金币 / 装备 / 各种 Buff / 事件标记
 *
 * 为什么是 USTRUCT（而不是纯 C++ struct）：
 *   - 含有 UObject 指针（Character、DefeatedEnemies），需要反射 GC 跟踪
 *   - 放在 URunSession 的 UPROPERTY 里就能被 UObject 系统管理引用
 *
 * 注意：FRunState 是内存数据层，不直接序列化到磁盘。
 * 磁盘格式见 UWacomSaveGame，两者之间做字段拷贝，参见 Docs/Save_System_Plan.md §3。
 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunState
{
	GENERATED_BODY()

	/** 玩家选择的角色。第一阶段固定为 BugGirl。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run")
	TObjectPtr<UCharacterDefinition> Character = nullptr;

	/**
	 * 战斗随机种子。
	 * 0 表示每场战斗独立随机；非 0 时用于复现。
	 * 第二阶段起可能基于 Run 全局种子派生每场战斗种子。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run")
	int32 BattleSeed = 0;

	/** 已击败的敌人 Definition 列表。用于不重复、计分、或后续事件解锁。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TArray<TObjectPtr<UEnemyDefinition>> DefeatedEnemies;

	/** 当前 Run 是否仍在进行（玩家未死亡、未主动退出）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	bool bRunActive = true;

	/**
	 * 已被永久销毁的场景触发器 ID 列表。
	 * Key 来自 ABattleTriggerActor::PersistentId。
	 * 关卡加载时用于跳过重新创建 Trigger。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	TSet<FName> DestroyedTriggerIds;

	/** 玩家在探索地图的 Transform。仅当 bHasPlayerTransform == true 时有效。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	FTransform PlayerTransform = FTransform::Identity;

	/** PlayerTransform 是否有效；新开 Run 时为 false，玩家探索 / 存档时置 true。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run")
	bool bHasPlayerTransform = false;
};
