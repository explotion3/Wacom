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

	/** 本卡当前所在容器。由 BattleState 维护，非规则真相的扩展字段。 */
	UPROPERTY()
	ECardLocation Location = ECardLocation::Unknown;
};
