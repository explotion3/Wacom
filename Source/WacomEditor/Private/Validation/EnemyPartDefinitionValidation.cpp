// Copyright Wacom. All Rights Reserved.

#include "Validation/EnemyPartDefinitionValidation.h"

#include "Enemies/EnemyPartDefinition.h"

#define LOCTEXT_NAMESPACE "WacomEnemyPartDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}
}

bool FWacomEnemyPartDefinitionValidation::Validate(
	const UEnemyPartDefinition* EnemyPartDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!EnemyPartDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEnemyPartDefinition", "EnemyPartDefinition 为空。"));
		return false;
	}

	if (EnemyPartDefinition->PartId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingPartId", "PartId 不能为空。"));
	}

	if (EnemyPartDefinition->MaxHp <= 0)
	{
		AddValidationError(OutErrors, LOCTEXT("InvalidMaxHp", "MaxHp 必须大于 0。"));
	}

	if (EnemyPartDefinition->ExperienceReward < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeExperienceReward", "ExperienceReward 不能为负数。"));
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
