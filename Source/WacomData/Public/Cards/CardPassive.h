// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Cards/CardEffect.h"
#include "CardPassive.generated.h"

/**
 * 卡牌被动触发。对齐 Data_Schema_Draft §5.5。
 *
 * 第一阶段支持：
 * - Passive.Trigger.AfterPlayed：烁光蝶的"打出后腾挪到随机区域"
 * - Passive.Trigger.OnCompanionCount：拂晓飞蛾的"每打三张伙伴回手"（S8+）
 * - Passive.Trigger.OnTwilightTriggered：暮蛉的"触发暮气时"（S8+）
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FCardPassive
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	FGameplayTag Trigger;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	TArray<FCardEffect> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Passive")
	int32 TriggerThreshold = 0;
};
