// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopUpgradePresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomShopUpgradePresentationBuilder"

namespace
{
FText CardName(const UCardDefinition* Card)
{
	if (!Card)
	{
		return LOCTEXT("MissingCard", "未知卡牌");
	}
	return Card->DisplayName.IsEmpty() ? FText::FromName(Card->CardId) : Card->DisplayName;
}

FText RarityName(const FGameplayTag& Rarity)
{
	const FString FullName = Rarity.ToString();
	int32 DotIndex = INDEX_NONE;
	return FullName.FindLastChar(TEXT('.'), DotIndex)
		? FText::FromString(FullName.Mid(DotIndex + 1))
		: FText::FromString(FullName);
}

FText EffectName(const FGameplayTag& EffectType)
{
	if (EffectType == WacomTags::Effect_Damage)
	{
		return LOCTEXT("Damage", "伤害");
	}
	if (EffectType == WacomTags::Effect_ApplyStatus_Poison)
	{
		return LOCTEXT("Poison", "中毒");
	}
	if (EffectType == WacomTags::Status_Shield)
	{
		return LOCTEXT("Shield", "护盾");
	}
	if (EffectType == WacomTags::Effect_Heal)
	{
		return LOCTEXT("Heal", "治疗");
	}
	return FText::FromString(EffectType.ToString());
}

void AppendEffectChanges(
	const TArray<FCardEffect>& CurrentEffects,
	const TArray<FCardEffect>& NextEffects,
	TArray<FText>& OutLines)
{
	const int32 Count = FMath::Min(CurrentEffects.Num(), NextEffects.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FCardEffect& Current = CurrentEffects[Index];
		const FCardEffect& Next = NextEffects[Index];
		if (Current.Magnitude != Next.Magnitude)
		{
			OutLines.Add(FText::Format(
				LOCTEXT("MagnitudeChange", "{0}：{1} → {2}"),
				EffectName(Current.EffectType),
				FText::AsNumber(Current.Magnitude),
				FText::AsNumber(Next.Magnitude)));
		}
		if (Current.Duration != Next.Duration)
		{
			OutLines.Add(FText::Format(
				LOCTEXT("DurationChange", "{0}持续：{1} → {2}"),
				EffectName(Current.EffectType),
				FText::AsNumber(Current.Duration),
				FText::AsNumber(Next.Duration)));
		}
	}
}

FText BuildChangeSummary(
	const UCardDefinition* Definition,
	EWacomCardUpgradeTier CurrentTier,
	EWacomCardUpgradeTier NextTier)
{
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	TArray<FText> Lines;
	const FGameplayTag CurrentRarity = Definition->ResolveRarity(CurrentTier);
	const FGameplayTag NextRarity = Definition->ResolveRarity(NextTier);
	if (CurrentRarity != NextRarity)
	{
		Lines.Add(FText::Format(
			LOCTEXT("RarityChange", "稀有度：{0} → {1}"),
			RarityName(CurrentRarity),
			RarityName(NextRarity)));
	}
	const int32 CurrentCost = Definition->ResolveBaseCost(CurrentTier);
	const int32 NextCost = Definition->ResolveBaseCost(NextTier);
	if (CurrentCost != NextCost)
	{
		Lines.Add(FText::Format(
			LOCTEXT("CostChange", "费用：{0} → {1}"),
			FText::AsNumber(CurrentCost),
			FText::AsNumber(NextCost)));
	}
	AppendEffectChanges(
		Definition->ResolveEffects(CurrentTier),
		Definition->ResolveEffects(NextTier),
		Lines);
	AppendEffectChanges(
		Definition->ResolvePerfectReleaseEffects(CurrentTier),
		Definition->ResolvePerfectReleaseEffects(NextTier),
		Lines);

	FString Joined;
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (Index > 0)
		{
			Joined += TEXT("\n");
		}
		Joined += Lines[Index].ToString();
	}
	return FText::FromString(Joined);
}

void ApplyNextCardValueEmphasis(
	const FWacomCardViewData& Current,
	FWacomCardViewData& Next)
{
	for (FWacomCardViewEffectBadge& NextBadge : Next.EffectBadges)
	{
		const FWacomCardViewEffectBadge* CurrentBadge = Current.EffectBadges.FindByPredicate(
			[&NextBadge](const FWacomCardViewEffectBadge& Candidate)
			{
				return Candidate.PresentationKey == NextBadge.PresentationKey;
			});
		if (!CurrentBadge)
		{
			NextBadge.ValueEmphasis = EWacomCardViewValueEmphasis::Increased;
		}
		else if (NextBadge.Value > CurrentBadge->Value)
		{
			NextBadge.ValueEmphasis = EWacomCardViewValueEmphasis::Increased;
		}
		else if (NextBadge.Value < CurrentBadge->Value)
		{
			NextBadge.ValueEmphasis = EWacomCardViewValueEmphasis::Decreased;
		}
	}
}
}

FWacomShopCardUpgradePresentationView
UWacomShopUpgradePresentationBuilder::BuildUpgradePresentationView(
	const FRunShopCardUpgradeQuote& Quote)
{
	FWacomShopCardUpgradePresentationView View;
	View.InstanceId = Quote.InstanceId;
	View.Definition = Quote.Definition;
	View.CurrentTier = Quote.CurrentTier;
	View.NextTier = Quote.NextTier;
	FWacomCardPresentationRuntimeContext CurrentContext;
	CurrentContext.bHasUpgradeTier = true;
	CurrentContext.UpgradeTier = Quote.CurrentTier;
	FWacomCardPresentationRuntimeContext NextContext;
	NextContext.bHasUpgradeTier = true;
	NextContext.UpgradeTier = Quote.NextTier;
	View.CurrentCardViewData = UWacomCardPresentationBuilder::BuildCardViewData(
		Quote.Definition,
		CurrentContext);
	View.NextCardViewData = UWacomCardPresentationBuilder::BuildCardViewData(
		Quote.Definition,
		NextContext);
	ApplyNextCardValueEmphasis(View.CurrentCardViewData, View.NextCardViewData);
	View.CurrentCardNameText = CardName(Quote.Definition);
	View.NextCardNameText = CardName(Quote.Definition);
	View.PriceText = FText::Format(LOCTEXT("Price", "{0} 金币"), FText::AsNumber(Quote.Price));
	View.ActionText = FText::Format(
		LOCTEXT("UpgradeAction", "支付 {0} 金币并强化"),
		FText::AsNumber(Quote.Price));
	View.bCanUpgrade = Quote.bCanUpgrade;
	View.DisabledReason = Quote.DisabledReason;
	View.StatusText = Quote.bCanUpgrade ? FText::GetEmpty() : BuildUpgradeFailureText(Quote.DisabledReason);
	View.ChangeSummaryText = BuildChangeSummary(Quote.Definition, Quote.CurrentTier, Quote.NextTier);
	return View;
}

TArray<FWacomShopCardUpgradePresentationView>
UWacomShopUpgradePresentationBuilder::BuildUpgradePresentationViews(
	const FRunShopSnapshot& Snapshot)
{
	TArray<FWacomShopCardUpgradePresentationView> Views;
	for (const FRunShopCardUpgradeQuote& Quote : Snapshot.CardUpgradeQuotes)
	{
		// 只有采用完整四阶 Profile 且尚有下一阶的实例进入强化列表。
		EWacomCardUpgradeTier IgnoredNextTier = EWacomCardUpgradeTier::White;
		if (Quote.Definition
			&& Quote.Definition->UsesTierProfiles()
			&& WacomCardUpgrade::TryGetNext(Quote.CurrentTier, IgnoredNextTier))
		{
			Views.Add(BuildUpgradePresentationView(Quote));
		}
	}
	return Views;
}

FText UWacomShopUpgradePresentationBuilder::BuildUpgradeFailureText(FName DisabledReason)
{
	if (DisabledReason == TEXT("InsufficientGold"))
	{
		return LOCTEXT("InsufficientGold", "金币不足");
	}
	if (DisabledReason == TEXT("StaleDefinition")
		|| DisabledReason == TEXT("StaleCurrentTier")
		|| DisabledReason == TEXT("CardLocationChanged"))
	{
		return LOCTEXT("StaleQuote", "卡牌状态已变化，请重新选择");
	}
	if (DisabledReason == TEXT("UpgradePriceMissing"))
	{
		return LOCTEXT("PriceMissing", "商店未配置该稀有度的强化价格");
	}
	if (DisabledReason == TEXT("CardUpgradeServiceDisabled"))
	{
		return LOCTEXT("ServiceDisabled", "该商店未开放强化服务");
	}
	if (DisabledReason == TEXT("NoNextUpgrade"))
	{
		return LOCTEXT("NoNext", "已达到当前强化上限");
	}
	if (DisabledReason == TEXT("CardNotOwned")
		|| DisabledReason == TEXT("InvalidCardInstanceId"))
	{
		return LOCTEXT("NotOwned", "找不到该实体卡");
	}
	if (DisabledReason == TEXT("RunNotActive")
		|| DisabledReason == TEXT("ShopVisitNotActive"))
	{
		return LOCTEXT("VisitInactive", "当前商店访问已结束");
	}
	return LOCTEXT("UpgradeFailed", "强化失败");
}

FText UWacomShopUpgradePresentationBuilder::BuildUpgradeSuccessText(
	const UCardDefinition* Definition,
	EWacomCardUpgradeTier PreviousTier,
	EWacomCardUpgradeTier NewTier)
{
	return FText::Format(
		LOCTEXT("UpgradeSucceeded", "已强化：{0}（{1} → {2}）"),
		CardName(Definition),
		RarityName(Definition ? Definition->ResolveRarity(PreviousTier) : FGameplayTag()),
		RarityName(Definition ? Definition->ResolveRarity(NewTier) : FGameplayTag()));
}

#undef LOCTEXT_NAMESPACE
