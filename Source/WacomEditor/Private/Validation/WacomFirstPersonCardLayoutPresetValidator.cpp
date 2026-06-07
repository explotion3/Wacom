// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomFirstPersonCardLayoutPresetValidator.h"

#include "UI/Card/WacomFirstPersonCardLayoutPreset.h"
#include "Validation/FirstPersonCardLayoutPresetValidation.h"

bool UWacomFirstPersonCardLayoutPresetValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UWacomFirstPersonCardLayoutPreset>();
}

EDataValidationResult UWacomFirstPersonCardLayoutPresetValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UWacomFirstPersonCardLayoutPreset* Preset =
		Cast<UWacomFirstPersonCardLayoutPreset>(InAsset);
	if (!Preset)
	{
		return EDataValidationResult::NotValidated;
	}

	const FWacomFirstPersonCardLayoutPresetValidationReport Report =
		FWacomFirstPersonCardLayoutPresetValidation::BuildReport(Preset);
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
