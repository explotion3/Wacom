// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomEnemyBehaviorDefinitionValidator.h"

#include "Enemies/EnemyBehaviorDefinition.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"

bool UWacomEnemyBehaviorDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UEnemyBehaviorDefinition>();
}

EDataValidationResult UWacomEnemyBehaviorDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UEnemyBehaviorDefinition* BehaviorDefinition = Cast<UEnemyBehaviorDefinition>(InAsset);
	if (!BehaviorDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	TArray<FText> Errors;
	if (FWacomEnemyBehaviorDefinitionValidation::Validate(BehaviorDefinition, Errors))
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
