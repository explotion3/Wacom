// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomValidateRunFloorSceneCommandletAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Commandlets/WacomValidateRunFloorSceneCommandletRunner.h"

int32 FWacomValidateRunFloorSceneCommandletAutomationTestView::ClassifyForTest(
	const UWorld* World,
	const bool bHasMapArgument,
	const bool bMapLoaded)
{
	return Wacom::Editor::ClassifyRunFloorSceneValidation(
		World, bHasMapArgument, bMapLoaded);
}

#endif
