// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class AWacomPlayerController;
class URunSession;
class UWorld;
class UWacomFloorMapDefinition;

/** Non-reflected automation seam for App-private Run Floor scene binding. */
struct WACOMAPP_API FWacomRunFloorSceneBindingAutomationTestView
{
	static FName ResolveDescriptorStatus(UWorld* World, FName ExpectedFloorId);
	static const UWacomFloorMapDefinition* ResolveDescriptorFloor(
		UWorld* World,
		FName ExpectedFloorId);

	static void SetRunSession(AWacomPlayerController& Controller, URunSession* Session);
	static bool Refresh(AWacomPlayerController& Controller);
	static void ForceVersionDriftOnNextRefresh(AWacomPlayerController& Controller);
	static void ForceFloorDriftOnNextRefresh(AWacomPlayerController& Controller);
	static uint64 GetInstalledGeneration(const AWacomPlayerController& Controller);
	static FName GetInstalledFloorId(const AWacomPlayerController& Controller);
	static int32 GetCoordinatorVersion(const AWacomPlayerController& Controller);
	static FName GetLastFailureDetail(const AWacomPlayerController& Controller);
};

#endif
