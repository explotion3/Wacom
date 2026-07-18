// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Read-only, non-reflection view for focused WacomTests coverage. */
struct WACOMEDITOR_API FWacomFormalFloor1ProductionSceneAutomationSummary
{
	int32 TotalCount = 0;
	int32 FloorAssetCount = 0;
	int32 BlueprintAssetCount = 0;
	int32 WorldAssetCount = 0;
	int32 FloorGroupCount = 0;
	int32 EnemyHostsGroupCount = 0;
	int32 SceneGroupCount = 0;
	TArray<FString> PackagePaths;
	TArray<FName> StableIds;
};

struct WACOMEDITOR_API FWacomFormalFloor1ProductionSceneAutomationTestView
{
	static FWacomFormalFloor1ProductionSceneAutomationSummary GetManifestSummary();
	static bool ValidateManifest(TArray<FString>& OutErrors);
	static bool ValidateTransientFloor(TArray<FString>& OutErrors);
	static bool ParseArguments(
		const TArray<FString>& Arguments,
		FString& OutCanonicalGroup,
		bool& bOutSeedMissing,
		bool& bOutCompareSeedDefaults,
		FString& OutReportPath,
		TArray<FString>& OutErrors);
};
