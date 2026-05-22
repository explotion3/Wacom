// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCharacterDefinition;

/** Shared editor validation rules for lightweight character definition assets. */
struct WACOMEDITOR_API FWacomCharacterDefinitionValidation
{
	static bool Validate(const UCharacterDefinition* CharacterDefinition, TArray<FText>& OutErrors);
};
