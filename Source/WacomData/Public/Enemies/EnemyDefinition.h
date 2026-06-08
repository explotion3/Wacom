// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyDefinition.generated.h"

class UEnemyPartDefinition;
class UEnemyBehaviorDefinition;

/**
 * 敌人部位槽。
 * 包一层结构而非直接 TArray<TObjectPtr<UEnemyPartDefinition>>，
 * 为将来扩展 per-slot 参数（例如部位出生位置、初始 Shield 覆盖）留空间。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FEnemyPartSlot
{
	GENERATED_BODY()

	/** 敌人内局部部位槽位 ID。为空时兼容回退到 PartDef->PartId。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FName PartSlotId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyPartDefinition> PartDef = nullptr;

	/** 可选部位行为覆盖；为空时使用 EnemyDefinition.DefaultBehavior。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TObjectPtr<UEnemyBehaviorDefinition> BehaviorOverride = nullptr;

	/** 可选初始 intent set；为空时按 PartSlotId 匹配。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName InitialIntentSetId = NAME_None;
};

/** 敌人静态定义。 */
UCLASS(BlueprintType)
class WACOMDATA_API UEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FName EnemyId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	TObjectPtr<UEnemyBehaviorDefinition> DefaultBehavior = nullptr;

	/** 可选初始 phase 覆盖；为空时使用 DefaultBehavior.InitialPhaseId。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Behavior")
	FName DefaultPhaseId = NAME_None;

	/** 部位顺序由 Parts 的数组顺序决定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TArray<FEnemyPartSlot> Parts;
};
