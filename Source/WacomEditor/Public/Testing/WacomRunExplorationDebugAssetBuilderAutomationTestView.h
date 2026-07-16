// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Editor-private Debug builder 的非反射测试快照。 */
struct WACOMEDITOR_API FWacomRunExplorationDebugAssetBuilderAutomationSnapshot
{
	FName JourneyId = NAME_None;
	FName FloorId = NAME_None;
	TArray<FName> NodeIds;
	TArray<FName> EdgeIds;
	TArray<FString> ContentObjectPaths;
	TArray<FName> AnchorNodeIds;
	TArray<FName> PathEdgeIds;
	TArray<FName> BranchEdgeIds;
	TArray<FName> HostNodeIds;
	FString DescriptorFloorPath;
	FString GameModeJourneyPath;
	int32 DescriptorCount = 0;
	bool bSharedBlueprintsValid = false;
	bool bDataValidationPassed = false;
	bool bSceneValidationPassed = false;
	bool bOwnedPackagesClean = false;
};

struct WACOMEDITOR_API FWacomRunExplorationDebugAssetBuilderAutomationTestView
{
	static bool Build(
		FWacomRunExplorationDebugAssetBuilderAutomationSnapshot& OutSnapshot);

	static bool BuildWithSharedBlueprintOverrides(
		const FString& PlayerBlueprintObjectPath,
		const FString& AnchorBlueprintObjectPath,
		const FString& PathBlueprintObjectPath,
		const FString& BranchBlueprintObjectPath);
};

#endif
