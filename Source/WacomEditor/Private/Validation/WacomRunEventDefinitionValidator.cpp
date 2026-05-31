// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomRunEventDefinitionValidator.h"

#include "Events/RunEventDefinition.h"
#include "Validation/RunEventDefinitionValidation.h"

bool UWacomRunEventDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UWacomRunEventDefinition>();
}

EDataValidationResult UWacomRunEventDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UWacomRunEventDefinition* EventDefinition = Cast<UWacomRunEventDefinition>(InAsset);
	if (!EventDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	const FWacomRunEventDefinitionValidationReport Report =
		FWacomRunEventDefinitionValidation::BuildReport(EventDefinition);
	for (const FText& Warning : Report.Warnings)
	{
		AssetMessage(InAssetData, EMessageSeverity::Warning, Warning);
	}

	if (Report.IsValid())
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	for (const FText& Error : Report.Errors)
	{
		AssetMessage(InAssetData, EMessageSeverity::Error, Error);
	}
	return EDataValidationResult::Invalid;
}
