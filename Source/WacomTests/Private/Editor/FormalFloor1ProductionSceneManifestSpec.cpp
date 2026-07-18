// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomFormalFloor1ProductionSceneAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ProductionSceneManifestSpec,
	"Wacom.Editor.FormalFloor1ProductionScene.Manifest.CountsAndIdentities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ProductionSceneManifestSpec::RunTest(
	const FString& Parameters)
{
	const FWacomFormalFloor1ProductionSceneAutomationSummary Summary =
		FWacomFormalFloor1ProductionSceneAutomationTestView::GetManifestSummary();
	TestEqual(TEXT("Total manifest count"), Summary.TotalCount, 7);
	TestEqual(TEXT("Floor asset count"), Summary.FloorAssetCount, 1);
	TestEqual(TEXT("Blueprint asset count"), Summary.BlueprintAssetCount, 5);
	TestEqual(TEXT("World asset count"), Summary.WorldAssetCount, 1);
	TestEqual(TEXT("Floor group count"), Summary.FloorGroupCount, 1);
	TestEqual(TEXT("EnemyHosts group count"), Summary.EnemyHostsGroupCount, 4);
	TestEqual(TEXT("Scene group count"), Summary.SceneGroupCount, 2);

	TSet<FString> UniquePackages;
	TSet<FName> UniqueIds;
	UniquePackages.Append(Summary.PackagePaths);
	UniqueIds.Append(Summary.StableIds);
	TestEqual(TEXT("Package paths are unique"), UniquePackages.Num(), 7);
	TestEqual(TEXT("Stable identities are unique"), UniqueIds.Num(), 7);
	for (const FString& PackagePath : Summary.PackagePaths)
	{
		TestTrue(*FString::Printf(TEXT("Wacom package root: %s"), *PackagePath),
			PackagePath.StartsWith(TEXT("/Game/Wacom/")));
		TestFalse(*FString::Printf(TEXT("No Debug reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Debug"), ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("No Authoring reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Authoring"), ESearchCase::IgnoreCase));
	}
	TArray<FString> Errors;
	const bool bValid =
		FWacomFormalFloor1ProductionSceneAutomationTestView::ValidateManifest(Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("Manifest invariants pass"), bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ProductionSceneArgumentSpec,
	"Wacom.Editor.FormalFloor1ProductionScene.Manifest.Arguments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ProductionSceneArgumentSpec::RunTest(
	const FString& Parameters)
{
	FString Group;
	FString ReportPath;
	bool bSeedMissing = false;
	bool bCompareSeedDefaults = false;
	TArray<FString> Errors;
	TestTrue(TEXT("Floor seed arguments parse"),
		FWacomFormalFloor1ProductionSceneAutomationTestView::ParseArguments(
			{TEXT("SeedMissing"), TEXT("Group=Floor"),
			 TEXT("CompareSeedDefaults"), TEXT("Report=Saved/Floor.json")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	TestEqual(TEXT("Floor group normalized"), Group, FString(TEXT("Floor")));
	TestTrue(TEXT("Seed flag"), bSeedMissing);
	TestTrue(TEXT("Strict flag"), bCompareSeedDefaults);
	TestEqual(TEXT("Report path"), ReportPath, FString(TEXT("Saved/Floor.json")));

	Errors.Reset();
	TestTrue(TEXT("Inspect is accepted"),
		FWacomFormalFloor1ProductionSceneAutomationTestView::ParseArguments(
			{TEXT("Inspect"), TEXT("-Group=Scene")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	TestEqual(TEXT("Scene group normalized"), Group, FString(TEXT("Scene")));
	TestFalse(TEXT("Inspect remains non-seeding"), bSeedMissing);

	Errors.Reset();
	TestFalse(TEXT("Invalid group rejected"),
		FWacomFormalFloor1ProductionSceneAutomationTestView::ParseArguments(
			{TEXT("Group=Everything")}, Group, bSeedMissing,
			 bCompareSeedDefaults, ReportPath, Errors));
	Errors.Reset();
	TestFalse(TEXT("Force mode rejected"),
		FWacomFormalFloor1ProductionSceneAutomationTestView::ParseArguments(
			{TEXT("Force")}, Group, bSeedMissing,
			 bCompareSeedDefaults, ReportPath, Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ProductionSceneTransientFloorSpec,
	"Wacom.Editor.FormalFloor1ProductionScene.Floor.TransientContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ProductionSceneTransientFloorSpec::RunTest(
	const FString& Parameters)
{
	TArray<FString> Errors;
	const bool bValid =
		FWacomFormalFloor1ProductionSceneAutomationTestView::ValidateTransientFloor(
			Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("Formal Floor 1 transient contract passes"), bValid);
	return true;
}

#endif
