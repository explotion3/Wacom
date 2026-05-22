// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomEnemyPartDefinitionValidator.h"

#include "Enemies/EnemyPartDefinition.h"
#include "Validation/EnemyPartDefinitionValidation.h"

bool UWacomEnemyPartDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UEnemyPartDefinition>();
}

EDataValidationResult UWacomEnemyPartDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UEnemyPartDefinition* EnemyPartDefinition = Cast<UEnemyPartDefinition>(InAsset);
	if (!EnemyPartDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomEnemyPartDefinitionValidation::Validate(EnemyPartDefinition, Errors))
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
