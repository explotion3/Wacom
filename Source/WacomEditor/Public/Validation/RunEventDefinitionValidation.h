// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunEventDefinition;

/** Shared validation output for lightweight RunEvent definition assets. */
struct WACOMEDITOR_API FWacomRunEventDefinitionValidationReport
{
	TArray<FText> Errors;
	TArray<FText> Warnings;

	bool HasErrors() const { return !Errors.IsEmpty(); }
	bool HasWarnings() const { return !Warnings.IsEmpty(); }
	bool IsValid() const { return Errors.IsEmpty(); }
};

/** Shared editor validation rules for lightweight RunEvent definition assets. */
struct WACOMEDITOR_API FWacomRunEventDefinitionValidation
{
	static FWacomRunEventDefinitionValidationReport BuildReport(
		const UWacomRunEventDefinition* EventDefinition);
	static bool Validate(const UWacomRunEventDefinition* EventDefinition, TArray<FText>& OutErrors);
	static bool IsValidPressureTypeId(FName PressureTypeId);
};
