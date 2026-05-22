// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomCharacterDefinitionValidator.h"

#include "Characters/CharacterDefinition.h"
#include "Validation/CharacterDefinitionValidation.h"

bool UWacomCharacterDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UCharacterDefinition>();
}

EDataValidationResult UWacomCharacterDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UCharacterDefinition* CharacterDefinition = Cast<UCharacterDefinition>(InAsset);
	if (!CharacterDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomCharacterDefinitionValidation::Validate(CharacterDefinition, Errors))
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
