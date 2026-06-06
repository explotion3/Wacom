// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEncounterDefinition;

/** Shared editor validation rules for Encounter definition assets. */
struct WACOMEDITOR_API FWacomEncounterDefinitionValidation
{
	static bool Validate(const UEncounterDefinition* EncounterDefinition, TArray<FText>& OutErrors);
};
