// Copyright Wacom. All Rights Reserved.

#pragma once

#include "ContentBuilders/FormalProductionContentSeedService.h"

namespace Wacom::ContentBuilder
{
	using EFormalFloor1ContentGroup = EFormalProductionContentGroup;
	using EFormalFloor1ContentEntryState = EFormalProductionContentEntryState;
	using FFormalFloor1ContentOptions = FFormalProductionContentOptions;
	using FFormalFloor1ContentManifestEntry = FFormalProductionContentManifestEntry;
	using FFormalFloor1ContentEntryReport = FFormalProductionContentEntryReport;
	using FFormalFloor1ContentBuildReport = FFormalProductionContentBuildReport;

	const TArray<FFormalFloor1ContentManifestEntry>& GetFormalFloor1ContentManifest();
	bool ParseFormalFloor1ContentOptions(
		const TArray<FString>& Arguments,
		FFormalFloor1ContentOptions& OutOptions,
		TArray<FString>& OutErrors);
	bool ValidateFormalFloor1ContentManifest(TArray<FString>& OutErrors);
	bool ValidateFormalFloor1TransientDefaults(TArray<FString>& OutErrors);
	bool ValidateFormalFloor1ComparatorBoundaries(TArray<FString>& OutErrors);
	bool ValidateFormalFloor1LoadedAssets(
		bool bCompareSeedDefaults,
		TArray<FString>& OutErrors);
	int32 RunFormalFloor1ContentBuilder(
		const TArray<FString>& Arguments,
		FFormalFloor1ContentBuildReport* OutReport = nullptr);
}
