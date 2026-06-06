// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EncounterDefinition.generated.h"

class UEnemyDefinition;

/**
 * Encounter 内的敌人槽静态定义。
 *
 * WacomData 只描述内容结构；运行时会由上层模块把这些槽转换为 Battle init params。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FEncounterEnemySlot
{
	GENERATED_BODY()

	/** Encounter 内稳定敌人槽 ID。后续会映射到 Battle EnemySlotId，参与多敌人部位身份。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Encounter",
		meta = (ToolTip = "Encounter 内稳定敌人槽 ID。后续会映射到 Battle EnemySlotId，参与多敌人部位身份。"))
	FName EnemySlotId = TEXT("Enemy");

	/** 该敌人槽使用的静态敌人定义。允许多个不同槽引用同一 EnemyDefinition。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Encounter",
		meta = (ToolTip = "该敌人槽使用的静态敌人定义。允许多个不同槽引用同一 EnemyDefinition。"))
	TObjectPtr<UEnemyDefinition> EnemyDefinition = nullptr;
};

/**
 * 单个战斗 Encounter 的静态内容定义。
 *
 * 本资产只声明“这场战斗有哪些敌人槽”。它不保存场景 Actor、运行时进度、视觉 prefab、
 * 奖励、阵型或存档状态。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UEncounterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Encounter 内容 ID，用于内容识别、debug 和后续运行时映射；不从资产名自动回退。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Encounter",
		meta = (ToolTip = "Encounter 内容 ID，用于内容识别、debug 和后续运行时映射；不从资产名自动回退，必须显式填写。"))
	FName EncounterDefinitionId = NAME_None;

	/** Encounter 显示名。可为空；规则层不依赖显示文本。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Encounter",
		meta = (ToolTip = "Encounter 显示名。可为空；规则层不依赖显示文本。"))
	FText DisplayName;

	/** 敌人槽列表。数组顺序表示 Encounter 敌人槽顺序；不表示场景摆放位置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Encounter",
		meta = (ToolTip = "敌人槽列表。数组顺序表示 Encounter 敌人槽顺序；不表示场景摆放位置。"))
	TArray<FEncounterEnemySlot> EnemySlots;
};
