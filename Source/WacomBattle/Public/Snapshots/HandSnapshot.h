// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/CardUpgradeTypes.h"
#include "GameplayTagContainer.h"
#include "Snapshots/BattleCardRuntimeSnapshot.h"
#include "Types/WacomEnums.h"
#include "HandSnapshot.generated.h"

class UCardDefinition;

/**
 * 手牌队列中单张卡的只读快照。
 *
 * UI 不直接引用 FRuntimeCardInstance，只通过本结构获取展示所需信息。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FHandCardSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TObjectPtr<const UCardDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	EWacomCardUpgradeTier UpgradeTier = EWacomCardUpgradeTier::White;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 CurrentDurability = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bHasFiniteDurability = false;

	/** 本场战斗内实际生效的 Cost（含修正）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 RuntimeCost = 0;

	/** 是否仅从费用角度满足 RuntimeCost <= 敌方先机总和。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bIsCostLegal = false;

	/** 当前卡牌状态的标签投影；层数读取 StatusStacks。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGameplayTagContainer Statuses;

	/** 当前卡牌 stack status。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TMap<FGameplayTag, int32> StatusStacks;

	/** 冻结会阻止正式出牌，但不阻止结构性 Target Preview。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bIsFrozen = false;

	/** 本卡当前所在区域。可能为 None（左右手锚点缺失时）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	EHandZone Zone = EHandZone::None;

	/** 本卡是否为手牌锚点（左手或右手）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bIsHandAnchor = false;

	/** 本卡是否同时满足费用和卡牌运行时限制。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bIsPlayable = false;

	/** 本卡当前是否拥有迅捷关键词（含战斗内临时关键词）。用于 UI 预测，不改变规则判断来源。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bIsSwift = false;

	/**
	 * 各主动效果当前确定性数值。包含战斗内来源卡加值/倍率，不包含目标专属条件或暴击。
	 * UI 应以 EffectIndex 匹配；目标预演可以临时覆盖这里的静止卡面数值。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TArray<FBattleCardEffectMagnitudeSnapshot> CurrentEffectMagnitudes;
};

/**
 * 整个手牌队列的快照。Cards 按从左到右的顺序排列。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FHandQueueSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TArray<FHandCardSnapshot> Cards;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bLeftHandPresent = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bRightHandPresent = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 NormalCardCount = 0;   // 不计左右手锚点的普通卡牌数

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 NormalCardLimit = 10;  // 普通卡手牌上限，不计左右手锚点
};
