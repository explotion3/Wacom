// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomRunKeyChestDefinitionValidator.h"

#include "KeyChests/RunKeyChestDefinition.h"
#include "Validation/RunKeyChestDefinitionValidation.h"

bool UWacomRunKeyChestDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UWacomRunKeyChestDefinition>();
}

EDataValidationResult UWacomRunKeyChestDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UWacomRunKeyChestDefinition* KeyChestDefinition =
		Cast<UWacomRunKeyChestDefinition>(InAsset);
	if (!KeyChestDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomRunKeyChestDefinitionValidation::Validate(KeyChestDefinition, Errors))
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
