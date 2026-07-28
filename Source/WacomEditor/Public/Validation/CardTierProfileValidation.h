// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCardDefinition;

/** Shared editor-only validation for single-definition four-tier card profiles. */
struct WACOMEDITOR_API FWacomCardTierProfileValidation
{
	/** Appends errors for CardDefinition tier profiles without clearing OutErrors. */
	static void AppendTierProfileErrors(
		const UCardDefinition* CardDefinition,
		TArray<FText>& OutErrors);

	/** Validates tier profile structure and cross-asset CardId uniqueness. */
	static bool Validate(
		const TArray<const UCardDefinition*>& CardDefinitions,
		TArray<FText>& OutErrors);
};
