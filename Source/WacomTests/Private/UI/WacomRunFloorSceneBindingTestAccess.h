// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class AWacomPlayerController;
class URunSession;

/** WacomTests-private adapter over the production non-reflected automation view. */
struct FWacomRunFloorSceneBindingTestAccess
{
	static void SetRunSession(AWacomPlayerController& Controller, URunSession* Session);
	static bool Refresh(AWacomPlayerController& Controller);
	static void ForceVersionDriftOnNextRefresh(AWacomPlayerController& Controller);
	static void ForceFloorDriftOnNextRefresh(AWacomPlayerController& Controller);
	static uint64 InstalledGeneration(const AWacomPlayerController& Controller);
	static FName InstalledFloorId(const AWacomPlayerController& Controller);
	static int32 CoordinatorVersion(const AWacomPlayerController& Controller);
	static FName LastFailureDetail(const AWacomPlayerController& Controller);
};

#endif
