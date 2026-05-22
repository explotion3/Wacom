// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyDefinition.generated.h"

class UEnemyPartDefinition;

/**
 * 敌人部位槽。
 * 包一层结构而非直接 TArray<TObjectPtr<UEnemyPartDefinition>>，
 * 为将来扩展 per-slot 参数（例如部位出生位置、初始 Shield 覆盖）留空间。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FEnemyPartSlot
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyPartDefinition> PartDef = nullptr;
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

	/** 部位顺序由 Parts 的数组顺序决定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TArray<FEnemyPartSlot> Parts;
};
