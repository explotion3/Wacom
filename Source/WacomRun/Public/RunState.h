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
 * R5 骨架阶段：只保留最关键字段。后续扩展：
 *   - 当前 HP（跨战斗保留；现在每场战斗从 Character BaseMaxHp 重置）
 *   - 当前卡组（现在战斗内用 Character StarterDeck；未来支持换卡/加卡）
 *   - 金币 / 装备 / 各种 Buff / 事件标记
 *   - 当前地图位置 / 已探索区域
 *
 * 为什么是 USTRUCT（而不是纯 C++ struct）：
 *   - 含有 UObject 指针（Character、DefeatedEnemies），需要反射 GC 跟踪
 *   - 放在 URunSession 的 UPROPERTY 里就能被 UObject 系统管理引用
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
};
