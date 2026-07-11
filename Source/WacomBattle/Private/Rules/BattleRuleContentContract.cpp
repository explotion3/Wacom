// Copyright Wacom. All Rights Reserved.

#include "Rules/BattleRuleContentContract.h"

#include "Effects/Semantics/EffectSemanticRegistry.h"
#include "Tags/WacomGameplayTags.h"

bool FWacomBattleRuleContentContract::IsSupportedCardEffectType(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::IsSupportedCardEffectType(EffectType);
}

bool FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectType(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::IsSupportedEnemyIntentEffectType(EffectType);
}

TArray<FGameplayTag> FWacomBattleRuleContentContract::GetSupportedCardEffectTypes()
{
	return FEffectSemanticRegistry::GetSupportedCardEffectTypes();
}

TArray<FGameplayTag> FWacomBattleRuleContentContract::GetSupportedEnemyIntentEffectTypes()
{
	return FEffectSemanticRegistry::GetSupportedEnemyIntentEffectTypes();
}

bool FWacomBattleRuleContentContract::IsSupportedMagnitudeSource(const FGameplayTag& MagnitudeSource)
{
	return FEffectSemanticRegistry::IsSupportedMagnitudeSource(MagnitudeSource);
}

bool FWacomBattleRuleContentContract::IsSupportedCardEffectMagnitudeSource(
	const FGameplayTag& EffectType,
	const FGameplayTag& MagnitudeSource)
{
	return FEffectSemanticRegistry::IsSupportedCardEffectMagnitudeSource(
		EffectType,
		MagnitudeSource);
}

bool FWacomBattleRuleContentContract::IsSupportedConditionType(const FGameplayTag& ConditionType)
{
	return !ConditionType.IsValid()
		|| ConditionType == WacomTags::Condition_Self_InZone
		|| ConditionType == WacomTags::Condition_Target_HasStatus;
}

bool FWacomBattleRuleContentContract::IsStackStatusTag(const FGameplayTag& StatusTag)
{
	return StatusTag == WacomTags::Status_Poison
		|| StatusTag == WacomTags::Status_Freeze
		|| StatusTag == WacomTags::Status_Twilight
		|| StatusTag == WacomTags::Status_Stunned;
}

bool FWacomBattleRuleContentContract::IsRemovableCombatantStatusTag(
	const FGameplayTag& StatusTag)
{
	// Enemy Slow is an immediate initiative operation; it has no lingering
	// combatant stack to remove. Card Slow is owned by card-state lifecycle.
	return StatusTag == WacomTags::Status_Poison
		|| StatusTag == WacomTags::Status_Freeze
		|| StatusTag == WacomTags::Status_Twilight
		|| StatusTag == WacomTags::Status_Stunned;
}

bool FWacomBattleRuleContentContract::IsCardKeywordTag(const FGameplayTag& KeywordTag)
{
	return KeywordTag == WacomTags::Card_Keyword_Swift
		|| KeywordTag == WacomTags::Card_Keyword_Retain
		|| KeywordTag == WacomTags::Card_Keyword_Combo
		|| KeywordTag == WacomTags::Card_Keyword_Companion
		|| KeywordTag == WacomTags::Card_Keyword_Weapon
		|| KeywordTag == WacomTags::Card_Keyword_Tool
		|| KeywordTag == WacomTags::Card_Keyword_Hand
		|| KeywordTag == WacomTags::Card_Keyword_Exhaust
		|| KeywordTag == WacomTags::Card_Keyword_BagProvider
		|| KeywordTag == WacomTags::Card_Keyword_DeleteProvider;
}

bool FWacomBattleRuleContentContract::IsCardLocationTag(const FGameplayTag& LocationTag)
{
	return LocationTag == WacomTags::CardLocation_Draw
		|| LocationTag == WacomTags::CardLocation_Discard
		|| LocationTag == WacomTags::CardLocation_Exhaust;
}

bool FWacomBattleRuleContentContract::IsHandZoneTag(const FGameplayTag& ZoneTag)
{
	return ZoneTag == WacomTags::HandZone_Left
		|| ZoneTag == WacomTags::HandZone_Both
		|| ZoneTag == WacomTags::HandZone_Right;
}

bool FWacomBattleRuleContentContract::IsExecutablePassiveTrigger(const FGameplayTag& Trigger)
{
	return Trigger == WacomTags::Passive_Trigger_AfterPlayed
		|| Trigger == WacomTags::Passive_Trigger_OnDiscard;
}

bool FWacomBattleRuleContentContract::IsSpecialPassiveTriggerWithoutEffects(const FGameplayTag& Trigger)
{
	return Trigger == WacomTags::Passive_Trigger_OnCompanionCount;
}

bool FWacomBattleRuleContentContract::IsEventOnlyPassiveTrigger(const FGameplayTag& Trigger)
{
	return Trigger == WacomTags::Passive_Trigger_OnTwilightTriggered;
}

bool FWacomBattleRuleContentContract::IsReservedPassiveTrigger(const FGameplayTag& Trigger)
{
	return Trigger == WacomTags::Passive_Trigger_OnTurnStart
		|| Trigger == WacomTags::Passive_Trigger_OnTurnEnd
		|| Trigger == WacomTags::Passive_Trigger_OnDraw;
}

bool FWacomBattleRuleContentContract::IsSupportedCardEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target,
	ECardEffectContext Context,
	ECardTargetMode CardTargetMode)
{
	return FEffectSemanticRegistry::IsSupportedCardEffectTarget(
		EffectType,
		Target,
		Context,
		CardTargetMode);
}

bool FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target)
{
	return FEffectSemanticRegistry::IsSupportedEnemyIntentEffectTarget(
		EffectType,
		Target);
}

bool FWacomBattleRuleContentContract::EnemyIntentEffectUsesHandAfflictionDelivery(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target)
{
	return Target == WacomTags::Target_Player
		&& (EffectType == WacomTags::Effect_ApplyStatus_Slow
			|| EffectType == WacomTags::Effect_ApplyStatus_Freeze
			|| EffectType == WacomTags::Effect_ApplyStatus_Twilight);
}

EHandAfflictionSelection FWacomBattleRuleContentContract::GetCanonicalHandAfflictionSelection(
	const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_ApplyStatus_Twilight
		? EHandAfflictionSelection::AllCurrentHandCards
		: EHandAfflictionSelection::RandomUnique;
}

bool FWacomBattleRuleContentContract::CardEffectRequiresTargetZone(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectRequiresTargetZone(EffectType);
}

bool FWacomBattleRuleContentContract::CardEffectAllowsTargetZone(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectAllowsTargetZone(EffectType);
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeHandZone(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectTargetZoneMustBeHandZone(EffectType);
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeCardLocation(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectTargetZoneMustBeCardLocation(EffectType);
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeStackStatus(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectTargetZoneMustBeStackStatus(EffectType);
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeCardKeyword(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectTargetZoneMustBeCardKeyword(EffectType);
}

bool FWacomBattleRuleContentContract::CardEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::CardEffectSupportsNegativeMagnitude(EffectType);
}

bool FWacomBattleRuleContentContract::EnemyIntentEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::EnemyIntentEffectSupportsNegativeMagnitude(EffectType);
}

bool FWacomBattleRuleContentContract::EffectUsesPositiveMagnitude(const FGameplayTag& EffectType)
{
	return FEffectSemanticRegistry::EffectUsesPositiveMagnitude(EffectType);
}
