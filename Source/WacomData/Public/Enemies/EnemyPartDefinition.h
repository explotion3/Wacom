// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemies/IntentDefinition.h"
#include "EnemyPartDefinition.generated.h"

/**
 * 敌方部位静态定义。
 *
 * 对齐 Data_Schema_Draft §6.2。
 */
UCLASS(BlueprintType)
class WACOMDATA_API UEnemyPartDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FName PartId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	int32 MaxHp = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	int32 InitialIntentIndex = 0;

	/** 循环执行。对齐 Battle_Rules §2 / §10。第一阶段蛇的三个部位各有 3 条意图。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TArray<FIntentDefinition> IntentSequence;
};
