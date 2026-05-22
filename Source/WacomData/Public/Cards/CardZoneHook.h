// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Cards/CardEffect.h"
#include "CardZoneHook.generated.h"

/**
 * 卡牌区域钩子。当卡牌处于指定区域、指定触发时机时追加效果或修正。
 *
 * 当前支持：
 * - ZoneHook.Trigger.OnPlay
 * - ZoneHook.Trigger.OnPerfectReleaseHit
 */
USTRUCT(BlueprintType)
struct WACOMDATA_API FCardZoneHook
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Zone")
	FGameplayTag Zone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Zone")
	FGameplayTag Trigger;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Zone")
	TArray<FCardEffect> ExtraEffects;
};
