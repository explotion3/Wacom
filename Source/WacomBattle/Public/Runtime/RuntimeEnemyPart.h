// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/RuntimeStatus.h"
#include "RuntimeEnemyPart.generated.h"

class UEnemyPartDefinition;

/**
 * 敌方部位运行时实例。
 *
 * 对齐 Data_Schema_Draft §7。
 * 行动解析、抵抗判定、状态结算都作用于本结构。
 */
USTRUCT()
struct WACOMBATTLE_API FRuntimeEnemyPart
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid InstanceId;

	UPROPERTY()
	TObjectPtr<const UEnemyPartDefinition> Definition = nullptr;

	UPROPERTY()
	int32 CurrentHp = 0;

	/** 当前意图在 Definition->IntentSequence 中的索引。 */
	UPROPERTY()
	int32 CurrentIntentIndex = 0;

	/**
	 * 当前先机。
	 *
	 * 对齐 Battle_Rules §5：Enemy Initiative Sum 使用原值，不夹到 [0, +inf]。
	 * 行动结算流程负责把 <=0 的部位推入行动子流程后再由刷新逻辑重置。
	 */
	UPROPERTY()
	int32 CurrentInitiative = 0;

	UPROPERTY()
	bool bDestroyed = false;

	UPROPERTY()
	int32 Shield = 0;

	/** 状态标签集合与层数。Shield 单独存字段不进状态模型。 */
	UPROPERTY()
	FGameplayTagContainer Statuses;

	UPROPERTY()
	TMap<FGameplayTag, int32> StatusStacks;

	/** 便于未来把 FStatusInstance 的 Duration 一并承载。第一阶段可能仅通过 StatusStacks 使用。 */
	UPROPERTY()
	TArray<FStatusInstance> StatusInstances;
};
