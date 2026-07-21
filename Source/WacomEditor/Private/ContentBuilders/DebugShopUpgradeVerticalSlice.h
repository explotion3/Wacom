// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Wacom::ContentBuilder
{
	struct FDebugShopUpgradeVerticalSlicePassReport
	{
		int32 CreatedCount = 0;
		int32 ModifiedCount = 0;
		int32 ExistingCount = 0;
		int32 SavedCount = 0;
		int32 FailedCount = 0;
		TArray<FString> SavedPackages;
		TArray<FString> Diagnostics;

		bool IsOk() const { return FailedCount == 0; }
		bool IsIdempotent() const
		{
			return IsOk() && CreatedCount == 0 && ModifiedCount == 0 && SavedCount == 0;
		}
	};

	struct FDebugShopUpgradeVerticalSliceReport
	{
		FDebugShopUpgradeVerticalSlicePassReport FirstPass;
		FDebugShopUpgradeVerticalSlicePassReport SecondPass;
		FString ReportPath;
		FString FailureCategory;
		int32 ExitCode = 0;
	};

	const TArray<FString>& GetDebugShopUpgradeVerticalSlicePackageManifest();
	bool ValidateDebugShopUpgradeVerticalSliceManifest(TArray<FString>& OutErrors);
	FDebugShopUpgradeVerticalSlicePassReport InspectDebugShopUpgradeVerticalSliceAssets();
	int32 RunDebugShopUpgradeVerticalSliceSeed(
		FDebugShopUpgradeVerticalSliceReport* OutReport = nullptr);
}
