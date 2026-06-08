// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Enemies/IntentDefinition.h"
#include "EnemyBehaviorDefinition.generated.h"

UENUM(BlueprintType)
enum class EWacomEnemyIntentSelectorMode : uint8
{
	Sequence      UMETA(DisplayName = "Sequence"),
	Weighted      UMETA(DisplayName = "Weighted"),
	PriorityFirst UMETA(DisplayName = "PriorityFirst"),
};

UENUM(BlueprintType)
enum class EWacomEnemyIntentConditionType : uint8
{
	Always                    UMETA(DisplayName = "Always"),
	OwnHpAtOrBelowRatio       UMETA(DisplayName = "OwnHpAtOrBelowRatio"),
	AnyPartHpAtOrBelowRatio   UMETA(DisplayName = "AnyPartHpAtOrBelowRatio"),
	PartDestroyed             UMETA(DisplayName = "PartDestroyed"),
	UnitPhase                 UMETA(DisplayName = "UnitPhase"),
	SelfStatusPresent         UMETA(DisplayName = "SelfStatusPresent"),
	PlayerStatusPresent       UMETA(DisplayName = "PlayerStatusPresent"),
	CooldownAvailable         UMETA(DisplayName = "CooldownAvailable"),
};

/** 单条敌人意图候选。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomEnemyBehaviorIntent
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FIntentDefinition Intent;

	/** 为空时使用 Intent.IntentId 作为冷却组。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName CooldownGroup = NAME_None;

	/** 本意图执行后阻塞后续多少次选择。0 表示无冷却。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior", meta = (ClampMin = "0", UIMin = "0"))
	int32 CooldownSelections = 0;

	FName GetEffectiveCooldownGroup() const
	{
		return CooldownGroup.IsNone() ? Intent.IntentId : CooldownGroup;
	}
};

/** Selector rule 条件。所有条件同时满足时 rule 才有效。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomEnemyIntentCondition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	EWacomEnemyIntentConditionType Type = EWacomEnemyIntentConditionType::Always;

	/** PartDestroyed / AnyPartHpAtOrBelowRatio 可用。为空表示任意部位。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName PartSlotId = NAME_None;

	/** UnitPhase 条件使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName PhaseId = NAME_None;

	/** SelfStatusPresent / PlayerStatusPresent 条件使用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FGameplayTag StatusTag;

	/** HP ratio 条件使用，范围 0..1。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HpRatioThreshold = 1.0f;

	/** CooldownAvailable 条件使用。为空时由 rule 的 IntentId 对应意图决定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName CooldownGroup = NAME_None;
};

/** 一个 intent 的选择规则。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomEnemyIntentSelectorRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName RuleId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName IntentId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	int32 Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior", meta = (ClampMin = "1", UIMin = "1"))
	int32 Weight = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TArray<FWacomEnemyIntentCondition> Conditions;

	/** false 时此 rule 选中后不写入该意图冷却。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	bool bConsumesCooldown = true;
};

/** 一个部位在某 phase 下可用的一组意图。 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomEnemyIntentSetDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName IntentSetId = NAME_None;

	/** 为空表示可作为任意部位 fallback intent set。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName AppliesToPartSlotId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	EWacomEnemyIntentSelectorMode SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TArray<FWacomEnemyBehaviorIntent> Intents;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TArray<FWacomEnemyIntentSelectorRule> SelectorRules;

	/** 所有 rule 都失效时使用；为空则按 selector mode fallback 到第一个可用 intent。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName FallbackIntentId = NAME_None;
};

USTRUCT(BlueprintType)
struct WACOMDATA_API FWacomEnemyPhaseDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName PhaseId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TArray<FWacomEnemyIntentSetDefinition> IntentSets;
};

/** 敌人行为定义：phase + intent set + selector rule。 */
UCLASS(BlueprintType)
class WACOMDATA_API UEnemyBehaviorDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName BehaviorId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName InitialPhaseId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TArray<FWacomEnemyPhaseDefinition> Phases;
};
