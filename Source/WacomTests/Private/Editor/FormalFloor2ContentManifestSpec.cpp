// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomFormalFloor2ContentAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor2ContentManifestCountsSpec,
	"Wacom.Editor.FormalFloor2Content.Manifest.CountsAndIdentities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor2ContentManifestCountsSpec::RunTest(const FString& Parameters)
{
	const FWacomFormalFloor2ContentAutomationSummary Summary =
		FWacomFormalFloor2ContentAutomationTestView::GetManifestSummary();
	TestEqual(TEXT("Total manifest count"), Summary.TotalCount, 47);
	TestEqual(TEXT("Cards"), Summary.CardCount, 12);
	TestEqual(TEXT("Behaviors"), Summary.BehaviorCount, 4);
	TestEqual(TEXT("Parts"), Summary.PartCount, 12);
	TestEqual(TEXT("Enemies"), Summary.EnemyCount, 4);
	TestEqual(TEXT("Encounters"), Summary.EncounterCount, 7);
	TestEqual(TEXT("Events"), Summary.EventCount, 3);
	TestEqual(TEXT("Pickups"), Summary.PickupCount, 4);
	TestEqual(TEXT("Shops"), Summary.ShopCount, 1);
	TestEqual(TEXT("Cards group"), Summary.CardsGroupCount, 12);
	TestEqual(TEXT("EnemyGraph group"), Summary.EnemyGraphGroupCount, 20);
	TestEqual(TEXT("NodeDefinitions group"), Summary.NodeDefinitionsGroupCount, 15);

	TSet<FString> UniquePackages;
	TSet<FName> UniqueIds;
	UniquePackages.Append(Summary.PackagePaths);
	UniqueIds.Append(Summary.StableIds);
	TestEqual(TEXT("Package paths are unique"), UniquePackages.Num(), 47);
	TestEqual(TEXT("Stable identities are unique"), UniqueIds.Num(), 47);
	for (const FString& PackagePath : Summary.PackagePaths)
	{
		TestTrue(*FString::Printf(TEXT("MoltCavern package: %s"), *PackagePath),
			PackagePath.Contains(TEXT("MoltCavern")));
		TestFalse(*FString::Printf(TEXT("No Debug reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Debug"), ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("No Authoring reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Authoring"), ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("No test reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Test"), ESearchCase::IgnoreCase));
	}
	TArray<FString> Errors;
	TestTrue(TEXT("Manifest invariants pass"),
		FWacomFormalFloor2ContentAutomationTestView::ValidateManifest(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor2ContentArgumentsSpec,
	"Wacom.Editor.FormalFloor2Content.Manifest.Arguments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor2ContentArgumentsSpec::RunTest(const FString& Parameters)
{
	FString Group;
	FString ReportPath;
	bool bSeedMissing = false;
	bool bCompareSeedDefaults = false;
	TArray<FString> Errors;
	TestTrue(TEXT("Floor 2 arguments use shared parser"),
		FWacomFormalFloor2ContentAutomationTestView::ParseArguments(
			{TEXT("Group=EnemyGraph"), TEXT("SeedMissing"),
			 TEXT("CompareSeedDefaults"), TEXT("Report=Saved/Floor2.json")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	TestEqual(TEXT("Group"), Group, FString(TEXT("EnemyGraph")));
	TestTrue(TEXT("SeedMissing"), bSeedMissing);
	TestTrue(TEXT("CompareSeedDefaults"), bCompareSeedDefaults);
	TestEqual(TEXT("Report"), ReportPath, FString(TEXT("Saved/Floor2.json")));
	Errors.Reset();
	TestFalse(TEXT("Force is rejected"),
		FWacomFormalFloor2ContentAutomationTestView::ParseArguments(
			{TEXT("Force")}, Group, bSeedMissing,
			bCompareSeedDefaults, ReportPath, Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor2ContentTransientDefaultsSpec,
	"Wacom.Editor.FormalFloor2Content.Manifest.TransientDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor2ContentTransientDefaultsSpec::RunTest(
	const FString& Parameters)
{
	TArray<FString> Errors;
	const bool bValid =
		FWacomFormalFloor2ContentAutomationTestView::ValidateTransientDefaults(Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("All Floor 2 transient defaults satisfy current schemas"), bValid);
	Errors.Reset();
	const bool bComparatorValid =
		FWacomFormalFloor2ContentAutomationTestView::ValidateComparatorBoundaries(Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("Structural and strict comparator boundaries remain distinct"),
		bComparatorValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor2ContentInspectOnlyMissingSpec,
	"Wacom.Editor.FormalFloor2Content.Manifest.InspectOnlyMissingIsReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor2ContentInspectOnlyMissingSpec::RunTest(
	const FString& Parameters)
{
	const FWacomFormalFloor2ContentRunSummary Summary =
		FWacomFormalFloor2ContentAutomationTestView::RunSyntheticMissingInspect();
	TestEqual(TEXT("Missing assets are a validation failure"), Summary.ExitCode, 1);
	TestEqual(TEXT("Manifest count"), Summary.ManifestCount, 47);
	TestEqual(TEXT("Selected count"), Summary.SelectedCount, 47);
	TestEqual(TEXT("Missing count"), Summary.MissingCount, 47);
	TestEqual(TEXT("Created count"), Summary.CreatedCount, 0);
	TestEqual(TEXT("Saved count"), Summary.SavedCount, 0);
	TestEqual(TEXT("Failed count"), Summary.FailedCount, 0);
	TestEqual(TEXT("Failure category"), Summary.FailureCategory,
		FString(TEXT("Validation")));
	return true;
}

#endif
