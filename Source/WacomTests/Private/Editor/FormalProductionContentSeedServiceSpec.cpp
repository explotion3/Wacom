// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomFormalFloor1ContentAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalProductionContentSeedServiceArgumentsSpec,
	"Wacom.Editor.FormalProductionContentSeedService.ArgumentsAndSafeDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalProductionContentSeedServiceArgumentsSpec::RunTest(
	const FString& Parameters)
{
	FString Group;
	FString ReportPath;
	bool bSeedMissing = true;
	bool bCompareSeedDefaults = true;
	TArray<FString> Errors;
	TestTrue(TEXT("No arguments selects inspect-only All"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{}, Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	TestEqual(TEXT("Default group"), Group, FString(TEXT("All")));
	TestFalse(TEXT("Default never seeds"), bSeedMissing);
	TestFalse(TEXT("Default uses structural comparison"), bCompareSeedDefaults);

	for (const FString& UnsafeArgument :
		{FString(TEXT("Force")), FString(TEXT("Replace")),
		 FString(TEXT("Regenerate")), FString(TEXT("Unknown=Value"))})
	{
		Errors.Reset();
		TestFalse(*FString::Printf(TEXT("Reject %s"), *UnsafeArgument),
			FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
				{UnsafeArgument}, Group, bSeedMissing,
				bCompareSeedDefaults, ReportPath, Errors));
		TestTrue(TEXT("Unsafe argument reports a diagnostic"), !Errors.IsEmpty());
	}

	Errors.Reset();
	TestFalse(TEXT("Duplicate group rejected"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("Group=Cards"), TEXT("Group=EnemyGraph")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	Errors.Reset();
	TestFalse(TEXT("Duplicate SeedMissing rejected"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("SeedMissing"), TEXT("SeedMissing")},
			Group, bSeedMissing, bCompareSeedDefaults, ReportPath, Errors));
	Errors.Reset();
	TestFalse(TEXT("Empty report path rejected"),
		FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
			{TEXT("Report=")}, Group, bSeedMissing,
			bCompareSeedDefaults, ReportPath, Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalProductionContentSeedServiceFloor1FacadeSpec,
	"Wacom.Editor.FormalProductionContentSeedService.Floor1FacadeParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalProductionContentSeedServiceFloor1FacadeSpec::RunTest(
	const FString& Parameters)
{
	TArray<FString> Errors;
	TestTrue(TEXT("Floor 1 manifest is accepted through shared service"),
		FWacomFormalFloor1ContentAutomationTestView::ValidateManifest(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	Errors.Reset();
	TestTrue(TEXT("Shared comparator preserves Floor 1 boundaries"),
		FWacomFormalFloor1ContentAutomationTestView::ValidateComparatorBoundaries(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	return true;
}

#endif
