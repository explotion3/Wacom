// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS
struct WACOMEDITOR_API FWacomDebugShopUpgradeVerticalSliceAutomationSummary
{
	int32 ManifestCount = 0;
	int32 ExistingCount = 0;
	int32 MissingCount = 0;
	int32 RepairRequiredCount = 0;
	int32 FailedCount = 0;
	TArray<FString> PackagePaths;
	TArray<FString> Diagnostics;
};

/** 非反射、只读的 Spec020 Editor 自动化观察面。 */
struct WACOMEDITOR_API FWacomDebugShopUpgradeVerticalSliceAutomationTestView
{
	static FWacomDebugShopUpgradeVerticalSliceAutomationSummary InspectRealAssets();
	static bool ValidateManifest(TArray<FString>& OutErrors);
	static bool ValidateShopCollisionPolicyMatrix(TArray<FString>& OutErrors);
};
#endif
