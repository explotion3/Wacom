// Copyright Wacom. All Rights Reserved.

#pragma once

#include "ContentBuilders/FormalProductionContentSeedService.h"

namespace Wacom::ContentBuilder
{
	using EFormalFloor2ContentGroup = EFormalProductionContentGroup;
	using EFormalFloor2ContentEntryState = EFormalProductionContentEntryState;
	using FFormalFloor2ContentOptions = FFormalProductionContentOptions;
	using FFormalFloor2ContentManifestEntry = FFormalProductionContentManifestEntry;
	using FFormalFloor2ContentEntryReport = FFormalProductionContentEntryReport;
	using FFormalFloor2ContentBuildReport = FFormalProductionContentBuildReport;

	const TArray<FFormalFloor2ContentManifestEntry>& GetFormalFloor2ContentManifest();
	bool ParseFormalFloor2ContentOptions(
		const TArray<FString>& Arguments,
		FFormalFloor2ContentOptions& OutOptions,
		TArray<FString>& OutErrors);
	bool ValidateFormalFloor2ContentManifest(TArray<FString>& OutErrors);
	bool ValidateFormalFloor2TransientDefaults(TArray<FString>& OutErrors);
	bool ValidateFormalFloor2ComparatorBoundaries(TArray<FString>& OutErrors);
	bool ValidateFormalFloor2LoadedAssets(
		bool bCompareSeedDefaults,
		TArray<FString>& OutErrors);
	bool ValidateFormalFloor2DependencyClosure(TArray<FString>& OutErrors);
	int32 RunFormalFloor2ContentBuilder(
		const TArray<FString>& Arguments,
		FFormalFloor2ContentBuildReport* OutReport = nullptr);
}
