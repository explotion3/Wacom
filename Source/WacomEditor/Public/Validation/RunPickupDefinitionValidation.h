// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunPickupDefinition;

/** Shared editor validation rules for Run world pickup definition assets. */
struct WACOMEDITOR_API FWacomRunPickupDefinitionValidation
{
	static bool Validate(
		const UWacomRunPickupDefinition* PickupDefinition,
		TArray<FText>& OutErrors);
};
