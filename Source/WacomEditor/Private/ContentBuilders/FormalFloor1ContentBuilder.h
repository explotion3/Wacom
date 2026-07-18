// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UClass;

namespace Wacom::ContentBuilder
{
	enum class EFormalFloor1ContentGroup : uint8
	{
		Cards,
		EnemyGraph,
		NodeDefinitions,
		All,
	};

	enum class EFormalFloor1ContentEntryState : uint8
	{
		NotProcessed,
		Missing,
		Existing,
		Created,
		Failed,
	};

	struct FFormalFloor1ContentOptions
	{
		EFormalFloor1ContentGroup Group = EFormalFloor1ContentGroup::All;
		bool bSeedMissing = false;
		bool bCompareSeedDefaults = false;
		FString ReportPath;
	};

	struct FFormalFloor1ContentManifestEntry
	{
		EFormalFloor1ContentGroup Group = EFormalFloor1ContentGroup::Cards;
		FString PackagePath;
		FName StableId = NAME_None;
		UClass* AssetClass = nullptr;
	};

	struct FFormalFloor1ContentEntryReport
	{
		FString PackagePath;
		FString ClassName;
		FName StableId = NAME_None;
		EFormalFloor1ContentEntryState State = EFormalFloor1ContentEntryState::NotProcessed;
		bool bSaved = false;
		TArray<FString> Diagnostics;
	};

	struct FFormalFloor1ContentBuildReport
	{
		FFormalFloor1ContentOptions Options;
		FString ReportPath;
		TArray<FFormalFloor1ContentEntryReport> Entries;
		int32 ManifestCount = 0;
		int32 SelectedCount = 0;
		int32 CreatedCount = 0;
		int32 ExistingCount = 0;
		int32 MissingCount = 0;
		int32 FailedCount = 0;
		int32 SavedCount = 0;
		int32 ExitCode = 0;
		FString FailureCategory;

		bool IsOk() const { return ExitCode == 0; }
	};

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
