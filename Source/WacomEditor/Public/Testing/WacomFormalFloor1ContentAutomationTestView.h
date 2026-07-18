// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Read-only, non-reflection view for focused WacomTests coverage. */
struct WACOMEDITOR_API FWacomFormalFloor1ContentAutomationSummary
{
	int32 TotalCount = 0;
	int32 CardCount = 0;
	int32 BehaviorCount = 0;
	int32 PartCount = 0;
	int32 EnemyCount = 0;
	int32 EncounterCount = 0;
	int32 EventCount = 0;
	int32 PickupCount = 0;
	int32 ShopCount = 0;
	int32 CardsGroupCount = 0;
	int32 EnemyGraphGroupCount = 0;
	int32 NodeDefinitionsGroupCount = 0;
	TArray<FString> PackagePaths;
	TArray<FName> StableIds;
};

struct WACOMEDITOR_API FWacomFormalFloor1ContentAutomationTestView
{
	static FWacomFormalFloor1ContentAutomationSummary GetManifestSummary();
	static bool ValidateManifest(TArray<FString>& OutErrors);
	static bool ValidateTransientDefaults(TArray<FString>& OutErrors);
	static bool ValidateComparatorBoundaries(TArray<FString>& OutErrors);
	static bool ValidateLoadedAssets(
		bool bCompareSeedDefaults,
		TArray<FString>& OutErrors);
	static bool ParseArguments(
		const TArray<FString>& Arguments,
		FString& OutCanonicalGroup,
		bool& bOutSeedMissing,
		bool& bOutCompareSeedDefaults,
		FString& OutReportPath,
		TArray<FString>& OutErrors);
};
