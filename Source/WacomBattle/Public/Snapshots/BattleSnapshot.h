// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "Snapshots/HandSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "BattleSnapshot.generated.h"

/**
 * 玩家快照。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FPlayerSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 CurrentHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 MaxHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Shield = 0;

	/** 玩家持有的状态集合。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGameplayTagContainer Statuses;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TMap<FGameplayTag, int32> StatusStacks;
};

/**
 * 卡牌容器计数快照。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FPileCountsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 DrawCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 DiscardCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 ExhaustCount = 0;
};

/**
 * 战斗整体快照。
 *
 * UI、日志、自动化测试读取本结构。禁止通过 Snapshot 回写到 BattleState。
 *
 * 生产方：WacomBattle/Private/Snapshots/BattleSnapshotBuilder。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleSnapshot
{
	GENERATED_BODY()

	/** 快照版本号，每次 BattleState 变更后递增。用于 UI 的增量刷新判断。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Version = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	EBattlePhase Phase = EBattlePhase::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 TurnNumber = 0;

	/** 当前等待值。每回合开始重置为 2。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 CurrentWaitValue = 2;

	/**
	 * 本场战斗内已累计打出的 Companion 关键字卡牌数。
	 * 驱动 Passive.Trigger.OnCompanionCount（拂晓飞蛾）。任一此类被动触发后清零。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 CompanionPlayedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FPlayerSnapshot Player;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FEnemySnapshot Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FHandQueueSnapshot Hand;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FPileCountsSnapshot PileCounts;

	/** 战斗结果。Phase == BattleEnd 时有效。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	EBattleOutcome Outcome = EBattleOutcome::Undetermined;
};
