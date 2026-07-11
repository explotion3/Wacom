// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Effects/Semantics/EffectSemanticDescriptor.h"

class FEffectSemanticRegistry final
{
public:
	static const FEffectSemanticDescriptor* Find(const FGameplayTag& EffectType);
	static TConstArrayView<FEffectSemanticDescriptor> GetAll();
	static TArray<FGameplayTag> GetSupportedCardEffectTypes();
	static TArray<FGameplayTag> GetSupportedEnemyIntentEffectTypes();
	static bool IsSupportedCardEffectType(const FGameplayTag& EffectType);
	static bool IsSupportedEnemyIntentEffectType(const FGameplayTag& EffectType);
	static bool IsSupportedMagnitudeSource(const FGameplayTag& MagnitudeSource);
	static bool IsSupportedCardEffectMagnitudeSource(const FGameplayTag& EffectType, const FGameplayTag& MagnitudeSource);
	static bool IsSupportedCardEffectTarget(
		const FGameplayTag& EffectType,
		const FGameplayTag& Target,
		FWacomBattleRuleContentContract::ECardEffectContext Context,
		ECardTargetMode CardTargetMode);
	static bool IsSupportedEnemyIntentEffectTarget(const FGameplayTag& EffectType, const FGameplayTag& Target);
	static bool CardEffectRequiresTargetZone(const FGameplayTag& EffectType);
	static bool CardEffectAllowsTargetZone(const FGameplayTag& EffectType);
	static bool CardEffectTargetZoneMustBeHandZone(const FGameplayTag& EffectType);
	static bool CardEffectTargetZoneMustBeCardLocation(const FGameplayTag& EffectType);
	static bool CardEffectTargetZoneMustBeStackStatus(const FGameplayTag& EffectType);
	static bool CardEffectTargetZoneMustBeCardKeyword(const FGameplayTag& EffectType);
	static bool CardEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType);
	static bool EnemyIntentEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType);
	static bool EffectUsesPositiveMagnitude(const FGameplayTag& EffectType);
};

void AppendCombatantEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors);
void AppendCardMovementEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors);
void AppendCardRuntimeEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors);
void AppendInitiativeEffectSemanticDescriptors(TArray<FEffectSemanticDescriptor>& OutDescriptors);
