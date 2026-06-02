// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/WacomEnums.h"

/**
 * Authoring-facing view of the currently executable battle rule content.
 *
 * This is a read-only contract for editor validation and documentation. It
 * mirrors the active resolver/dispatcher behavior; it does not execute rules.
 */
struct WACOMBATTLE_API FWacomBattleRuleContentContract
{
	enum class ECardEffectContext : uint8
	{
		MainEffect,
		PerfectRelease,
		ZoneHookOnPlay,
		PassiveEffect,
	};

	static bool IsSupportedCardEffectType(const FGameplayTag& EffectType);
	static bool IsSupportedEnemyIntentEffectType(const FGameplayTag& EffectType);

	static bool IsSupportedMagnitudeSource(const FGameplayTag& MagnitudeSource);
	static bool IsSupportedCardEffectMagnitudeSource(
		const FGameplayTag& EffectType,
		const FGameplayTag& MagnitudeSource);
	static bool IsSupportedConditionType(const FGameplayTag& ConditionType);
	static bool IsStackStatusTag(const FGameplayTag& StatusTag);
	static bool IsCardKeywordTag(const FGameplayTag& KeywordTag);
	static bool IsCardLocationTag(const FGameplayTag& LocationTag);
	static bool IsHandZoneTag(const FGameplayTag& ZoneTag);

	static bool IsExecutablePassiveTrigger(const FGameplayTag& Trigger);
	static bool IsSpecialPassiveTriggerWithoutEffects(const FGameplayTag& Trigger);
	static bool IsEventOnlyPassiveTrigger(const FGameplayTag& Trigger);
	static bool IsReservedPassiveTrigger(const FGameplayTag& Trigger);

	static bool IsSupportedCardEffectTarget(
		const FGameplayTag& EffectType,
		const FGameplayTag& Target,
		ECardEffectContext Context,
		ECardTargetMode CardTargetMode);

	static bool IsSupportedEnemyIntentEffectTarget(
		const FGameplayTag& EffectType,
		const FGameplayTag& Target);

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
