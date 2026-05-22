// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EnemySnapshot.generated.h"

class UEnemyDefinition;
class UEnemyPartDefinition;

/**
 * 意图的只读快照。UI 展示"敌人接下来要干什么"。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FIntentSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName IntentId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Initiative = 0;

	/** 抵抗比较用数值，来自当前意图定义。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 ResistanceValue = 0;
};

/**
 * 敌方部位的只读快照。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FEnemyPartSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TObjectPtr<const UEnemyPartDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 CurrentHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 MaxHp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bDestroyed = false;

	/** 当前意图。若破坏或其它情况无意图，IntentId 为 NAME_None。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FIntentSnapshot CurrentIntent;

	/** 当前状态集合。层数查 StatusStacks。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGameplayTagContainer Statuses;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TMap<FGameplayTag, int32> StatusStacks;
};

/**
 * 敌人整体快照。
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FEnemySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TObjectPtr<const UEnemyDefinition> Definition = nullptr;

	/** 按部位定义顺序排列。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TArray<FEnemyPartSnapshot> Parts;

	/** 所有存活部位当前先机之和。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 InitiativeSum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bAllPartsDestroyed = false;
};
