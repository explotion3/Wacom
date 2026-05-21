// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomShopDefinitionValidator.h"

#include "Shops/ShopDefinition.h"
#include "Validation/ShopDefinitionValidation.h"

bool UWacomShopDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UShopDefinition>();
}

EDataValidationResult UWacomShopDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UShopDefinition* ShopDefinition = Cast<UShopDefinition>(InAsset);
	if (!ShopDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomShopDefinitionValidation::Validate(ShopDefinition, Errors))
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	for (const FText& Error : Errors)
	{
		AssetMessage(InAssetData, EMessageSeverity::Error, Error);
	}
	return EDataValidationResult::Invalid;
}
