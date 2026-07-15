// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunExplorationPresentationAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"
#include "GameFramework/WacomRunSceneBindingRegistry.h"
#include "Map/WacomMapTypes.h"

struct FWacomRunExplorationPresentationAutomationTestView::FImpl
{
	FWacomRunSceneBindingRegistry Registry;
	FWacomRunExplorationPresentationCoordinator Coordinator;
};

FWacomRunExplorationPresentationAutomationTestView::
	FWacomRunExplorationPresentationAutomationTestView()
	: Impl(MakeUnique<FImpl>())
{
}

FWacomRunExplorationPresentationAutomationTestView::
	~FWacomRunExplorationPresentationAutomationTestView() = default;

void FWacomRunExplorationPresentationAutomationTestView::ResetRegistry(const FName FloorId)
{
	Impl->Registry.Reset(FloorId);
}

bool FWacomRunExplorationPresentationAutomationTestView::RegisterPath(
	AWacomRunPathSegmentActor& Path)
{
	return Impl->Registry.RegisterPath(Path);
}

bool FWacomRunExplorationPresentationAutomationTestView::RegisterNodeAnchor(
	AWacomRunMapNodeAnchorActor& Anchor)
{
	return Impl->Registry.RegisterNodeAnchor(Anchor);
}

bool FWacomRunExplorationPresentationAutomationTestView::RegisterContentHost(
	const FName NodeId,
	const EWacomMapNodeType NodeType,
	AActor& Host)
{
	return Impl->Registry.RegisterContentHost(NodeId, NodeType, Host);
}

void FWacomRunExplorationPresentationAutomationTestView::UnregisterNodeAnchor(
	const AWacomRunMapNodeAnchorActor& Anchor)
{
	Impl->Registry.UnregisterNodeAnchor(Anchor);
}

void FWacomRunExplorationPresentationAutomationTestView::UnregisterContentHost(
	const AActor& Host)
{
	Impl->Registry.UnregisterContentHost(Host);
}

bool FWacomRunExplorationPresentationAutomationTestView::Initialize(
	URunSession& Session,
	UWacomRunPathTraversalComponent& TraversalComponent)
{
	return Impl->Coordinator.Initialize(Session, TraversalComponent, Impl->Registry);
}

void FWacomRunExplorationPresentationAutomationTestView::Shutdown()
{
	Impl->Coordinator.Shutdown();
}

bool FWacomRunExplorationPresentationAutomationTestView::HandleBranchIntent(const FName EdgeId)
{
	return Impl->Coordinator.HandleBranchIntent(EdgeId);
}

bool FWacomRunExplorationPresentationAutomationTestView::HasActiveTraversal() const
{
	return Impl->Coordinator.HasActiveTraversal();
}

int32 FWacomRunExplorationPresentationAutomationTestView::GetLastAppliedVersion() const
{
	return Impl->Coordinator.GetLastAppliedVersion();
}

FName FWacomRunExplorationPresentationAutomationTestView::GetLastErrorDetail() const
{
	return Impl->Coordinator.GetLastErrorDetail();
}

#endif
