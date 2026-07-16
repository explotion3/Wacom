// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Testing/WacomRunFloorSceneValidationMenuAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneValidationMenuSpec,
	"Wacom.Editor.RunSceneValidation.Menu.MainAndDebugMaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneValidationMenuSpec::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Tools -> Wacom validation entry is registered"),
		FWacomRunFloorSceneValidationMenuAutomationTestView::IsMenuEntryRegistered());
	TestTrue(TEXT("Main Run map executes the registered validation action"),
		FWacomRunFloorSceneValidationMenuAutomationTestView::LoadMapAndExecuteMenuEntry(
			TEXT("/Game/Wacom/Maps/L_Exploration")));
	TestTrue(TEXT("Debug Run map executes the registered validation action"),
		FWacomRunFloorSceneValidationMenuAutomationTestView::LoadMapAndExecuteMenuEntry(
			TEXT("/Game/Wacom/Maps/Debug/L_RunExploration_Debug")));
	return true;
}

#endif
