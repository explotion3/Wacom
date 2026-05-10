// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/IntentEffect.h"
#include "IntentDefinition.generated.h"

/**
 * 单条意图定义。对齐 Data_Schema_Draft §6.3。
 *
 * 意图描述"敌方部位下一次行动要做什么"，自带先机值和抵抗比较用值。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FIntentDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	FName IntentId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	FText DisplayName;

	/** 本意图的先机值。部位刷新到该意图时，CurrentInitiative 被设为此值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	int32 Initiative = 0;

	/** 抵抗比较用数值。Data_Schema_Draft §4：攻击填伤害值，非攻击填 0。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	int32 ResistanceValue = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	TArray<FIntentEffect> Effects;
};
