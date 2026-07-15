// Copyright Wacom. All Rights Reserved.

#pragma once

#include "EditorValidatorBase.h"
#include "WacomMapDefinitionValidator.generated.h"

/** Editor DataValidation bridge for Journey/Floor map assets. */
UCLASS()
class UWacomMapDefinitionValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) override;
};
