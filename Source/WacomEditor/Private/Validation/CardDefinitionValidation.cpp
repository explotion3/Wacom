// Copyright Wacom. All Rights Reserved.

#include "Validation/CardDefinitionValidation.h"
#include "Validation/CardUpgradeCatalogValidation.h"

#include "Cards/CardDefinition.h"
#include "Rules/BattleRuleContentContract.h"
#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A)
	{
		return FText::FromString(FString::Format(Format, { A }));
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A, const FString& B)
	{
		return FText::FromString(FString::Format(Format, { A, B }));
	}

	void ValidateCondition(
		const FEffectCondition& Condition,
		const FString& OwnerLabel,
		TArray<FText>& OutErrors)
	{
		if (!Condition.IsSet())
		{
			return;
		}

		if (!FWacomBattleRuleContentContract::IsSupportedConditionType(Condition.ConditionType))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 ConditionType 当前战斗规则未支持：{1}。"),
					OwnerLabel,
					Condition.ConditionType.ToString()));
			return;
		}

		if (Condition.ConditionType == WacomTags::Condition_Self_InZone)
		{
			if (!FWacomBattleRuleContentContract::IsHandZoneTag(Condition.ParamTag))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 ParamTag 必须是 HandZone.*。"), OwnerLabel));
			}
			return;
		}

		if (Condition.ConditionType == WacomTags::Condition_Target_HasStatus)
		{
			if (!FWacomBattleRuleContentContract::IsStackStatusTag(Condition.ParamTag))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 ParamTag 必须是可读取层数的 Status.*，不能使用 Status.Shield。"), OwnerLabel));
			}
		}
	}

	void ValidateEffectContract(
		const FCardEffect& Effect,
		const FString& EffectLabel,
		FWacomBattleRuleContentContract::ECardEffectContext Context,
		ECardTargetMode CardTargetMode,
		TArray<FText>& OutErrors)
	{
		if (!Effect.EffectType.IsValid())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 EffectType 无效。"), EffectLabel));
			return;
		}

		if (!FWacomBattleRuleContentContract::IsSupportedCardEffectType(Effect.EffectType))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 EffectType 当前战斗规则未注册：{1}。"),
					EffectLabel,
					Effect.EffectType.ToString()));
			return;
		}

		if (!FWacomBattleRuleContentContract::IsSupportedCardEffectTarget(
			Effect.EffectType,
			Effect.Target,
			Context,
			CardTargetMode))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Target 不适用于当前 Effect / TargetMode / 触发上下文：{1}。"),
					EffectLabel,
					Effect.Target.ToString()));
		}

		if (Effect.Magnitude < 0 && !FWacomBattleRuleContentContract::CardEffectSupportsNegativeMagnitude(Effect.EffectType))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Magnitude 不能为负数。"), EffectLabel));
		}

		if (FWacomBattleRuleContentContract::EffectUsesPositiveMagnitude(Effect.EffectType)
			&& Effect.Magnitude <= 0
			&& (!Effect.MagnitudeSource.IsValid()
				|| Effect.MagnitudeSource == WacomTags::Magnitude_Source_Literal))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 使用 Literal Magnitude 时必须大于 0。"), EffectLabel));
		}

		if (!FWacomBattleRuleContentContract::IsSupportedMagnitudeSource(Effect.MagnitudeSource))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 MagnitudeSource 当前战斗规则未支持：{1}。"),
					EffectLabel,
					Effect.MagnitudeSource.ToString()));
		}
		else if (!FWacomBattleRuleContentContract::IsSupportedCardEffectMagnitudeSource(Effect.EffectType, Effect.MagnitudeSource))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 MagnitudeSource 不适用于当前 EffectType：{1}。"),
					EffectLabel,
					Effect.MagnitudeSource.ToString()));
		}

		if (Effect.MagnitudeSource == WacomTags::Magnitude_Source_TargetStatusStacks
			&& !FWacomBattleRuleContentContract::IsStackStatusTag(Effect.TargetZone))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 使用 TargetStatusStacks 时 TargetZone 必须是可读取层数的 Status.*，不能使用 Status.Shield。"), EffectLabel));
		}

		if (FWacomBattleRuleContentContract::CardEffectRequiresTargetZone(Effect.EffectType)
			&& !Effect.TargetZone.IsValid())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 必须配置 TargetZone 参数。"), EffectLabel));
		}

		if (Effect.TargetZone.IsValid())
		{
			if (Effect.EffectType == WacomTags::Effect_RemoveStatus
				&& !FWacomBattleRuleContentContract::IsRemovableCombatantStatusTag(
					Effect.TargetZone))
			{
				AddValidationError(OutErrors,
					FormatValidationError(
						TEXT("{0} 的 RemoveStatus 只能移除有持久战斗单位层数的状态；敌方 Slow 是即时先机操作。"),
						EffectLabel));
			}
			else if (FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeHandZone(Effect.EffectType)
				&& !FWacomBattleRuleContentContract::IsHandZoneTag(Effect.TargetZone))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 TargetZone 必须是 HandZone.*。"), EffectLabel));
			}
			else if (FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeCardLocation(Effect.EffectType)
				&& !FWacomBattleRuleContentContract::IsCardLocationTag(Effect.TargetZone))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 TargetZone 必须是 CardLocation.*。"), EffectLabel));
			}
			else if (FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeStackStatus(Effect.EffectType)
				&& !FWacomBattleRuleContentContract::IsStackStatusTag(Effect.TargetZone))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 TargetZone 必须是可读取层数的 Status.*，不能使用 Status.Shield。"), EffectLabel));
			}
			else if (FWacomBattleRuleContentContract::CardEffectTargetZoneMustBeCardKeyword(Effect.EffectType)
				&& !FWacomBattleRuleContentContract::IsCardKeywordTag(Effect.TargetZone))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 TargetZone 必须是 Card.Keyword.*。"), EffectLabel));
			}
			else if (!FWacomBattleRuleContentContract::CardEffectAllowsTargetZone(Effect.EffectType)
				&& Effect.MagnitudeSource != WacomTags::Magnitude_Source_TargetStatusStacks)
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 当前 Effect 不读取 TargetZone 参数。"), EffectLabel));
			}
		}

		if (Effect.Duration < 0)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Duration 不能为负数。"), EffectLabel));
		}

		ValidateCondition(Effect.Condition, EffectLabel + TEXT(".Condition"), OutErrors);
		for (int32 ModifierIndex = 0; ModifierIndex < Effect.MagnitudeModifiers.Num(); ++ModifierIndex)
		{
			const FString ModifierLabel = FString::Printf(TEXT("%s.MagnitudeModifiers[%d]"), *EffectLabel, ModifierIndex);
			const FMagnitudeModifier& Modifier = Effect.MagnitudeModifiers[ModifierIndex];
			ValidateCondition(Modifier.Condition, ModifierLabel + TEXT(".Condition"), OutErrors);
		}
	}

	void ValidateEffects(
		const TArray<FCardEffect>& Effects,
		const FString& OwnerLabel,
		FWacomBattleRuleContentContract::ECardEffectContext Context,
		ECardTargetMode CardTargetMode,
		TArray<FText>& OutErrors)
	{
		for (int32 Index = 0; Index < Effects.Num(); ++Index)
		{
			ValidateEffectContract(
				Effects[Index],
				FString::Printf(TEXT("%s[%d]"), *OwnerLabel, Index),
				Context,
				CardTargetMode,
				OutErrors);
		}
	}

	bool IsCardRarityTag(const FGameplayTag& Tag)
	{
		return Tag == WacomTags::Card_Rarity_White
			|| Tag == WacomTags::Card_Rarity_Blue
			|| Tag == WacomTags::Card_Rarity_Yellow
			|| Tag == WacomTags::Card_Rarity_Purple
			|| Tag == WacomTags::Card_Rarity_Intrinsic;
	}

	bool IsZoneHookTriggerTag(const FGameplayTag& Tag)
	{
		return Tag == WacomTags::ZoneHook_Trigger_OnPlay
			|| Tag == WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;
	}

	bool IsPassiveTriggerTag(const FGameplayTag& Tag)
	{
		return Tag == WacomTags::Passive_Trigger_AfterPlayed
			|| Tag == WacomTags::Passive_Trigger_OnCompanionCount
			|| Tag == WacomTags::Passive_Trigger_OnTwilightTriggered
			|| Tag == WacomTags::Passive_Trigger_OnTurnStart
			|| Tag == WacomTags::Passive_Trigger_OnTurnEnd
			|| Tag == WacomTags::Passive_Trigger_OnDraw
			|| Tag == WacomTags::Passive_Trigger_OnDiscard;
	}

	void ValidatePassive(
		const FCardPassive& Passive,
		const FString& PassiveLabel,
		ECardTargetMode CardTargetMode,
		TArray<FText>& OutErrors)
	{
		if (!IsPassiveTriggerTag(Passive.Trigger))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Trigger 必须是 Passive.Trigger.*。"), PassiveLabel));
			return;
		}

		if (FWacomBattleRuleContentContract::IsReservedPassiveTrigger(Passive.Trigger))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Trigger 当前是保留触发点，主战斗流程未接入：{1}。"),
					PassiveLabel,
					Passive.Trigger.ToString()));
			return;
		}

		if (FWacomBattleRuleContentContract::IsEventOnlyPassiveTrigger(Passive.Trigger))
		{
			if (Passive.Condition.IsSet())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 只发事件 / 展示文本，当前不读取 Condition。"), PassiveLabel));
			}
			if (!Passive.Effects.IsEmpty())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 只发事件 / 展示文本，当前不执行 Effects。"), PassiveLabel));
			}
			return;
		}

		if (FWacomBattleRuleContentContract::IsSpecialPassiveTriggerWithoutEffects(Passive.Trigger))
		{
			ValidateCondition(Passive.Condition, PassiveLabel + TEXT(".Condition"), OutErrors);
			if (Passive.TriggerThreshold <= 0)
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 TriggerThreshold 必须大于 0。"), PassiveLabel));
			}
			if (!Passive.Effects.IsEmpty())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 是特殊回手触发，当前不执行 Effects。"), PassiveLabel));
			}
			return;
		}

		ValidateCondition(Passive.Condition, PassiveLabel + TEXT(".Condition"), OutErrors);
		ValidateEffects(
			Passive.Effects,
			PassiveLabel + TEXT(".Effects"),
			FWacomBattleRuleContentContract::ECardEffectContext::PassiveEffect,
			CardTargetMode,
			OutErrors);
	}
}

bool FWacomCardDefinitionValidation::Validate(
	const UCardDefinition* CardDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!CardDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingCardDefinition", "CardDefinition 为空。"));
		return false;
	}

	if (CardDefinition->CardId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingCardId", "CardId 不能为空。"));
	}

	if (CardDefinition->BaseCost < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeBaseCost", "BaseCost 不能为负数。"));
	}

	if (!IsCardRarityTag(CardDefinition->Rarity))
	{
		AddValidationError(OutErrors, LOCTEXT("InvalidRarity", "Rarity 必须是 Card.Rarity.*。"));
	}

	if (CardDefinition->Physique.Capacity < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeCapacity", "Physique.Capacity 不能为负数。"));
	}

	if (CardDefinition->Physique.Durability < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeDurability", "Physique.Durability 不能为负数。"));
	}

	if (CardDefinition->Physique.MaxHpBonus < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeMaxHpBonus", "Physique.MaxHpBonus 不能为负数。"));
	}

	ValidateEffects(
		CardDefinition->Effects,
		TEXT("Effects"),
		FWacomBattleRuleContentContract::ECardEffectContext::MainEffect,
		CardDefinition->TargetMode,
		OutErrors);
	ValidateEffects(
		CardDefinition->PerfectReleaseEffects,
		TEXT("PerfectReleaseEffects"),
		FWacomBattleRuleContentContract::ECardEffectContext::PerfectRelease,
		CardDefinition->TargetMode,
		OutErrors);

	for (int32 HookIndex = 0; HookIndex < CardDefinition->ZoneHooks.Num(); ++HookIndex)
	{
		const FCardZoneHook& ZoneHook = CardDefinition->ZoneHooks[HookIndex];
		const FString HookLabel = FString::Printf(TEXT("ZoneHooks[%d]"), HookIndex);

		if (!FWacomBattleRuleContentContract::IsHandZoneTag(ZoneHook.Zone))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Zone 必须是 HandZone.*。"), HookLabel));
		}

		if (!IsZoneHookTriggerTag(ZoneHook.Trigger))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Trigger 必须是 ZoneHook.Trigger.*。"), HookLabel));
		}

		if (ZoneHook.Trigger == WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit
			&& !ZoneHook.ExtraEffects.IsEmpty())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 当前只作为跳过先机推进标记，不执行 ExtraEffects。"), HookLabel));
		}

		ValidateEffects(
			ZoneHook.ExtraEffects,
			HookLabel + TEXT(".ExtraEffects"),
			FWacomBattleRuleContentContract::ECardEffectContext::ZoneHookOnPlay,
			CardDefinition->TargetMode,
			OutErrors);
	}

	for (int32 PassiveIndex = 0; PassiveIndex < CardDefinition->Passives.Num(); ++PassiveIndex)
	{
		const FCardPassive& Passive = CardDefinition->Passives[PassiveIndex];
		const FString PassiveLabel = FString::Printf(TEXT("Passives[%d]"), PassiveIndex);

		ValidatePassive(Passive, PassiveLabel, CardDefinition->TargetMode, OutErrors);
	}

	FWacomCardUpgradeCatalogValidation::AppendReachableChainErrors(CardDefinition, OutErrors);

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
