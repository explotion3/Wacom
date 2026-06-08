// Copyright Wacom. All Rights Reserved.

#pragma once

#include "EditorValidatorBase.h"

#include "WacomEnemyBehaviorDefinitionValidator.generated.h"

UCLASS()
class UWacomEnemyBehaviorDefinitionValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) override;
};
