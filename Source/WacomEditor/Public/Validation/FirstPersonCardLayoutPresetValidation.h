// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomFirstPersonCardLayoutPreset;

/** Shared validation output for first-person card layout preset assets. */
struct WACOMEDITOR_API FWacomFirstPersonCardLayoutPresetValidationReport
{
	TArray<FText> Errors;
	TArray<FText> Warnings;

	bool HasErrors() const { return !Errors.IsEmpty(); }
	bool HasWarnings() const { return !Warnings.IsEmpty(); }
	bool IsValid() const { return Errors.IsEmpty(); }
};

/** Shared editor validation rules for first-person card layout preset assets. */
struct WACOMEDITOR_API FWacomFirstPersonCardLayoutPresetValidation
{
	static FWacomFirstPersonCardLayoutPresetValidationReport BuildReport(
		const UWacomFirstPersonCardLayoutPreset* Preset);
	static bool Validate(const UWacomFirstPersonCardLayoutPreset* Preset, TArray<FText>& OutErrors);
};
