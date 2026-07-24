// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "EnemySnapshot.generated.h"

class UEnemyDefinition;
class UEnemyPartDefinition;

/** Enemy Intent 对玩家公开的规范化目标。UI 不需要读取 authoring DataAsset 推断目标语义。 */
UENUM(BlueprintType)
enum class EBattleIntentEffectTargetKind : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	Player UMETA(DisplayName = "Player"),
	SelfEnemyPart UMETA(DisplayName = "Self Enemy Part"),
	RandomPlayerHandCards UMETA(DisplayName = "Random Player Hand Cards"),
	AllPlayerHandCards UMETA(DisplayName = "All Player Hand Cards"),
};

/** 当前 Intent 中一条按执行顺序保留的权威公开效果。 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleIntentEffectSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGameplayTag EffectType;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	EBattleIntentEffectTargetKind TargetKind =
		EBattleIntentEffectTargetKind::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Magnitude = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Duration = 0;

	/** RandomPlayerHandCards 的不重复目标数量；其它目标为 0。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 TargetCount = 0;
};

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

	/** 当前意图是否包含面向玩家的正伤害效果。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bIsAttackIntent = false;

	/** 攻击意图的最高单段伤害；非攻击意图为 0。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 PeakAttackDamage = 0;

	/** 当前 Intent 的公开效果，保持正式执行顺序；UI 可做相邻等价行聚合但不得重排。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TArray<FBattleIntentEffectSnapshot> Effects;
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
	FBattlePartSlotIdentity Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FBattleEnemyPartKey PartKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName PartSlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName CurrentPhaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName CurrentIntentSetId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName CurrentIntentId = NAME_None;

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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName EncounterId = TEXT("Encounter");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FBattleEnemyUnitKey UnitKey;

	/** 按部位定义顺序排列。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	TArray<FEnemyPartSnapshot> Parts;

	/** 所有存活部位当前先机之和。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 InitiativeSum = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	bool bAllPartsDestroyed = false;
};
