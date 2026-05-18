// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"
#include "RuntimeCardInstance.generated.h"

class UCardDefinition;

/**
 * 卡牌的运行时实例。
 *
 * 对齐 Data_Schema_Draft §7。
 * 一张卡被带入战斗后生成一个 FRuntimeCardInstance，整场战斗保持同一 InstanceId。
 * 战斗内的一切对该卡的修改写在这里，不回写到 UCardDefinition。
 */
USTRUCT()
struct WACOMBATTLE_API FRuntimeCardInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid InstanceId;

	UPROPERTY()
	TObjectPtr<const UCardDefinition> Definition = nullptr;

	/** 本场战斗内 Cost 修正累计。RuntimeCost = clamp(BaseCost + RuntimeCostModifier, 0, ...)。 */
	UPROPERTY()
	int32 RuntimeCostModifier = 0;

	/** 本场战斗内临时关键字，不写入 Definition。 */
	UPROPERTY()
	FGameplayTagContainer TemporaryKeywords;

	/**
	 * 本卡入战时携带的 CapacityEffect tag 集合。
	 *
	 * 来源（见 Stage 4.5 design §8 / R4.3）：
	 *   - 来自 BattleDeck 原生位置的 instance：空集合。
	 *   - 来自 SpecialZone 的 instance：单元素集合 `{ B 主卡.Physique.CapacityEffect }`。
	 *
	 * 由 `UBattleSession::Initialize` 从 `FBattleDeckEntry::CapacityEffectTags` 拷贝而来，
	 * 战斗中只读，用于 `FCardEffectDispatcher` Damage 路径的 +3 修正等增量分支。
	 *
	 * 反射门槛：仅 UPROPERTY()，不暴露 Blueprint —— 战斗内核字段，不需要 WBP 直接读取。
	 */
	UPROPERTY()
	FGameplayTagContainer CapacityEffectTags;

	/** 本卡当前所在容器。由 BattleState 维护，非规则真相的扩展字段。 */
	UPROPERTY()
	ECardLocation Location = ECardLocation::Unknown;
};
