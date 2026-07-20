// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomFormalFloor2ContentAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor2ContentAssetSpec,
	"Wacom.Data.FormalFloor2Content.Assets.LoadAndMatchSeedDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor2ContentAssetSpec::RunTest(const FString& Parameters)
{
	TArray<FString> Errors;
	const bool bValid =
		FWacomFormalFloor2ContentAutomationTestView::ValidateLoadedAssets(
			true, Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("All 47 persisted Floor 2 assets match strict seed defaults"), bValid);
	Errors.Reset();
	const bool bClosureValid =
		FWacomFormalFloor2ContentAutomationTestView::ValidateDependencyClosure(Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("Floor 2 Production dependency closure is allowlisted"),
		bClosureValid);
	return true;
}

#endif
