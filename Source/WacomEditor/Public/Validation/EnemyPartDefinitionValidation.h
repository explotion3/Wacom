// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEnemyPartDefinition;

/** Shared editor validation rules for lightweight enemy part definition assets. */
struct WACOMEDITOR_API FWacomEnemyPartDefinitionValidation
{
	static bool Validate(const UEnemyPartDefinition* EnemyPartDefinition, TArray<FText>& OutErrors);
};
