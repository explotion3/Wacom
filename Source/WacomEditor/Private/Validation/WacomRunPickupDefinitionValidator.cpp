// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomRunPickupDefinitionValidator.h"

#include "Pickups/RunPickupDefinition.h"
#include "Validation/RunPickupDefinitionValidation.h"

bool UWacomRunPickupDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UWacomRunPickupDefinition>();
}

EDataValidationResult UWacomRunPickupDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UWacomRunPickupDefinition* PickupDefinition =
		Cast<UWacomRunPickupDefinition>(InAsset);
	if (!PickupDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomRunPickupDefinitionValidation::Validate(PickupDefinition, Errors))
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
