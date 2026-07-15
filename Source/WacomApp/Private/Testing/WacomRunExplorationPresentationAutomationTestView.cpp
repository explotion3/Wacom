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

FName FWacomRunExplorationPresentationAutomationTestView::HandleForwardIntent()
{
	switch (Impl->Coordinator.HandleForwardIntent())
	{
	case EWacomRunForwardIntentResult::Started:
		return TEXT("Started");
	case EWacomRunForwardIntentResult::ChoiceRequired:
		return TEXT("ChoiceRequired");
	case EWacomRunForwardIntentResult::DeadEnd:
		return TEXT("DeadEnd");
	case EWacomRunForwardIntentResult::Unavailable:
		return TEXT("Unavailable");
	case EWacomRunForwardIntentResult::Rejected:
	default:
		return TEXT("Rejected");
	}
}

bool FWacomRunExplorationPresentationAutomationTestView::ApplyNodeActivityResolution(
	const FRunExplorationResolution& Resolution)
{
	return Impl->Coordinator.ApplyNodeActivityResolution(Resolution);
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

FName FWacomRunExplorationPresentationAutomationTestView::GetRouteChoiceModeName() const
{
	switch (Impl->Coordinator.GetRouteChoiceState().Mode)
	{
	case EWacomRunRouteChoiceMode::DeadEnd:
		return TEXT("DeadEnd");
	case EWacomRunRouteChoiceMode::Automatic:
		return TEXT("Automatic");
	case EWacomRunRouteChoiceMode::ChoiceRequired:
		return TEXT("ChoiceRequired");
	case EWacomRunRouteChoiceMode::Unavailable:
	default:
		return TEXT("Unavailable");
	}
}

TArray<FName>
FWacomRunExplorationPresentationAutomationTestView::GetLegalRouteEdgeIds() const
{
	return Impl->Coordinator.GetRouteChoiceState().LegalEdgeIds;
}

int32 FWacomRunExplorationPresentationAutomationTestView::
	GetRouteChoiceSnapshotVersion() const
{
	return Impl->Coordinator.GetRouteChoiceState().SnapshotVersion;
}

#endif
