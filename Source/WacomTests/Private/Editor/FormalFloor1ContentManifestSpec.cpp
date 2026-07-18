// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomFormalFloor1ContentAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ContentManifestSpec,
	"Wacom.Editor.FormalFloor1Content.Manifest.CountsAndIdentities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ContentManifestSpec::RunTest(const FString& Parameters)
{
	const FWacomFormalFloor1ContentAutomationSummary Summary =
		FWacomFormalFloor1ContentAutomationTestView::GetManifestSummary();
	TestEqual(TEXT("Total manifest count"), Summary.TotalCount, 46);
	TestEqual(TEXT("Cards"), Summary.CardCount, 12);
	TestEqual(TEXT("Behaviors"), Summary.BehaviorCount, 4);
	TestEqual(TEXT("Parts"), Summary.PartCount, 11);
	TestEqual(TEXT("Enemies"), Summary.EnemyCount, 4);
	TestEqual(TEXT("Encounters"), Summary.EncounterCount, 6);
	TestEqual(TEXT("Events"), Summary.EventCount, 4);
	TestEqual(TEXT("Pickups"), Summary.PickupCount, 4);
	TestEqual(TEXT("Shops"), Summary.ShopCount, 1);
	TestEqual(TEXT("Cards group"), Summary.CardsGroupCount, 12);
	TestEqual(TEXT("EnemyGraph group"), Summary.EnemyGraphGroupCount, 19);
	TestEqual(TEXT("NodeDefinitions group"), Summary.NodeDefinitionsGroupCount, 15);

	TSet<FString> UniquePackages;
	TSet<FName> UniqueIds;
	UniquePackages.Append(Summary.PackagePaths);
	UniqueIds.Append(Summary.StableIds);
	TestEqual(TEXT("Package paths are unique"), UniquePackages.Num(), 46);
	TestEqual(TEXT("Stable identities are unique"), UniqueIds.Num(), 46);
	for (const FString& PackagePath : Summary.PackagePaths)
	{
		TestTrue(*FString::Printf(TEXT("Production package root: %s"), *PackagePath),
			PackagePath.StartsWith(TEXT("/Game/Wacom/Data/")));
		TestFalse(*FString::Printf(TEXT("No Debug reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Debug"), ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("No Authoring reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Authoring"), ESearchCase::IgnoreCase));
		TestFalse(*FString::Printf(TEXT("No test reference: %s"), *PackagePath),
			PackagePath.Contains(TEXT("Test"), ESearchCase::IgnoreCase));
	}
	TArray<FString> Errors;
	TestTrue(TEXT("Manifest invariants pass"),
		FWacomFormalFloor1ContentAutomationTestView::ValidateManifest(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ContentArgumentSpec,
	"Wacom.Editor.FormalFloor1Content.Manifest.Arguments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ContentArgumentSpec::RunTest(const FString& Parameters)
{
	FString Group;
	FString ReportPath;
	bool bSeedMissing = false;
	bool bCompareSeedDefaults = false;
	TArray<FString> Errors;
	TestTrue(TEXT("Bare editor arguments parse"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("SeedMissing"), TEXT("Group=Cards"),
			 TEXT("CompareSeedDefaults"), TEXT("Report=Saved/Cards.json")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	TestEqual(TEXT("Cards group normalized"), Group, FString(TEXT("Cards")));
	TestTrue(TEXT("Seed flag"), bSeedMissing);
	TestTrue(TEXT("Strict flag"), bCompareSeedDefaults);
	TestEqual(TEXT("Report path"), ReportPath, FString(TEXT("Saved/Cards.json")));

	Errors.Reset();
	TestTrue(TEXT("Commandlet dash arguments parse"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("-Group=NodeDefinitions"), TEXT("-CompareSeedDefaults")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	TestEqual(TEXT("Node group normalized"), Group,
		FString(TEXT("NodeDefinitions")));
	TestFalse(TEXT("Inspect remains default"), bSeedMissing);

	Errors.Reset();
	TestFalse(TEXT("Invalid group rejected"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("Group=Everything")}, Group, bSeedMissing,
			 bCompareSeedDefaults, ReportPath, Errors));
	TestTrue(TEXT("Invalid group diagnostic"), !Errors.IsEmpty());
	Errors.Reset();
	TestFalse(TEXT("Unknown force mode rejected"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("Force")}, Group, bSeedMissing,
			 bCompareSeedDefaults, ReportPath, Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ContentTransientDefaultsSpec,
	"Wacom.Editor.FormalFloor1Content.Manifest.TransientDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ContentTransientDefaultsSpec::RunTest(
	const FString& Parameters)
{
	TArray<FString> Errors;
	const bool bValid =
		FWacomFormalFloor1ContentAutomationTestView::ValidateTransientDefaults(Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("All transient seed defaults satisfy current schema and formal profile"),
		bValid);
	Errors.Reset();
	const bool bComparatorValid =
		FWacomFormalFloor1ContentAutomationTestView::ValidateComparatorBoundaries(Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("Structural and strict comparator boundaries remain distinct"),
		bComparatorValid);
	return true;
}

#endif
