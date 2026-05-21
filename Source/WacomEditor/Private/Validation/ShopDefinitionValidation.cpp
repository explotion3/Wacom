// Copyright Wacom. All Rights Reserved.

#include "Validation/ShopDefinitionValidation.h"

#include "Shops/ShopDefinition.h"

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

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
