// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Read-only, non-reflection view for focused Preview bootstrap tests. */
struct WACOMEDITOR_API FWacomFormalFloor1PreviewBootstrapAutomationSummary
{
	int32 ManifestCount = 0;
	TArray<FString> PackagePaths;
	int32 ExistingCount = 0;
	int32 MissingCount = 0;
	int32 FailedCount = 0;
	int32 SavedCount = 0;
	bool bPreviewBlueprintExists = false;
	bool bMapExists = false;
	bool bMapConfigured = false;
	bool bMapReadyForBootstrap = false;
	int32 PlayerStartCount = 0;
	TArray<FString> Diagnostics;
};

struct WACOMEDITOR_API FWacomFormalFloor1PreviewBootstrapAutomationTestView
{
	static FWacomFormalFloor1PreviewBootstrapAutomationSummary InspectRealAssets();
	static bool ValidateManifest(TArray<FString>& OutErrors);
	static bool ValidateCollisionPolicyMatrix(TArray<FString>& OutErrors);
};
