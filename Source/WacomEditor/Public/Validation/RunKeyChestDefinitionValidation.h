// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunKeyChestDefinition;

/** Shared editor validation rules for Run world key chest definition assets. */
struct WACOMEDITOR_API FWacomRunKeyChestDefinitionValidation
{
	static bool Validate(
		const UWacomRunKeyChestDefinition* KeyChestDefinition,
		TArray<FText>& OutErrors);
};
