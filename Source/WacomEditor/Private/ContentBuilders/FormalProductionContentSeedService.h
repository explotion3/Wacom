// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class UClass;

namespace Wacom::ContentBuilder
{
	enum class EFormalProductionContentGroup : uint8
	{
		Cards,
		EnemyGraph,
		NodeDefinitions,
		All,
	};

	enum class EFormalProductionContentEntryState : uint8
	{
		NotProcessed,
		Missing,
		Existing,
		Created,
		Failed,
	};

	struct FFormalProductionContentOptions
	{
		EFormalProductionContentGroup Group = EFormalProductionContentGroup::All;
		bool bSeedMissing = false;
		bool bCompareSeedDefaults = false;
		FString ReportPath;
	};

	struct FFormalProductionContentManifestEntry
	{
		EFormalProductionContentGroup Group = EFormalProductionContentGroup::Cards;
		FString PackagePath;
		FName StableId = NAME_None;
		UClass* AssetClass = nullptr;
	};

	struct FFormalProductionContentEntryReport
	{
		FString PackagePath;
		FString ClassName;
		FName StableId = NAME_None;
		EFormalProductionContentEntryState State =
			EFormalProductionContentEntryState::NotProcessed;
		bool bSaved = false;
		TArray<FString> Diagnostics;
	};

	struct FFormalProductionContentBuildReport
	{
		FFormalProductionContentOptions Options;
		FString ReportPath;
		TArray<FFormalProductionContentEntryReport> Entries;
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

	using FFormalProductionResolveObject = TFunction<UObject*(const FString&)>;
	using FFormalProductionConfigureExpected = TFunction<bool(
		UObject&,
		const FFormalProductionContentManifestEntry&,
		const FFormalProductionResolveObject&,
		TArray<FString>&)>;
	using FFormalProductionValidateProfile = TFunction<bool(TArray<FString>&)>;

	struct FFormalProductionContentProfile
	{
		FString LogLabel;
		FString ReportFolder;
		const TArray<FFormalProductionContentManifestEntry>* Manifest = nullptr;
		int32 ExpectedCardsCount = 0;
		int32 ExpectedEnemyGraphCount = 0;
		int32 ExpectedNodeDefinitionsCount = 0;
		TArray<TPair<UClass*, int32>> ExpectedClassCounts;
		TArray<FString> ReadOnlyDependencies;
		FFormalProductionConfigureExpected ConfigureExpected;
		FFormalProductionValidateProfile ValidateProfileSpecific;
	};

	FString FormalProductionGroupToString(EFormalProductionContentGroup Group);
	FString FormalProductionStateToString(EFormalProductionContentEntryState State);
	FString FormalProductionObjectPathForPackage(const FString& PackagePath);

	bool ParseFormalProductionContentOptions(
		const TArray<FString>& Arguments,
		FFormalProductionContentOptions& OutOptions,
		TArray<FString>& OutErrors);
	void TokenizeFormalProductionCommandletParams(
		const FString& Params,
		TArray<FString>& OutArguments);

	bool ValidateFormalProductionContentManifest(
		const FFormalProductionContentProfile& Profile,
		TArray<FString>& OutErrors);

	bool ValidateFormalProductionObjectWithSharedRules(
		UObject& Object,
		TArray<FString>& OutErrors);

	bool CompareFormalProductionEditableProperties(
		const UObject& Actual,
		const UObject& Expected,
		bool bStrict,
		TArray<FString>& OutErrors);

	bool BuildFormalProductionExpectedObject(
		const FFormalProductionContentProfile& Profile,
		const FFormalProductionContentManifestEntry& Entry,
		TMap<FString, UObject*>& ObjectsByPackage,
		TStrongObjectPtr<UObject>& OutExpected,
		TArray<FString>& OutErrors);

	bool ValidateFormalProductionLoadedAssets(
		const FFormalProductionContentProfile& Profile,
		bool bCompareSeedDefaults,
		TArray<FString>& OutErrors);
	bool ValidateFormalProductionDependencyClosure(
		const FFormalProductionContentProfile& Profile,
		TArray<FString>& OutErrors);

	int32 RunFormalProductionContentSeedService(
		const FFormalProductionContentProfile& Profile,
		const TArray<FString>& Arguments,
		FFormalProductionContentBuildReport* OutReport = nullptr);
}
