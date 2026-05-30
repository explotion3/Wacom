// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardPresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"

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

	int32 GetDisplayMagnitude(const FCardEffect& Effect, const UCardDefinition* Card)
	{
		if (Effect.MagnitudeSource.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost))
		{
			return Card ? Card->BaseCost : Effect.Magnitude;
		}
		return Effect.Magnitude;
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
		default:
			return FText::AsNumber(Value);
		}
	}

	bool TryBuildEffectBadge(const FCardEffect& Effect, const UCardDefinition* Card, FWacomCardViewEffectBadge& OutBadge)
	{
		const int32 Value = GetDisplayMagnitude(Effect, Card);
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
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Poison;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Slow;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Freeze;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Twilight;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Draw))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Draw;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Discard))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Discard;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_DiscardSelected))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Discard;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Generic;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ModifyInitiative))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Initiative;
		}
		else if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost)
			|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
		{
			OutBadge.Kind = EWacomCardViewEffectBadgeKind::Cost;
		}
		else
		{
			return false;
		}

		OutBadge.Value = Value;
		OutBadge.DisplayText = BuildEffectBadgeText(OutBadge.Kind, Value);
		return true;
	}

	FText BuildPassiveTriggerText(const FCardPassive& Passive)
	{
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_AfterPlayed))
		{
			return LOCTEXT("PassiveTriggerAfterPlayed", "被动：打出后");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
		{
			return Passive.TriggerThreshold > 0
				? FText::Format(LOCTEXT("PassiveTriggerOnCompanionCountFmt", "被动：每打出{0}张伙伴"), FText::AsNumber(Passive.TriggerThreshold))
				: LOCTEXT("PassiveTriggerOnCompanionCount", "被动：打出伙伴计数");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered))
		{
			return LOCTEXT("PassiveTriggerOnTwilightTriggered", "被动：暮气触发");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnStart))
		{
			return LOCTEXT("PassiveTriggerOnTurnStart", "被动：回合开始");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnEnd))
		{
			return LOCTEXT("PassiveTriggerOnTurnEnd", "被动：回合结束");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDraw))
		{
			return LOCTEXT("PassiveTriggerOnDraw", "被动：抽到时");
		}
		if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDiscard))
		{
			return LOCTEXT("PassiveTriggerOnDiscard", "被动：弃掉时");
		}

		return Passive.Trigger.IsValid()
			? FText::FromString(FString::Printf(TEXT("被动：%s"), *ShortGameplayTagName(Passive.Trigger)))
			: LOCTEXT("PassiveTriggerUnknown", "被动");
	}

	FText BuildPassiveLine(const FCardPassive& Passive)
	{
		FText TriggerText = BuildPassiveTriggerText(Passive);
		if (Passive.Effects.Num() <= 0)
		{
			return TriggerText;
		}

		return FText::Format(
			LOCTEXT("PassiveLineWithEffectCountFmt", "{0}（效果 {1}）"),
			TriggerText,
			FText::AsNumber(Passive.Effects.Num()));
	}
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(const UCardDefinition* Card)
{
	FWacomCardViewData Data;
	Data.Name = GetCardDisplayName(Card);
	Data.TypeText = BuildTypeLine(Card);
	Data.Description = BuildCompactDescriptionText(Card);
	Data.Cost = Card ? Card->BaseCost : 0;
	Data.bShowCost = Card != nullptr;
	Data.Value = GetDeleteValueFromRarity(Card);
	Data.bShowValue = Data.Value > 0;
	Data.PhysiqueText = BuildPhysiqueText(Card);
	Data.bShowPhysique = !Data.PhysiqueText.IsEmpty();
	Data.EffectBadges = BuildEffectBadges(Card);
	return Data;
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(const UCardDefinition* Card)
{
	FWacomCardDetailViewData Data;
	Data.Name = GetCardDisplayName(Card);
	if (!Card)
	{
		return Data;
	}

	Data.Description = Card->Description;
	for (const FCardPassive& Passive : Card->Passives)
	{
		Data.PassiveLines.Add(Passive.DisplayText.IsEmpty() ? BuildPassiveLine(Passive) : Passive.DisplayText);
	}
	return Data;
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(const UCardDefinition* Card)
{
	TArray<FWacomCardViewEffectBadge> Badges;
	if (!Card)
	{
		return Badges;
	}

	for (const FCardEffect& Effect : Card->Effects)
	{
		FWacomCardViewEffectBadge Badge;
		if (TryBuildEffectBadge(Effect, Card, Badge))
		{
			Badges.Add(Badge);
		}
	}
	return Badges;
}

#undef LOCTEXT_NAMESPACE
