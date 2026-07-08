// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace WacomCardExplanationText
{
	FString GetDisplayTagLeafName(const FGameplayTag& Tag);
	FText GetDisplayHandZoneName(const FGameplayTag& HandZoneTag);
	FText GetDisplayStatusName(const FGameplayTag& StatusTag);
}
