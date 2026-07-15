// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Editor-private Debug builder 的非反射测试视图。 */
struct WACOMEDITOR_API FWacomRunExplorationDebugAssetBuilderAutomationSnapshot
{
	FName JourneyId = NAME_None;
	FName FloorId = NAME_None;
	TArray<FName> NodeIds;
	TArray<FName> EdgeIds;
	TArray<FString> ContentObjectPaths;
	bool bPathBlueprintsValid = false;
	bool bValidationPassed = false;
};

struct WACOMEDITOR_API FWacomRunExplorationDebugAssetBuilderAutomationTestView
{
	static bool Build(FWacomRunExplorationDebugAssetBuilderAutomationSnapshot& OutSnapshot);
};

#endif
