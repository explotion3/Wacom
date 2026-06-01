// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomRunWorldCardInteractionDefinitionValidator.h"

#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Validation/RunWorldCardInteractionDefinitionValidation.h"

bool UWacomRunWorldCardInteractionDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UWacomRunWorldCardInteractionDefinition>();
}

EDataValidationResult UWacomRunWorldCardInteractionDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UWacomRunWorldCardInteractionDefinition* InteractionDefinition =
		Cast<UWacomRunWorldCardInteractionDefinition>(InAsset);
	if (!InteractionDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomRunWorldCardInteractionDefinitionValidation::Validate(InteractionDefinition, Errors))
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
