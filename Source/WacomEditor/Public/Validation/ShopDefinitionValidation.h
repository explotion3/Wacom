// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UShopDefinition;

/** Shared editor validation rules for static shop definition assets. */
struct WACOMEDITOR_API FWacomShopDefinitionValidation
{
	static bool Validate(const UShopDefinition* ShopDefinition, TArray<FText>& OutErrors);
};
