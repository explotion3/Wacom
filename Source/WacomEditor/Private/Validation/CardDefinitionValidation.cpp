// Copyright Wacom. All Rights Reserved.

#include "Validation/CardDefinitionValidation.h"

#include "Cards/CardDefinition.h"
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

	void ValidateEffectTypes(
		const TArray<FCardEffect>& Effects,
		const FString& OwnerLabel,
		TArray<FText>& OutErrors)
	{
		for (int32 Index = 0; Index < Effects.Num(); ++Index)
		{
			if (!Effects[Index].EffectType.IsValid())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 EffectType 无效。"), FString::Printf(TEXT("%s[%d]"), *OwnerLabel, Index)));
			}
		}
	}

	bool IsCardRarityTag(const FGameplayTag& Tag)
	{
		return Tag == WacomTags::Card_Rarity_White
			|| Tag == WacomTags::Card_Rarity_Blue
			|| Tag == WacomTags::Card_Rarity_Intrinsic;
	}

	bool IsHandZoneTag(const FGameplayTag& Tag)
	{
		return Tag == WacomTags::HandZone_Left
			|| Tag == WacomTags::HandZone_Both
			|| Tag == WacomTags::HandZone_Right;
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

	ValidateEffectTypes(CardDefinition->Effects, TEXT("Effects"), OutErrors);
	ValidateEffectTypes(CardDefinition->PerfectReleaseEffects, TEXT("PerfectReleaseEffects"), OutErrors);

	for (int32 HookIndex = 0; HookIndex < CardDefinition->ZoneHooks.Num(); ++HookIndex)
	{
		const FCardZoneHook& ZoneHook = CardDefinition->ZoneHooks[HookIndex];
		const FString HookLabel = FString::Printf(TEXT("ZoneHooks[%d]"), HookIndex);

		if (!IsHandZoneTag(ZoneHook.Zone))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Zone 必须是 HandZone.*。"), HookLabel));
		}

		if (!IsZoneHookTriggerTag(ZoneHook.Trigger))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Trigger 必须是 ZoneHook.Trigger.*。"), HookLabel));
		}

		ValidateEffectTypes(ZoneHook.ExtraEffects, HookLabel + TEXT(".ExtraEffects"), OutErrors);
	}

	for (int32 PassiveIndex = 0; PassiveIndex < CardDefinition->Passives.Num(); ++PassiveIndex)
	{
		const FCardPassive& Passive = CardDefinition->Passives[PassiveIndex];
		const FString PassiveLabel = FString::Printf(TEXT("Passives[%d]"), PassiveIndex);

		if (!IsPassiveTriggerTag(Passive.Trigger))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 Trigger 必须是 Passive.Trigger.*。"), PassiveLabel));
		}

		ValidateEffectTypes(Passive.Effects, PassiveLabel + TEXT(".Effects"), OutErrors);
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
