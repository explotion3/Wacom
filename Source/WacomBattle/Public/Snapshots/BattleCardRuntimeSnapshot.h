// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BattleCardRuntimeSnapshot.generated.h"

/**
 * One card effect's current deterministic, untargeted magnitude.
 *
 * Battle computes this fact from the runtime card and publishes it through
 * snapshots so resting card UI never needs to reimplement gameplay rules.
 * Target-dependent facts and critical rolls are intentionally excluded:
 * target preview may temporarily overlay a more precise value later.
 */
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleCardEffectMagnitudeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 EffectIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	FGameplayTag EffectType;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Snapshot")
	int32 Magnitude = 0;
};
