// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Editor/RunFloorSceneValidationTestFixture.h"
#include "Testing/WacomValidateRunFloorSceneCommandletAutomationTestView.h"

using namespace WacomRunFloorSceneValidationTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneValidationCommandletExitSpec,
	"Wacom.Editor.RunSceneValidation.Commandlet.ExitCodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneValidationCommandletExitSpec::RunTest(
	const FString& Parameters)
{
	TestEqual(TEXT("Missing Map argument exits 2"),
		FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
			nullptr, false, false), 2);
	TestEqual(TEXT("Map load failure exits 2"),
		FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
			nullptr, true, false), 2);
	{
		UWorld* WorldWithoutDescriptor = UWorld::CreateWorld(EWorldType::Game, false);
		TestEqual(TEXT("Descriptor resolution failure exits 2"),
			FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
				WorldWithoutDescriptor, true, true), 2);
		WorldWithoutDescriptor->DestroyWorld(false);
	}
	{
		FFixture Fixture;
		Fixture.World->WorldType = EWorldType::Inactive;
		TestEqual(TEXT("Loaded inactive map asset exits 0"),
			FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
				Fixture.World, true, true), 0);
		Fixture.World->WorldType = EWorldType::Game;
		TestEqual(TEXT("Valid scene exits 0"),
			FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
				Fixture.World, true, true), 0);
		Fixture.World->DestroyActor(Fixture.EventAnchor);
		TestEqual(TEXT("Resolved scene contract error exits 1"),
			FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
				Fixture.World, true, true), 1);
	}
	return true;
}

#endif
