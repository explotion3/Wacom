// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEnemyBehaviorDefinition;
class UEnemyDefinition;

/** Shared editor validation rules for enemy behavior definition assets. */
struct WACOMEDITOR_API FWacomEnemyBehaviorDefinitionValidation
{
	static bool Validate(
		const UEnemyBehaviorDefinition* BehaviorDefinition,
		TArray<FText>& OutErrors,
		const UEnemyDefinition* OwningEnemyDefinition = nullptr);
};
