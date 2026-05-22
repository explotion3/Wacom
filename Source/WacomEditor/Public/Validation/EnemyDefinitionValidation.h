// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEnemyDefinition;

/** Shared editor validation rules for lightweight enemy definition assets. */
struct WACOMEDITOR_API FWacomEnemyDefinitionValidation
{
	static bool Validate(const UEnemyDefinition* EnemyDefinition, TArray<FText>& OutErrors);
};
