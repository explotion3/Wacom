// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomEnemyDefinitionValidator.h"

#include "Enemies/EnemyDefinition.h"
#include "Validation/EnemyDefinitionValidation.h"

bool UWacomEnemyDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UEnemyDefinition>();
}

EDataValidationResult UWacomEnemyDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UEnemyDefinition* EnemyDefinition = Cast<UEnemyDefinition>(InAsset);
	if (!EnemyDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomEnemyDefinitionValidation::Validate(EnemyDefinition, Errors))
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
