// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunWorldCardInteractionDefinition;

/** Shared editor validation rules for generic Run world card interaction definition assets. */
struct WACOMEDITOR_API FWacomRunWorldCardInteractionDefinitionValidation
{
	static bool Validate(
		const UWacomRunWorldCardInteractionDefinition* InteractionDefinition,
		TArray<FText>& OutErrors);
};
