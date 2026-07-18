// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomFormalFloor1ProductionSceneAutomationTestView.h"

#include "ContentBuilders/FormalFloor1ProductionSceneBuilder.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Map/WacomFloorMapDefinition.h"

namespace
{
	FString GroupToString(
		const Wacom::ContentBuilder::EFormalFloor1ProductionSceneGroup Group)
	{
		switch (Group)
		{
		case Wacom::ContentBuilder::EFormalFloor1ProductionSceneGroup::Floor:
			return TEXT("Floor");
		case Wacom::ContentBuilder::EFormalFloor1ProductionSceneGroup::EnemyHosts:
			return TEXT("EnemyHosts");
		case Wacom::ContentBuilder::EFormalFloor1ProductionSceneGroup::Scene:
			return TEXT("Scene");
		case Wacom::ContentBuilder::EFormalFloor1ProductionSceneGroup::All:
		default:
			return TEXT("All");
		}
	}
}

FWacomFormalFloor1ProductionSceneAutomationSummary
FWacomFormalFloor1ProductionSceneAutomationTestView::GetManifestSummary()
{
	using namespace Wacom::ContentBuilder;
	FWacomFormalFloor1ProductionSceneAutomationSummary Summary;
	for (const FFormalFloor1ProductionSceneManifestEntry& Entry :
		GetFormalFloor1ProductionSceneManifest())
	{
		++Summary.TotalCount;
		Summary.PackagePaths.Add(Entry.PackagePath);
		Summary.StableIds.Add(Entry.StableId);
		if (Entry.AssetClass == UWacomFloorMapDefinition::StaticClass())
		{
			++Summary.FloorAssetCount;
		}
		else if (Entry.AssetClass == UBlueprint::StaticClass())
		{
			++Summary.BlueprintAssetCount;
		}
		else if (Entry.AssetClass == UWorld::StaticClass())
		{
			++Summary.WorldAssetCount;
		}
		switch (Entry.Group)
		{
		case EFormalFloor1ProductionSceneGroup::Floor:
			++Summary.FloorGroupCount;
			break;
		case EFormalFloor1ProductionSceneGroup::EnemyHosts:
			++Summary.EnemyHostsGroupCount;
			break;
		case EFormalFloor1ProductionSceneGroup::Scene:
			++Summary.SceneGroupCount;
			break;
		case EFormalFloor1ProductionSceneGroup::All:
			break;
		}
	}
	return Summary;
}

bool FWacomFormalFloor1ProductionSceneAutomationTestView::ValidateManifest(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor1ProductionSceneManifest(
		OutErrors);
}

bool FWacomFormalFloor1ProductionSceneAutomationTestView::ValidateTransientFloor(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor1ProductionTransientFloor(
		OutErrors);
}

FWacomFormalFloor1ProductionSceneRealAssetSummary
FWacomFormalFloor1ProductionSceneAutomationTestView::InspectRealAssets()
{
	using namespace Wacom::ContentBuilder;
	FFormalFloor1ProductionSceneBuildReport Report;
	RunFormalFloor1ProductionSceneBuilder(
		{TEXT("Inspect"), TEXT("Group=All"), TEXT("CompareSeedDefaults"),
		 TEXT("Report=Saved/Automation/Spec015-real-assets.json")},
		&Report);
	FWacomFormalFloor1ProductionSceneRealAssetSummary Summary;
	Summary.ExitCode = Report.ExitCode;
	Summary.ExistingCount = Report.ExistingCount;
	Summary.FailedCount = Report.FailedCount;
	Summary.SavedCount = Report.SavedCount;
	for (const FFormalFloor1ProductionSceneEntryReport& Entry : Report.Entries)
	{
		for (const FString& Diagnostic : Entry.Diagnostics)
		{
			Summary.Diagnostics.Add(FString::Printf(
				TEXT("%s: %s"), *Entry.PackagePath, *Diagnostic));
		}
	}
	return Summary;
}

bool FWacomFormalFloor1ProductionSceneAutomationTestView::ParseArguments(
	const TArray<FString>& Arguments,
	FString& OutCanonicalGroup,
	bool& bOutSeedMissing,
	bool& bOutCompareSeedDefaults,
	FString& OutReportPath,
	TArray<FString>& OutErrors)
{
	Wacom::ContentBuilder::FFormalFloor1ProductionSceneOptions Options;
	const bool bParsed =
		Wacom::ContentBuilder::ParseFormalFloor1ProductionSceneOptions(
			Arguments, Options, OutErrors);
	OutCanonicalGroup = GroupToString(Options.Group);
	bOutSeedMissing = Options.bSeedMissing;
	bOutCompareSeedDefaults = Options.bCompareSeedDefaults;
	OutReportPath = Options.ReportPath;
	return bParsed;
}
