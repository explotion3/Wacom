// Copyright Wacom. All Rights Reserved.

#include "UI/WacomRunFloorSceneBindingTestAccess.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Testing/WacomRunFloorSceneBindingAutomationTestView.h"

void FWacomRunFloorSceneBindingTestAccess::SetRunSession(
	AWacomPlayerController& Controller,
	URunSession* Session)
{
	FWacomRunFloorSceneBindingAutomationTestView::SetRunSession(Controller, Session);
}

bool FWacomRunFloorSceneBindingTestAccess::Refresh(AWacomPlayerController& Controller)
{
	return FWacomRunFloorSceneBindingAutomationTestView::Refresh(Controller);
}

void FWacomRunFloorSceneBindingTestAccess::ForceVersionDriftOnNextRefresh(
	AWacomPlayerController& Controller)
{
	FWacomRunFloorSceneBindingAutomationTestView::
		ForceVersionDriftOnNextRefresh(Controller);
}

void FWacomRunFloorSceneBindingTestAccess::ForceFloorDriftOnNextRefresh(
	AWacomPlayerController& Controller)
{
	FWacomRunFloorSceneBindingAutomationTestView::
		ForceFloorDriftOnNextRefresh(Controller);
}

uint64 FWacomRunFloorSceneBindingTestAccess::InstalledGeneration(
	const AWacomPlayerController& Controller)
{
	return FWacomRunFloorSceneBindingAutomationTestView::
		GetInstalledGeneration(Controller);
}

FName FWacomRunFloorSceneBindingTestAccess::InstalledFloorId(
	const AWacomPlayerController& Controller)
{
	return FWacomRunFloorSceneBindingAutomationTestView::GetInstalledFloorId(Controller);
}

int32 FWacomRunFloorSceneBindingTestAccess::CoordinatorVersion(
	const AWacomPlayerController& Controller)
{
	return FWacomRunFloorSceneBindingAutomationTestView::
		GetCoordinatorVersion(Controller);
}

FName FWacomRunFloorSceneBindingTestAccess::LastFailureDetail(
	const AWacomPlayerController& Controller)
{
	return FWacomRunFloorSceneBindingAutomationTestView::
		GetLastFailureDetail(Controller);
}

#endif
