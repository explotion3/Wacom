// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Exploration/RunExplorationCommand.h"
#include "RunSession.h"

FWacomRunExplorationPresentationCoordinator::~FWacomRunExplorationPresentationCoordinator()
{
	Shutdown();
}

bool FWacomRunExplorationPresentationCoordinator::Initialize(
	URunSession& InSession,
	UWacomRunPathTraversalComponent& InTraversal,
	FWacomRunSceneBindingRegistry& InRegistry)
{
	Shutdown();
	const FRunExplorationSnapshot Snapshot = InSession.BuildExplorationSnapshot();
	if (Snapshot.StateVersion <= 0 || Snapshot.CurrentNode.FloorId != InRegistry.GetFloorId())
	{
		LastErrorDetail = TEXT("CoordinatorInitializationMismatch");
		return false;
	}

	AWacomRunMapNodeAnchorActor* CurrentAnchor = InRegistry.FindNodeAnchor(Snapshot.CurrentNode.NodeId);
	if (!CurrentAnchor || !InTraversal.AnchorAtTransform(CurrentAnchor->GetViewTransform()))
	{
		LastErrorDetail = TEXT("CurrentNodeAnchorMissing");
		return false;
	}

	Session = &InSession;
	Traversal = &InTraversal;
	Registry = &InRegistry;
	LastAppliedVersion = Snapshot.StateVersion;
	LastErrorDetail = NAME_None;
	InTraversal.OnReachedStartNative().AddRaw(this, &FWacomRunExplorationPresentationCoordinator::HandleReachedStart);
	InTraversal.OnReachedEndNative().AddRaw(this, &FWacomRunExplorationPresentationCoordinator::HandleReachedEnd);
	return true;
}

void FWacomRunExplorationPresentationCoordinator::Shutdown()
{
	if (ActiveTicket.IsSet() && Session.IsValid())
	{
		CancelActiveTraversal(TEXT("CoordinatorShutdown"));
	}
	if (UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get())
	{
		TraversalComponent->OnReachedStartNative().RemoveAll(this);
		TraversalComponent->OnReachedEndNative().RemoveAll(this);
	}
	Session.Reset();
	Traversal.Reset();
	Registry = nullptr;
	ActiveTicket.Reset();
	ActiveSceneBinding.Reset();
	LastAppliedVersion = 0;
	NodeContentPresentationRequestedNative.Clear();
}

bool FWacomRunExplorationPresentationCoordinator::HandleBranchIntent(const FName EdgeId)
{
	URunSession* RunSession = Session.Get();
	UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get();
	if (!RunSession || !TraversalComponent || !Registry || ActiveTicket.IsSet())
	{
		LastErrorDetail = TEXT("CoordinatorUnavailableOrBusy");
		return false;
	}

	const FRunExplorationSnapshot Snapshot = RunSession->BuildExplorationSnapshot();
	if (Snapshot.StateVersion != LastAppliedVersion)
	{
		DisableTraversal(TEXT("CoordinatorVersionDrift"));
		return false;
	}
	FWacomRunTraversalSceneBinding SceneBinding = Registry->PreflightTraversal(Snapshot, EdgeId);
	if (!SceneBinding.IsOk())
	{
		LastErrorDetail = SceneBinding.Status.Detail;
		return false;
	}

	const FRunExplorationResolution Resolution = RunSession->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(SceneBinding.Edge, LastAppliedVersion));
	if (!ApplyResolution(Resolution) || !Resolution.TraversalTicket.IsSet())
	{
		return false;
	}

	ActiveTicket = Resolution.TraversalTicket;
	ActiveSceneBinding = MoveTemp(SceneBinding);
	if (!TraversalComponent->BeginTraversal(ActiveSceneBinding->Path.Get()))
	{
		CancelActiveTraversal(TEXT("PathTraversalStartFailed"));
		return false;
	}
	LastErrorDetail = NAME_None;
	return true;
}

bool FWacomRunExplorationPresentationCoordinator::ApplyMapTravel(
	const FWacomMapNodeHandle& TargetNode)
{
	URunSession* RunSession = Session.Get();
	UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get();
	AWacomRunMapNodeAnchorActor* TargetAnchor = Registry
		? Registry->FindNodeAnchor(TargetNode.NodeId)
		: nullptr;
	if (!RunSession || !TraversalComponent || !Registry || !TargetAnchor || ActiveTicket.IsSet())
	{
		LastErrorDetail = TEXT("MapTravelSceneBindingMissing");
		return false;
	}
	const FTransform CachedTarget = TargetAnchor->GetViewTransform();
	const FRunExplorationResolution Resolution = RunSession->ResolveExplorationCommand(
		FRunExplorationCommand::MapTravel(TargetNode, LastAppliedVersion));
	if (!ApplyResolution(Resolution))
	{
		return false;
	}
	if (!TraversalComponent->AnchorAtTransform(CachedTarget))
	{
		DisableTraversal(TEXT("MapTravelAnchorApplyFailed"));
		return false;
	}
	return true;
}

void FWacomRunExplorationPresentationCoordinator::HandleSessionChanged(URunSession* NewSession)
{
	UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get();
	FWacomRunSceneBindingRegistry* ExistingRegistry = Registry;
	Shutdown();
	if (NewSession && TraversalComponent && ExistingRegistry)
	{
		Initialize(*NewSession, *TraversalComponent, *ExistingRegistry);
	}
}

bool FWacomRunExplorationPresentationCoordinator::ApplyResolution(
	const FRunExplorationResolution& Resolution)
{
	if (!Resolution.IsOk())
	{
		LastErrorDetail = Resolution.Status.Detail;
		return false;
	}
	if (Resolution.VersionBefore != LastAppliedVersion
		|| Resolution.VersionAfter != Resolution.VersionBefore + 1
		|| Resolution.PostSnapshot.StateVersion != Resolution.VersionAfter)
	{
		DisableTraversal(TEXT("ResolutionVersionMismatch"));
		return false;
	}
	LastAppliedVersion = Resolution.VersionAfter;
	return true;
}

void FWacomRunExplorationPresentationCoordinator::HandleReachedStart()
{
	CancelActiveTraversal(TEXT("TraversalReturnedToSource"));
}

void FWacomRunExplorationPresentationCoordinator::HandleReachedEnd()
{
	if (!ActiveTicket.IsSet() || !ActiveSceneBinding.IsSet() || !Registry)
	{
		DisableTraversal(TEXT("TraversalPresentationStateMissing"));
		return;
	}

	AWacomRunMapNodeAnchorActor* TargetAnchor = nullptr;
	AActor* ContentHost = nullptr;
	const FWacomStatus Preflight = Registry->RevalidateTarget(
		ActiveSceneBinding->TargetNode,
		ActiveSceneBinding->TargetNodeType,
		TargetAnchor,
		ContentHost);
	if (!Preflight.IsOk())
	{
		CancelActiveTraversal(Preflight.Detail);
		return;
	}

	URunSession* RunSession = Session.Get();
	UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get();
	if (!RunSession || !TraversalComponent)
	{
		DisableTraversal(TEXT("CoordinatorUnavailable"));
		return;
	}
	const FWacomMapNodeHandle CommittedTarget = ActiveSceneBinding->TargetNode;
	const FTransform CachedTarget = ActiveSceneBinding->CachedTargetTransform;
	const FRunExplorationResolution Resolution = RunSession->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(ActiveTicket.GetValue()));
	if (!Resolution.IsOk())
	{
		// Complete 失败不会消费规则票据；必须显式 Cancel，不能只恢复场景位置。
		CancelActiveTraversal(Resolution.Status.Detail);
		return;
	}
	if (!ApplyResolution(Resolution))
	{
		RecoverToSource();
		return;
	}

	// 规则提交成功后绝不回源；Actor 此刻失效时使用 Begin 缓存的目标 Transform。
	const FTransform TargetTransform = IsValid(TargetAnchor)
		? TargetAnchor->GetViewTransform()
		: CachedTarget;
	ActiveTicket.Reset();
	ActiveSceneBinding.Reset();
	if (!TraversalComponent->AnchorAtTransform(TargetTransform))
	{
		DisableTraversal(TEXT("CommittedTargetAnchorApplyFailed"));
		return;
	}
	if (ContentHost)
	{
		NodeContentPresentationRequestedNative.Broadcast(CommittedTarget, ContentHost);
	}
}

bool FWacomRunExplorationPresentationCoordinator::CancelActiveTraversal(const FName FailureDetail)
{
	URunSession* RunSession = Session.Get();
	if (!RunSession || !ActiveTicket.IsSet())
	{
		DisableTraversal(FailureDetail);
		return false;
	}
	const FRunExplorationResolution Resolution = RunSession->ResolveExplorationCommand(
		FRunExplorationCommand::CancelTraversal(ActiveTicket.GetValue()));
	if (!ApplyResolution(Resolution))
	{
		DisableTraversal(Resolution.Status.Detail);
		return false;
	}
	RecoverToSource();
	LastErrorDetail = FailureDetail;
	return true;
}

void FWacomRunExplorationPresentationCoordinator::RecoverToSource()
{
	UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get();
	const TOptional<FWacomRunTraversalSceneBinding> Binding = ActiveSceneBinding;
	ActiveTicket.Reset();
	ActiveSceneBinding.Reset();
	if (!TraversalComponent || !Binding.IsSet()
		|| !TraversalComponent->AnchorAtTransform(Binding->CachedSourceTransform))
	{
		DisableTraversal(TEXT("SourceRecoveryFailed"));
	}
}

void FWacomRunExplorationPresentationCoordinator::DisableTraversal(const FName FailureDetail)
{
	LastErrorDetail = FailureDetail;
	ActiveTicket.Reset();
	ActiveSceneBinding.Reset();
	if (UWacomRunPathTraversalComponent* TraversalComponent = Traversal.Get())
	{
		TraversalComponent->DeactivateTraversal();
	}
}
