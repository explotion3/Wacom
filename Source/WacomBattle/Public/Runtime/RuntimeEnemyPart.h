// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Runtime/RuntimeStatus.h"
#include "RuntimeEnemyPart.generated.h"

class UEnemyPartDefinition;

/**
 * 敌方部位运行时实例。
 *
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
	FBattlePartSlotIdentity Identity;

	UPROPERTY()
	int32 CurrentHp = 0;

	/** 当前意图在 Definition->IntentSequence 中的索引。 */
	UPROPERTY()
	int32 CurrentIntentIndex = 0;

	/**
	 * 当前先机。
	 *
	 * Enemy Initiative Sum 使用原值，不夹到 [0, +inf]。
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

	/** 便于把 FStatusInstance 的 Duration 一并承载；多数当前状态仍通过 StatusStacks 结算。 */
	UPROPERTY()
	TArray<FStatusInstance> StatusInstances;
};
