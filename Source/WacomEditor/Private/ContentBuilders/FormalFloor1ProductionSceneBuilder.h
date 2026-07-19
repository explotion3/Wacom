// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UClass;
class UWorld;
class UWacomFloorMapDefinition;

namespace Wacom::ContentBuilder
{
	enum class EFormalFloor1ProductionSceneGroup : uint8
	{
		Floor,
		EnemyHosts,
		Scene,
		All,
	};

	enum class EFormalFloor1ProductionSceneEntryState : uint8
	{
		NotProcessed,
		Missing,
		Existing,
		Created,
		Failed,
	};

	struct FFormalFloor1ProductionSceneOptions
	{
		EFormalFloor1ProductionSceneGroup Group =
			EFormalFloor1ProductionSceneGroup::All;
		bool bSeedMissing = false;
		bool bCompareSeedDefaults = false;
		FString ReportPath;
	};

	struct FFormalFloor1ProductionSceneManifestEntry
	{
		EFormalFloor1ProductionSceneGroup Group =
			EFormalFloor1ProductionSceneGroup::Floor;
		FString PackagePath;
		FName StableId = NAME_None;
		UClass* AssetClass = nullptr;
	};

	struct FFormalFloor1ProductionSceneEntryReport
	{
		FString PackagePath;
		FString ClassName;
		FName StableId = NAME_None;
		EFormalFloor1ProductionSceneEntryState State =
			EFormalFloor1ProductionSceneEntryState::NotProcessed;
		bool bSaved = false;
		TArray<FString> Diagnostics;
	};

	struct FFormalFloor1ProductionSceneBuildReport
	{
		FFormalFloor1ProductionSceneOptions Options;
		FString ReportPath;
		TArray<FFormalFloor1ProductionSceneEntryReport> Entries;
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

	const TArray<FFormalFloor1ProductionSceneManifestEntry>&
		GetFormalFloor1ProductionSceneManifest();
	bool ParseFormalFloor1ProductionSceneOptions(
		const TArray<FString>& Arguments,
		FFormalFloor1ProductionSceneOptions& OutOptions,
		TArray<FString>& OutErrors);
	bool ValidateFormalFloor1ProductionSceneManifest(TArray<FString>& OutErrors);
	bool ValidateFormalFloor1ProductionFloor(
		const UWacomFloorMapDefinition& Floor,
		bool bCompareSeedDefaults,
		TArray<FString>& OutErrors);
	bool ValidateFormalFloor1ProductionTransientFloor(TArray<FString>& OutErrors);
	/** Reuse the exact persisted Spec 015 world contract without running its builder. */
	bool ValidateFormalFloor1ProductionWorld(
		UWorld& World,
		UWacomFloorMapDefinition& Floor,
		TArray<FString>& OutErrors);
	int32 RunFormalFloor1ProductionSceneBuilder(
		const TArray<FString>& Arguments,
		FFormalFloor1ProductionSceneBuildReport* OutReport = nullptr);
}
