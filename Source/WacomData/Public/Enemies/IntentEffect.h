// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IntentEffect.generated.h"

/**
 * 意图单个效果条目。对齐 Data_Schema_Draft §6.3。
 *
 * 第一阶段 EffectType 复用 Card 的 Effect.* tag 体系。意图打到玩家时
 * Target = Target.Player，加自身护盾时 Target = Target.Self。
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FIntentEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	FGameplayTag EffectType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	int32 Magnitude = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	FGameplayTag Target;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intent")
	int32 Duration = 0;
};
