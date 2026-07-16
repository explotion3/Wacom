// Copyright Wacom. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

class UWorld;

struct WACOMEDITOR_API FWacomValidateRunFloorSceneCommandletAutomationTestView
{
	static int32 ClassifyForTest(
		const UWorld* World,
		bool bHasMapArgument,
		bool bMapLoaded);
};

#endif
