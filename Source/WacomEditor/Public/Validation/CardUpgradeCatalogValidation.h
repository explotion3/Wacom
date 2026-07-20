// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCardDefinition;

/** Shared editor-only validation for immutable CardDefinition upgrade chains. */
struct WACOMEDITOR_API FWacomCardUpgradeCatalogValidation
{
	/** Appends errors for the chain reachable from CardDefinition without clearing OutErrors. */
	static void AppendReachableChainErrors(
		const UCardDefinition* CardDefinition,
		TArray<FText>& OutErrors);

	/** Validates cross-asset uniqueness, predecessor counts and connected linear families. */
	static bool Validate(
		const TArray<const UCardDefinition*>& CardDefinitions,
		TArray<FText>& OutErrors);
};
