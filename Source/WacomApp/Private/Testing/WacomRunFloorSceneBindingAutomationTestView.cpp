// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunFloorSceneBindingAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WacomPlayerController.h"
#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"
#include "GameFramework/WacomRunFloorSceneDescriptorResolver.h"
#include "GameFramework/WacomRunSceneBindingRegistry.h"

FName FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
	UWorld* World,
	const FName ExpectedFloorId)
{
	return FWacomRunFloorSceneDescriptorResolver::Resolve(World, ExpectedFloorId)
		.GetDetail();
}

const UWacomFloorMapDefinition*
FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorFloor(
	UWorld* World,
	const FName ExpectedFloorId)
{
	return FWacomRunFloorSceneDescriptorResolver::Resolve(World, ExpectedFloorId)
		.FloorDefinition;
}

void FWacomRunFloorSceneBindingAutomationTestView::SetRunSession(
	AWacomPlayerController& Controller,
	URunSession* Session)
{
	Controller.RunSession = Session;
}

bool FWacomRunFloorSceneBindingAutomationTestView::Refresh(
	AWacomPlayerController& Controller)
{
	return Controller.RefreshRunExplorationPresentationBinding();
}

void FWacomRunFloorSceneBindingAutomationTestView::ForceVersionDriftOnNextRefresh(
	AWacomPlayerController& Controller)
{
	Controller.RunExplorationSceneBindingPreCommitFaultForAutomation =
		TEXT("VersionDrift");
}

void FWacomRunFloorSceneBindingAutomationTestView::ForceFloorDriftOnNextRefresh(
	AWacomPlayerController& Controller)
{
	Controller.RunExplorationSceneBindingPreCommitFaultForAutomation =
		TEXT("FloorDrift");
}

uint64 FWacomRunFloorSceneBindingAutomationTestView::GetInstalledGeneration(
	const AWacomPlayerController& Controller)
{
	return Controller.RunExplorationSceneBindingGeneration;
}

FName FWacomRunFloorSceneBindingAutomationTestView::GetInstalledFloorId(
	const AWacomPlayerController& Controller)
{
	return Controller.RunExplorationSceneBindingRegistry
		? Controller.RunExplorationSceneBindingRegistry->GetFloorId()
		: NAME_None;
}

int32 FWacomRunFloorSceneBindingAutomationTestView::GetCoordinatorVersion(
	const AWacomPlayerController& Controller)
{
	return Controller.RunExplorationPresentationCoordinator
		? Controller.RunExplorationPresentationCoordinator->GetLastAppliedVersion()
		: 0;
}

FName FWacomRunFloorSceneBindingAutomationTestView::GetLastFailureDetail(
	const AWacomPlayerController& Controller)
{
	return Controller.RunExplorationSceneBindingLastFailureDetail;
}

#endif
