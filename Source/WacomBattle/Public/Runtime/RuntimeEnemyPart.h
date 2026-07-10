// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Enemies/IntentDefinition.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "RuntimeEnemyPart.generated.h"

class UEnemyBehaviorDefinition;
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
	TObjectPtr<const UEnemyBehaviorDefinition> BehaviorDefinition = nullptr;

	UPROPERTY()
	FName CurrentPhaseId = NAME_None;

	UPROPERTY()
	FName PreferredIntentSetId = NAME_None;

	UPROPERTY()
	int32 CurrentHp = 0;

	UPROPERTY()
	FName CurrentIntentSetId = NAME_None;

	UPROPERTY()
	FName CurrentIntentId = NAME_None;

	UPROPERTY()
	FIntentDefinition CurrentIntent;

	UPROPERTY()
	int32 BehaviorSequenceCursor = 0;

	UPROPERTY()
	int32 BehaviorSelectionCounter = 0;

	UPROPERTY()
	TMap<FName, int32> IntentCooldownSelectionsRemaining;

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

	/** Stack status 的唯一运行时真相，只保存正层数。Shield 单独存字段。 */
	UPROPERTY()
	TMap<FGameplayTag, int32> StatusStacks;
};
