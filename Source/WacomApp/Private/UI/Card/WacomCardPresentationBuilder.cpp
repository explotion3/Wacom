// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardPresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "WacomCardDetailDocumentBuilder.h"

#define LOCTEXT_NAMESPACE "WacomCardPresentationBuilder"

namespace
{
	FText GetCardDisplayName(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return LOCTEXT("UnknownCardName", "未知卡牌");
		}
		return Card->DisplayName.IsEmpty()
			? FText::FromName(Card->CardId)
			: Card->DisplayName;
	}

	FString ShortGameplayTagName(const FGameplayTag& Tag)
	{
		FString TagName = Tag.GetTagName().ToString();
		int32 LastDot = INDEX_NONE;
		TagName.FindLastChar(TEXT('.'), LastDot);
		return LastDot != INDEX_NONE ? TagName.Mid(LastDot + 1) : TagName;
	}

	FString GetKeywordDisplayName(const FGameplayTag& Tag)
	{
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Swift))          { return TEXT("迅捷"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Retain))         { return TEXT("保留"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Combo))          { return TEXT("连击"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Companion))      { return TEXT("伙伴"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Weapon))         { return TEXT("武器"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Tool))           { return TEXT("工具"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Hand))           { return TEXT("手"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_Exhaust))        { return TEXT("消耗"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_BagProvider))    { return TEXT("容器"); }
		if (Tag.MatchesTagExact(WacomTags::Card_Keyword_DeleteProvider)) { return TEXT("删牌"); }
		return ShortGameplayTagName(Tag);
	}

	FText BuildTypeLine(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return FText::GetEmpty();
		}

		if (Card->Physique.Capacity > 0)
		{
			return Card->Physique.CapacityEffect.IsValid()
				? LOCTEXT("TypeContainerB", "容器")
				: LOCTEXT("TypeContainerA", "背包");
		}

		TArray<FString> KeywordNames;
		for (const FGameplayTag& Tag : Card->Keywords)
		{
			KeywordNames.Add(GetKeywordDisplayName(Tag));
		}

		return KeywordNames.Num() > 0
			? FText::FromString(FString::Join(KeywordNames, TEXT(" / ")))
			: FText::GetEmpty();
	}

	FText BuildCompactDescriptionText(const UCardDefinition* Card)
	{
		if (!Card || Card->Description.IsEmpty())
		{
			return FText::GetEmpty();
		}

		FString Text = Card->Description.ToString();
		Text.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		Text.ReplaceInline(TEXT("\r"), TEXT("\n"));

		int32 FirstBreak = INDEX_NONE;
		if (Text.FindChar(TEXT('\n'), FirstBreak))
		{
			Text = Text.Left(FirstBreak);
		}

		constexpr int32 MaxChars = 28;
		if (Text.Len() > MaxChars)
		{
			Text = Text.Left(MaxChars).TrimEnd() + TEXT("...");
		}

		return FText::FromString(Text);
	}

	int32 GetDeleteValueFromRarity(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return 0;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_White))
		{
			return 1;
		}
		if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Blue))
		{
			return 2;
		}
		return 0;
	}

	FText BuildPhysiqueText(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return FText::GetEmpty();
		}

		TArray<FString> Parts;
		if (Card->Physique.Durability > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d耐久"), Card->Physique.Durability));
		}
		if (Card->Physique.Capacity > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d容量"), Card->Physique.Capacity));
		}
		if (Card->Physique.MaxHpBonus > 0)
		{
			Parts.Add(FString::Printf(TEXT("+%d生命"), Card->Physique.MaxHpBonus));
		}

		return Parts.Num() > 0
			? FText::FromString(FString::Join(Parts, TEXT("/")))
			: FText::GetEmpty();
	}

	bool UsesRuntimeCostMagnitude(const FCardEffect& Effect)
	{
		return Effect.MagnitudeSource.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost)
			|| (!Effect.MagnitudeSource.IsValid() && Effect.bMagnitudeFromRuntimeCost);
	}

	const FWacomCardPresentationRuntimeContext::FEffectPreview* FindEffectPreview(
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex)
	{
		for (const FWacomCardPresentationRuntimeContext::FEffectPreview& Preview : RuntimeContext.EffectPreviews)
		{
			if (Preview.EffectIndex == EffectIndex)
			{
				return &Preview;
			}
		}
		return nullptr;
	}

	int32 GetBaseDisplayMagnitude(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext)
	{
		if (UsesRuntimeCostMagnitude(Effect))
		{
			if (RuntimeContext.bHasRuntimeCost)
			{
				return RuntimeContext.RuntimeCost;
			}
			return Card ? Card->BaseCost : Effect.Magnitude;
		}
		return Effect.Magnitude;
	}

	int32 GetPreviewDisplayMagnitude(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex)
	{
		if (const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex))
		{
			if (Preview->bHasMagnitude)
			{
				return Preview->Magnitude;
			}
		}

		return GetBaseDisplayMagnitude(Effect, Card, RuntimeContext);
	}

	FText BuildEffectBadgeText(EWacomCardViewEffectBadgeKind Kind, int32 Value)
	{
		switch (Kind)
		{
		case EWacomCardViewEffectBadgeKind::Damage:
			return FText::Format(LOCTEXT("DamageBadgeFmt", "伤{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Heal:
			return FText::Format(LOCTEXT("HealBadgeFmt", "疗{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Poison:
			return FText::Format(LOCTEXT("PoisonBadgeFmt", "毒{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Burn:
			return FText::Format(LOCTEXT("BurnBadgeFmt", "灼{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Slow:
			return FText::Format(LOCTEXT("SlowBadgeFmt", "缓{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Freeze:
			return FText::Format(LOCTEXT("FreezeBadgeFmt", "冻{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Twilight:
			return FText::Format(LOCTEXT("TwilightBadgeFmt", "暮{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Draw:
			return FText::Format(LOCTEXT("DrawBadgeFmt", "抽{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Discard:
			return FText::Format(LOCTEXT("DiscardBadgeFmt", "弃{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Initiative:
			return FText::Format(LOCTEXT("InitiativeBadgeFmt", "机{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Cost:
			return FText::Format(LOCTEXT("CostBadgeFmt", "费{0}"), FText::AsNumber(Value));
		case EWacomCardViewEffectBadgeKind::Shield:
			return FText::Format(LOCTEXT("ShieldBadgeFmt", "盾{0}"), FText::AsNumber(Value));
		default:
			return FText::AsNumber(Value);
		}
	}

	bool IsArtBackedCardFaceEffectBadgeKind(EWacomCardViewEffectBadgeKind Kind)
	{
		switch (Kind)
		{
		case EWacomCardViewEffectBadgeKind::Damage:
		case EWacomCardViewEffectBadgeKind::Poison:
		case EWacomCardViewEffectBadgeKind::Burn:
		case EWacomCardViewEffectBadgeKind::Heal:
		case EWacomCardViewEffectBadgeKind::Shield:
			return true;
		default:
			return false;
		}
	}

	bool TryBuildEffectBadge(
		const FCardEffect& Effect,
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		int32 EffectIndex,
		FWacomCardViewEffectBadge& OutBadge)
	{
		if (const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex))
		{
			if (Preview->bSkip)
			{
				return false;
			}
		}

		const int32 Value = GetPreviewDisplayMagnitude(Effect, Card, RuntimeContext, EffectIndex);
		if (Value == 0)
		{
			return false;
		}

		if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Damage;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Heal;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Shield;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Poison;
		}
		else
		{
			return false;
		}

		if (!IsArtBackedCardFaceEffectBadgeKind(OutBadge.Kind))
		{
			return false;
		}

		OutBadge.Value = Value;
		OutBadge.DisplayText = BuildEffectBadgeText(OutBadge.Kind, Value);
		return true;
	}

}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(const UCardDefinition* Card)
{
	return BuildCardViewData(Card, FWacomCardPresentationRuntimeContext());
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	FWacomCardViewData Data;
	Data.Name = GetCardDisplayName(Card);
	Data.TypeText = BuildTypeLine(Card);
	Data.Description = BuildCompactDescriptionText(Card);
	Data.Cost = RuntimeContext.bHasRuntimeCost ? RuntimeContext.RuntimeCost : (Card ? Card->BaseCost : 0);
	Data.bShowCost = Card != nullptr;
	Data.Rarity = Card ? Card->Rarity : FGameplayTag();
	Data.Value = GetDeleteValueFromRarity(Card);
	Data.bShowValue = Data.Value > 0;
	Data.PhysiqueText = BuildPhysiqueText(Card);
	Data.bShowPhysique = !Data.PhysiqueText.IsEmpty();
	if (Card)
	{
		int32 Effective = Card->Physique.Durability;
		if (Effective == 0) Effective = Card->Physique.MaxHpBonus;
		Data.Durability = Effective;
		Data.bShowDurability = Effective > 0;
	}
	else
	{
		Data.Durability = 0;
		Data.bShowDurability = false;
	}
	Data.EffectBadges = BuildEffectBadges(Card, RuntimeContext);
	if (RuntimeContext.bHasPlayableState)
	{
		Data.bDisabled = !RuntimeContext.bIsPlayable;
	}
	return Data;
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(const UCardDefinition* Card)
{
	return BuildCardDetailViewData(Card, FWacomCardPresentationRuntimeContext());
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardDetailDocumentBuilder::BuildCardDetailViewData(Card, RuntimeContext);
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(const UCardDefinition* Card)
{
	return BuildEffectBadges(Card, FWacomCardPresentationRuntimeContext());
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	TArray<FWacomCardViewEffectBadge> Badges;
	if (!Card)
	{
		return Badges;
	}

	for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
	{
		const FCardEffect& Effect = Card->Effects[EffectIndex];
		FWacomCardViewEffectBadge Badge;
		if (TryBuildEffectBadge(Effect, Card, RuntimeContext, EffectIndex, Badge))
		{
			Badges.Add(Badge);
		}
	}
	return Badges;
}

#undef LOCTEXT_NAMESPACE
