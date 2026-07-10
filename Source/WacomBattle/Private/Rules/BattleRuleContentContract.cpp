// Copyright Wacom. All Rights Reserved.

#include "Rules/BattleRuleContentContract.h"

#include "Effects/EffectSemanticsRegistry.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	bool IsStatusEffect(const FGameplayTag& EffectType)
	{
		return EffectType == WacomTags::Effect_ApplyStatus_Poison
			|| EffectType == WacomTags::Effect_ApplyStatus_Slow
			|| EffectType == WacomTags::Effect_ApplyStatus_Freeze
			|| EffectType == WacomTags::Effect_ApplyStatus_Twilight;
	}

	bool IsCardCostEffect(const FGameplayTag& EffectType)
	{
		return EffectType == WacomTags::Effect_Card_AddCost
			|| EffectType == WacomTags::Effect_Card_ReduceCost;
	}

	bool IsSelectedHandCardMoveEffect(const FGameplayTag& EffectType)
	{
		return EffectType == WacomTags::Effect_Card_DiscardSelected
			|| EffectType == WacomTags::Effect_Card_ExhaustSelected;
	}

	bool IsPlayerOrEnemyPartTarget(const FGameplayTag& Target)
	{
		return Target == WacomTags::Target_Player
			|| Target == WacomTags::Target_Self
			|| Target == WacomTags::Target_SingleEnemyPart
			|| Target == WacomTags::Target_AllEnemyParts;
	}
}

bool FWacomBattleRuleContentContract::IsSupportedCardEffectType(const FGameplayTag& EffectType)
{
	const FBattleEffectSemantics* Semantics = FBattleEffectSemanticsRegistry::Find(EffectType);
	return Semantics && Semantics->bSupportedCardEffect;
}

bool FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectType(const FGameplayTag& EffectType)
{
	const FBattleEffectSemantics* Semantics = FBattleEffectSemanticsRegistry::Find(EffectType);
	return Semantics && Semantics->bSupportedEnemyIntentEffect;
}

bool FWacomBattleRuleContentContract::IsSupportedMagnitudeSource(const FGameplayTag& MagnitudeSource)
{
	return !MagnitudeSource.IsValid()
		|| MagnitudeSource == WacomTags::Magnitude_Source_Literal
		|| MagnitudeSource == WacomTags::Magnitude_Source_RuntimeCost
		|| MagnitudeSource == WacomTags::Magnitude_Source_HandCount
		|| MagnitudeSource == WacomTags::Magnitude_Source_TargetStatusStacks;
}

bool FWacomBattleRuleContentContract::IsSupportedCardEffectMagnitudeSource(
	const FGameplayTag& EffectType,
	const FGameplayTag& MagnitudeSource)
{
	if (!IsSupportedMagnitudeSource(MagnitudeSource))
	{
		return false;
	}

	if (!MagnitudeSource.IsValid() || MagnitudeSource == WacomTags::Magnitude_Source_Literal)
	{
		return true;
	}

	if (MagnitudeSource == WacomTags::Magnitude_Source_RuntimeCost)
	{
		return EffectType == WacomTags::Effect_Damage
			|| EffectType == WacomTags::Effect_ApplyStatus_Poison
			|| EffectType == WacomTags::Effect_Draw;
	}

	if (MagnitudeSource == WacomTags::Magnitude_Source_TargetStatusStacks)
	{
		return EffectType == WacomTags::Effect_Damage
			|| EffectType == WacomTags::Status_Shield
			|| IsStatusEffect(EffectType)
			|| EffectType == WacomTags::Effect_Heal
			|| EffectType == WacomTags::Effect_RemoveStatus
			|| IsCardCostEffect(EffectType)
			|| IsSelectedHandCardMoveEffect(EffectType)
			|| EffectType == WacomTags::Effect_ModifyInitiative;
	}

	return false;
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
		|| StatusTag == WacomTags::Status_Slow
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
	if (!IsSupportedCardEffectType(EffectType))
	{
		return false;
	}

	if (EffectType == WacomTags::Effect_Heal)
	{
		return Target == WacomTags::Target_Player || Target == WacomTags::Target_Self;
	}

	if (EffectType == WacomTags::Effect_Damage
		|| EffectType == WacomTags::Status_Shield
		|| IsStatusEffect(EffectType)
		|| EffectType == WacomTags::Effect_RemoveStatus)
	{
		if (Target == WacomTags::Target_SingleEnemyPart)
		{
			return Context == ECardEffectContext::PerfectRelease
				|| ((Context == ECardEffectContext::MainEffect || Context == ECardEffectContext::ZoneHookOnPlay)
					&& CardTargetMode == ECardTargetMode::SingleEnemyPart);
		}
		return IsPlayerOrEnemyPartTarget(Target);
	}

	if (EffectType == WacomTags::Effect_ModifyInitiative)
	{
		if (Target == WacomTags::Target_SingleEnemyPart)
		{
			return Context == ECardEffectContext::PerfectRelease
				|| ((Context == ECardEffectContext::MainEffect || Context == ECardEffectContext::ZoneHookOnPlay)
					&& CardTargetMode == ECardTargetMode::SingleEnemyPart);
		}
		return Target == WacomTags::Target_AllEnemyParts;
	}

	if (EffectType == WacomTags::Effect_Shuffle_Random)
	{
		return Target == WacomTags::Target_RandomHandCard;
	}
	if (EffectType == WacomTags::Effect_Shuffle_FromBothToOther)
	{
		return Target == WacomTags::Target_ZoneHandCard;
	}
	if (EffectType == WacomTags::Effect_Shuffle_ToRandomZone)
	{
		return Target == WacomTags::Target_Self;
	}

	if (IsCardCostEffect(EffectType))
	{
		if (Target == WacomTags::Target_SelectedHandCard)
		{
			return CardTargetMode == ECardTargetMode::HandCard;
		}
		return Target == WacomTags::Target_Self || Target == WacomTags::Target_LastShuffledCard;
	}

	if (IsSelectedHandCardMoveEffect(EffectType))
	{
		return Target == WacomTags::Target_SelectedHandCard
			&& CardTargetMode == ECardTargetMode::HandCard;
	}

	if (EffectType == WacomTags::Effect_Draw
		|| EffectType == WacomTags::Effect_Discard
		|| EffectType == WacomTags::Effect_ExhaustSelf)
	{
		return !Target.IsValid()
			|| Target == WacomTags::Target_Player
			|| Target == WacomTags::Target_Self;
	}

	if (EffectType == WacomTags::Effect_GainKeyword)
	{
		if (Target == WacomTags::Target_SelectedHandCard)
		{
			return CardTargetMode == ECardTargetMode::HandCard;
		}
		return Target == WacomTags::Target_LastShuffledCard;
	}

	return false;
}

bool FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectTarget(
	const FGameplayTag& EffectType,
	const FGameplayTag& Target)
{
	if (!IsSupportedEnemyIntentEffectType(EffectType))
	{
		return false;
	}

	if (EffectType == WacomTags::Effect_Damage)
	{
		return Target == WacomTags::Target_Player;
	}

	if (EffectType == WacomTags::Status_Shield)
	{
		return Target == WacomTags::Target_Self;
	}

	if (IsStatusEffect(EffectType))
	{
		return Target == WacomTags::Target_Player || Target == WacomTags::Target_Self;
	}

	return false;
}

bool FWacomBattleRuleContentContract::CardEffectRequiresTargetZone(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_Shuffle_FromBothToOther
		|| EffectType == WacomTags::Effect_GainKeyword
		|| EffectType == WacomTags::Effect_RemoveStatus;
}

bool FWacomBattleRuleContentContract::CardEffectAllowsTargetZone(const FGameplayTag& EffectType)
{
	return CardEffectRequiresTargetZone(EffectType)
		|| EffectType == WacomTags::Effect_Draw;
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeHandZone(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_Shuffle_FromBothToOther;
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeCardLocation(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_Draw;
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeStackStatus(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_RemoveStatus;
}

bool FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeCardKeyword(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_GainKeyword;
}

bool FWacomBattleRuleContentContract::CardEffectSupportsNegativeMagnitude(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_ModifyInitiative;
}

bool FWacomBattleRuleContentContract::EnemyIntentEffectSupportsNegativeMagnitude(const FGameplayTag& /*EffectType*/)
{
	return false;
}

bool FWacomBattleRuleContentContract::EffectUsesPositiveMagnitude(const FGameplayTag& EffectType)
{
	return EffectType == WacomTags::Effect_Damage
		|| EffectType == WacomTags::Status_Shield
		|| IsStatusEffect(EffectType)
		|| EffectType == WacomTags::Effect_Draw
		|| EffectType == WacomTags::Effect_Discard
		|| EffectType == WacomTags::Effect_Heal
		|| EffectType == WacomTags::Effect_RemoveStatus
		|| IsCardCostEffect(EffectType)
		|| IsSelectedHandCardMoveEffect(EffectType);
}
