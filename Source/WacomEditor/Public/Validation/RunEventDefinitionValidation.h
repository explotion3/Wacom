// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunEventDefinition;

/** Shared editor validation rules for lightweight RunEvent definition assets. */
struct WACOMEDITOR_API FWacomRunEventDefinitionValidation
{
	static bool Validate(const UWacomRunEventDefinition* EventDefinition, TArray<FText>& OutErrors);
	static bool IsValidPressureTypeId(FName PressureTypeId);
};
