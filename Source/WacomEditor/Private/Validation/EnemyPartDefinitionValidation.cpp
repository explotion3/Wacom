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
	TArray<FText>& OutErrors,
	EWacomEnemyPartValidationProfile Profile)
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

	const bool bHasLegacyReward = EnemyPartDefinition->KnockdownRewardCard != nullptr;
	const bool bHasAidReward = EnemyPartDefinition->AidRewardCard != nullptr;
	const bool bHasDestroyReward = EnemyPartDefinition->DestroyRewardCard != nullptr;
	if (bHasLegacyReward && (bHasAidReward || bHasDestroyReward))
	{
		AddValidationError(
			OutErrors,
			LOCTEXT(
				"MixedKnockdownRewardFields",
				"KnockdownRewardCard 不能与 AidRewardCard 或 DestroyRewardCard 混填。"));
	}

	if (Profile == EWacomEnemyPartValidationProfile::FormalProduction)
	{
		if (bHasLegacyReward)
		{
			AddValidationError(
				OutErrors,
				LOCTEXT(
					"LegacyRewardForbiddenForFormalProduction",
					"正式 Production 部位不得填写旧 KnockdownRewardCard。"));
		}
		if (!bHasAidReward)
		{
			AddValidationError(
				OutErrors,
				LOCTEXT(
					"MissingAidRewardForFormalProduction",
					"正式 Production 部位必须显式填写 AidRewardCard。"));
		}
		if (!bHasDestroyReward)
		{
			AddValidationError(
				OutErrors,
				LOCTEXT(
					"MissingDestroyRewardForFormalProduction",
					"正式 Production 部位必须显式填写 DestroyRewardCard。"));
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
