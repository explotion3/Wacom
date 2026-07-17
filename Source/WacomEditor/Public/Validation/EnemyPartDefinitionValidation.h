// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEnemyPartDefinition;

/** Enemy-part reward validation strictness. */
enum class EWacomEnemyPartValidationProfile : uint8
{
	/** Allows no reward, legacy-only assets, or explicit branch fields. */
	General,
	/** Requires both explicit branch fields and forbids the legacy field. */
	FormalProduction,
};

/** Shared editor validation rules for lightweight enemy part definition assets. */
struct WACOMEDITOR_API FWacomEnemyPartDefinitionValidation
{
	static bool Validate(
		const UEnemyPartDefinition* EnemyPartDefinition,
		TArray<FText>& OutErrors,
		EWacomEnemyPartValidationProfile Profile =
			EWacomEnemyPartValidationProfile::General);
};
