// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomEncounterDefinitionValidator.h"

#include "Encounters/EncounterDefinition.h"
#include "Validation/EncounterDefinitionValidation.h"

bool UWacomEncounterDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UEncounterDefinition>();
}

EDataValidationResult UWacomEncounterDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UEncounterDefinition* EncounterDefinition = Cast<UEncounterDefinition>(InAsset);
	if (!EncounterDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomEncounterDefinitionValidation::Validate(EncounterDefinition, Errors))
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
