// Copyright Wacom. All Rights Reserved.

#include "Validation/ShopDefinitionValidation.h"

#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomShopDefinitionValidation"

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

}

bool FWacomShopDefinitionValidation::Validate(
	const UShopDefinition* ShopDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!ShopDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingShopDefinition", "ShopDefinition 为空。"));
		return false;
	}

	if (ShopDefinition->ShopId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingShopId", "ShopId 不能为空。"));
	}

	if (ShopDefinition->Offers.IsEmpty())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingOffers", "Offers 不能为空。"));
	}

	for (int32 Index = 0; Index < ShopDefinition->Offers.Num(); ++Index)
	{
		const FShopOfferDefinition& Offer = ShopDefinition->Offers[Index];
		const FString IndexText = FString::FromInt(Index);

		if (!Offer.CardDefinition)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("Offer {0} 缺少 CardDefinition。"), IndexText));
		}

		if (Offer.Price < 0)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("Offer {0} 的 Price 不能为负数。"), IndexText));
		}
	}

	if (ShopDefinition->CardUpgradeService.bEnabled)
	{
		if (ShopDefinition->CardUpgradeService.Prices.IsEmpty())
		{
			AddValidationError(OutErrors,
				LOCTEXT("MissingCardUpgradePrices", "启用 CardUpgradeService 时 Prices 不能为空。"));
		}

		TSet<FGameplayTag> SeenRarities;
		for (int32 Index = 0; Index < ShopDefinition->CardUpgradeService.Prices.Num(); ++Index)
		{
			const FShopCardUpgradePriceDefinition& Price =
				ShopDefinition->CardUpgradeService.Prices[Index];
			const bool bSupportedRarity =
				Price.FromRarity.MatchesTagExact(WacomTags::Card_Rarity_White)
				|| Price.FromRarity.MatchesTagExact(WacomTags::Card_Rarity_Blue)
				|| Price.FromRarity.MatchesTagExact(WacomTags::Card_Rarity_Yellow);
			if (!bSupportedRarity)
			{
				AddValidationError(OutErrors, FText::FromString(FString::Printf(
					TEXT("CardUpgradeService.Prices[%d].FromRarity 只允许 White、Blue 或 Yellow。"),
					Index)));
			}
			if (Price.Price < 0)
			{
				AddValidationError(OutErrors, FText::FromString(FString::Printf(
					TEXT("CardUpgradeService.Prices[%d].Price 不能为负数。"), Index)));
			}
			if (SeenRarities.Contains(Price.FromRarity))
			{
				AddValidationError(OutErrors, FText::FromString(FString::Printf(
					TEXT("CardUpgradeService.Prices[%d].FromRarity 重复：%s。"),
					Index, *Price.FromRarity.ToString())));
			}
			SeenRarities.Add(Price.FromRarity);
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
