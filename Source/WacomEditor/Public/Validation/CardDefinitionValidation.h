// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCardDefinition;

/** Shared editor validation rules for lightweight card definition assets. */
struct WACOMEDITOR_API FWacomCardDefinitionValidation
{
	static bool Validate(const UCardDefinition* CardDefinition, TArray<FText>& OutErrors);
};
