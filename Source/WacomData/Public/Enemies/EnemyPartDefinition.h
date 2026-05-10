// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyPartDefinition.generated.h"

/**
 * 敌方部位静态定义。
 *
 * 对齐 Data_Schema_Draft §6.2。
 * S1 只搭骨架，IntentSequence 等字段在 S6/S9 填充。
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

	// IntentSequence 将在 S6/S9 追加。
};
