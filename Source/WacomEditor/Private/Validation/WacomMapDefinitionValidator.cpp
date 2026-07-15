// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomMapDefinitionValidator.h"

#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Validation/WacomMapDefinitionValidation.h"

bool UWacomMapDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && (InAsset->IsA<UWacomFloorMapDefinition>() || InAsset->IsA<UWacomJourneyDefinition>());
}

EDataValidationResult UWacomMapDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	FWacomMapDefinitionValidationReport Report;
	if (const UWacomJourneyDefinition* Journey = Cast<UWacomJourneyDefinition>(InAsset))
	{
		Report = FWacomMapDefinitionValidation::ValidateJourney(Journey);
	}
	else if (const UWacomFloorMapDefinition* Floor = Cast<UWacomFloorMapDefinition>(InAsset))
	{
		Report = FWacomMapDefinitionValidation::ValidateFloor(Floor);
	}
	else
	{
		return EDataValidationResult::NotValidated;
	}

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
