// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomFormalFloor2ContentAutomationTestView.h"

#include "Cards/CardDefinition.h"
#include "ContentBuilders/FormalFloor2ContentBuilder.h"
#include "ContentBuilders/FormalProductionContentSeedService.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"

FWacomFormalFloor2ContentAutomationSummary
FWacomFormalFloor2ContentAutomationTestView::GetManifestSummary()
{
	using namespace Wacom::ContentBuilder;
	FWacomFormalFloor2ContentAutomationSummary Summary;
	for (const FFormalFloor2ContentManifestEntry& Entry :
		GetFormalFloor2ContentManifest())
	{
		++Summary.TotalCount;
		Summary.PackagePaths.Add(Entry.PackagePath);
		Summary.StableIds.Add(Entry.StableId);
		if (Entry.AssetClass == UCardDefinition::StaticClass()) { ++Summary.CardCount; }
		else if (Entry.AssetClass == UEnemyBehaviorDefinition::StaticClass()) { ++Summary.BehaviorCount; }
		else if (Entry.AssetClass == UEnemyPartDefinition::StaticClass()) { ++Summary.PartCount; }
		else if (Entry.AssetClass == UEnemyDefinition::StaticClass()) { ++Summary.EnemyCount; }
		else if (Entry.AssetClass == UEncounterDefinition::StaticClass()) { ++Summary.EncounterCount; }
		else if (Entry.AssetClass == UWacomRunEventDefinition::StaticClass()) { ++Summary.EventCount; }
		else if (Entry.AssetClass == UWacomRunPickupDefinition::StaticClass()) { ++Summary.PickupCount; }
		else if (Entry.AssetClass == UShopDefinition::StaticClass()) { ++Summary.ShopCount; }

		switch (Entry.Group)
		{
		case EFormalFloor2ContentGroup::Cards: ++Summary.CardsGroupCount; break;
		case EFormalFloor2ContentGroup::EnemyGraph: ++Summary.EnemyGraphGroupCount; break;
		case EFormalFloor2ContentGroup::NodeDefinitions: ++Summary.NodeDefinitionsGroupCount; break;
		case EFormalFloor2ContentGroup::All: break;
		}
	}
	return Summary;
}

bool FWacomFormalFloor2ContentAutomationTestView::ValidateManifest(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor2ContentManifest(OutErrors);
}

bool FWacomFormalFloor2ContentAutomationTestView::ValidateTransientDefaults(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor2TransientDefaults(OutErrors);
}

bool FWacomFormalFloor2ContentAutomationTestView::ValidateComparatorBoundaries(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor2ComparatorBoundaries(OutErrors);
}

bool FWacomFormalFloor2ContentAutomationTestView::ValidateLoadedAssets(
	const bool bCompareSeedDefaults,
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor2LoadedAssets(
		bCompareSeedDefaults, OutErrors);
}

bool FWacomFormalFloor2ContentAutomationTestView::ValidateDependencyClosure(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor2DependencyClosure(OutErrors);
}

bool FWacomFormalFloor2ContentAutomationTestView::ParseArguments(
	const TArray<FString>& Arguments,
	FString& OutCanonicalGroup,
	bool& bOutSeedMissing,
	bool& bOutCompareSeedDefaults,
	FString& OutReportPath,
	TArray<FString>& OutErrors)
{
	Wacom::ContentBuilder::FFormalFloor2ContentOptions Options;
	const bool bParsed = Wacom::ContentBuilder::ParseFormalFloor2ContentOptions(
		Arguments, Options, OutErrors);
	OutCanonicalGroup = Wacom::ContentBuilder::FormalProductionGroupToString(
		Options.Group);
	bOutSeedMissing = Options.bSeedMissing;
	bOutCompareSeedDefaults = Options.bCompareSeedDefaults;
	OutReportPath = Options.ReportPath;
	return bParsed;
}

FWacomFormalFloor2ContentRunSummary
FWacomFormalFloor2ContentAutomationTestView::Run(const TArray<FString>& Arguments)
{
	Wacom::ContentBuilder::FFormalFloor2ContentBuildReport Report;
	Wacom::ContentBuilder::RunFormalFloor2ContentBuilder(Arguments, &Report);
	FWacomFormalFloor2ContentRunSummary Summary;
	Summary.ExitCode = Report.ExitCode;
	Summary.ManifestCount = Report.ManifestCount;
	Summary.SelectedCount = Report.SelectedCount;
	Summary.CreatedCount = Report.CreatedCount;
	Summary.ExistingCount = Report.ExistingCount;
	Summary.MissingCount = Report.MissingCount;
	Summary.FailedCount = Report.FailedCount;
	Summary.SavedCount = Report.SavedCount;
	Summary.FailureCategory = Report.FailureCategory;
	Summary.ReportPath = Report.ReportPath;
	return Summary;
}

FWacomFormalFloor2ContentRunSummary
FWacomFormalFloor2ContentAutomationTestView::RunSyntheticMissingInspect()
{
	using namespace Wacom::ContentBuilder;
	TArray<FFormalProductionContentManifestEntry> Manifest;
	Manifest.Reserve(47);
	const auto AddEntries = [&Manifest](
		const EFormalProductionContentGroup Group,
		const TCHAR* GroupName,
		const int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FFormalProductionContentManifestEntry& Entry = Manifest.AddDefaulted_GetRef();
			Entry.Group = Group;
			Entry.PackagePath = FString::Printf(
				TEXT("/Game/Wacom/Data/Spec018SyntheticMissing/%s/DA_Missing_%02d"),
				GroupName,
				Index);
			Entry.StableId = FName(*FString::Printf(
				TEXT("Spec018.SyntheticMissing.%s.%02d"), GroupName, Index));
			Entry.AssetClass = UCardDefinition::StaticClass();
		}
	};
	AddEntries(EFormalProductionContentGroup::Cards, TEXT("Cards"), 12);
	AddEntries(EFormalProductionContentGroup::EnemyGraph, TEXT("EnemyGraph"), 20);
	AddEntries(EFormalProductionContentGroup::NodeDefinitions,
		TEXT("NodeDefinitions"), 15);

	FFormalProductionContentProfile Profile;
	Profile.LogLabel = TEXT("Spec018SyntheticMissing");
	Profile.ReportFolder = TEXT("FormalFloor2Content");
	Profile.Manifest = &Manifest;
	Profile.ExpectedCardsCount = 12;
	Profile.ExpectedEnemyGraphCount = 20;
	Profile.ExpectedNodeDefinitionsCount = 15;
	Profile.ExpectedClassCounts = {{UCardDefinition::StaticClass(), 47}};
	Profile.ConfigureExpected = [](
		UObject&,
		const FFormalProductionContentManifestEntry&,
		const FFormalProductionResolveObject&,
		TArray<FString>& OutErrors)
	{
		OutErrors.Add(TEXT("Synthetic missing inspect unexpectedly configured an asset."));
		return false;
	};

	FFormalProductionContentBuildReport Report;
	RunFormalProductionContentSeedService(
		Profile,
		{TEXT("Group=All"), TEXT("CompareSeedDefaults"),
		 TEXT("Report=Saved/Automation/Spec018-synthetic-missing-inspect.json")},
		&Report);
	FWacomFormalFloor2ContentRunSummary Summary;
	Summary.ExitCode = Report.ExitCode;
	Summary.ManifestCount = Report.ManifestCount;
	Summary.SelectedCount = Report.SelectedCount;
	Summary.CreatedCount = Report.CreatedCount;
	Summary.ExistingCount = Report.ExistingCount;
	Summary.MissingCount = Report.MissingCount;
	Summary.FailedCount = Report.FailedCount;
	Summary.SavedCount = Report.SavedCount;
	Summary.FailureCategory = Report.FailureCategory;
	Summary.ReportPath = Report.ReportPath;
	return Summary;
}
