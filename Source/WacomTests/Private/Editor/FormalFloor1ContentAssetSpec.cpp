// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomFormalFloor1ContentAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFormalFloor1ContentAssetSpec,
	"Wacom.Data.FormalFloor1Content.Assets.LoadAndMatchSeedDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFormalFloor1ContentAssetSpec::RunTest(const FString& Parameters)
{
	TArray<FString> Errors;
	const bool bValid =
		FWacomFormalFloor1ContentAutomationTestView::ValidateLoadedAssets(
			true, Errors);
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("All 46 formal Floor 1 assets load and match initial seed defaults"),
		bValid);
	return true;
}

#endif
