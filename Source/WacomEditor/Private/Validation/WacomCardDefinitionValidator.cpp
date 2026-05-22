// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomCardDefinitionValidator.h"

#include "Cards/CardDefinition.h"
#include "Validation/CardDefinitionValidation.h"

bool UWacomCardDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UCardDefinition>();
}

EDataValidationResult UWacomCardDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UCardDefinition* CardDefinition = Cast<UCardDefinition>(InAsset);
	if (!CardDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomCardDefinitionValidation::Validate(CardDefinition, Errors))
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
