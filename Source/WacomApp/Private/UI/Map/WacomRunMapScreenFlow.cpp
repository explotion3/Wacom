// Copyright Wacom. All Rights Reserved.

#include "UI/Map/WacomRunMapScreenFlow.h"

#include "Framework/Application/SlateApplication.h"
#include "GameFramework/WacomPlayerController.h"
#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"
#include "RunSession.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/Map/WacomRunMapViewDataBuilder.h"

#define LOCTEXT_NAMESPACE "WacomRunMapScreenFlow"

FWacomRunMapScreenFlow::~FWacomRunMapScreenFlow()
{
	Shutdown();
}

void FWacomRunMapScreenFlow::Initialize(
	AWacomPlayerController& InOwner,
	FWacomRunExplorationPresentationCoordinator& InCoordinator)
{
	Shutdown();
	Owner = &InOwner;
	Coordinator = &InCoordinator;
}

int32 FWacomRunMapScreenFlow::BeginOpenRequest()
{
	if (!Coordinator || !Owner.IsValid() || IsActive() || bOpening)
	{
		return 0;
	}
	++Generation;
	bOpening = true;
	return Generation;
}

bool FWacomRunMapScreenFlow::IsOpenRequestCurrent(const int32 RequestGeneration) const
{
	return bOpening && RequestGeneration > 0 && RequestGeneration == Generation
		&& Coordinator && Owner.IsValid() && !IsActive();
}

void FWacomRunMapScreenFlow::CancelOpenRequest(const int32 RequestGeneration)
{
	if (!IsOpenRequestCurrent(RequestGeneration))
	{
		return;
	}
	bOpening = false;
	++Generation;
}

bool FWacomRunMapScreenFlow::AttachScreen(
	URunSession& InSession,
	UWacomRunMapScreen& InScreen,
	const bool bPreferRecommendedTarget,
	const int32 RequestGeneration)
{
	if (!Coordinator || !Owner.IsValid() || IsActive())
	{
		return false;
	}
	if (RequestGeneration > 0)
	{
		if (!IsOpenRequestCurrent(RequestGeneration))
		{
			return false;
		}
	}
	else if (!bOpening)
	{
		const int32 LocalGeneration = BeginOpenRequest();
		if (LocalGeneration == 0)
		{
			return false;
		}
	}

	const FRunFloorMapSnapshot Snapshot = InSession.BuildCurrentFloorMapSnapshot();
	if (!Snapshot.IsValid()
		|| Snapshot.ActiveActivityKind != ERunExplorationActivityKind::None
		|| Snapshot.StateVersion != Coordinator->GetLastAppliedVersion())
	{
		bOpening = false;
		++Generation;
		return false;
	}

	bOpening = false;
	Session = &InSession;
	Screen = &InScreen;
	OpenedFloorId = Snapshot.FloorId;
	bPreferRecommended = bPreferRecommendedTarget;
	SelectedNode.Reset();
	LastOutcomeDetail = NAME_None;
	ScreenActionHandle = InScreen.OnRunMapActionNative.AddRaw(
		this, &FWacomRunMapScreenFlow::HandleScreenAction);
	ScreenDeactivatedHandle = InScreen.OnRunMapDeactivatedNative.AddRaw(
		this, &FWacomRunMapScreenFlow::HandleScreenDeactivated);
	SessionChangedHandle = InSession.OnRunStateChangedNative.AddRaw(
		this, &FWacomRunMapScreenFlow::HandleRunStateChanged);
	RefreshFromSnapshot(Snapshot);
	return true;
}

void FWacomRunMapScreenFlow::HandleSessionChanged(URunSession* NewSession)
{
	if (bOpening || Session.Get() != NewSession)
	{
		CloseAndCleanup();
	}
}

void FWacomRunMapScreenFlow::Shutdown()
{
	Cleanup(false);
	Owner.Reset();
	Coordinator = nullptr;
}

void FWacomRunMapScreenFlow::HandleScreenAction(
	const FWacomRunMapScreenActionRequest& Request)
{
	if (!Screen.IsValid() || bCleaningUp)
	{
		return;
	}

	switch (Request.Action)
	{
	case EWacomRunMapScreenAction::SelectNode:
		if (Request.SourceStateVersion == LastPresentedVersion)
		{
			SelectedNode = Request.Node;
		}
		break;
	case EWacomRunMapScreenAction::ConfirmTravel:
		ConfirmTravel(Request);
		break;
	case EWacomRunMapScreenAction::Close:
		CloseAndCleanup();
		break;
	}
}

void FWacomRunMapScreenFlow::HandleScreenDeactivated()
{
	Cleanup(true);
}

void FWacomRunMapScreenFlow::HandleRunStateChanged()
{
	if (bTravelSubmissionPending || bCleaningUp)
	{
		return;
	}
	URunSession* RunSession = Session.Get();
	if (!RunSession)
	{
		CloseAndCleanup();
		return;
	}

	const FRunFloorMapSnapshot Snapshot = RunSession->BuildCurrentFloorMapSnapshot();
	if (!Snapshot.IsValid()
		|| Snapshot.FloorId != OpenedFloorId
		|| Snapshot.ActiveActivityKind != ERunExplorationActivityKind::None
		|| !Coordinator
		|| Snapshot.StateVersion != Coordinator->GetLastAppliedVersion())
	{
		CloseAndCleanup();
		return;
	}
	RefreshFromSnapshot(Snapshot);
}

void FWacomRunMapScreenFlow::RefreshFromSnapshot(
	const FRunFloorMapSnapshot& Snapshot,
	const FText& StatusOverride)
{
	UWacomRunMapScreen* ActiveScreen = Screen.Get();
	if (!ActiveScreen)
	{
		return;
	}
	const FWacomRunMapScreenViewData ViewData = FWacomRunMapViewDataBuilder::Build(
		Snapshot,
		SelectedNode,
		bPreferRecommended,
		StatusOverride);
	SelectedNode = ViewData.SelectedNode.IsValid()
		? TOptional<FWacomMapNodeHandle>(ViewData.SelectedNode)
		: TOptional<FWacomMapNodeHandle>();
	bPreferRecommended = false;
	bOpening = false;
	LastPresentedVersion = ViewData.StateVersion;
	ActiveScreen->ApplyViewData(ViewData);
}

void FWacomRunMapScreenFlow::ConfirmTravel(
	const FWacomRunMapScreenActionRequest& Request)
{
	if (bTravelSubmissionPending || !Coordinator)
	{
		return;
	}
	URunSession* RunSession = Session.Get();
	UWacomRunMapScreen* ActiveScreen = Screen.Get();
	if (!RunSession || !ActiveScreen)
	{
		CloseAndCleanup();
		return;
	}

	const FRunFloorMapSnapshot Snapshot = RunSession->BuildCurrentFloorMapSnapshot();
	const bool bVersionsMatch =
		Request.SourceStateVersion == ActiveScreen->GetViewData().StateVersion
		&& Request.SourceStateVersion == Snapshot.StateVersion
		&& Request.SourceStateVersion == Coordinator->GetLastAppliedVersion();
	const FRunFloorMapNodeSnapshot* Target = Snapshot.Nodes.FindByPredicate(
		[&Request](const FRunFloorMapNodeSnapshot& Node)
		{
			return Node.Handle == Request.Node;
		});
	if (!Snapshot.IsValid()
		|| Snapshot.FloorId != OpenedFloorId
		|| !bVersionsMatch
		|| !Target
		|| !Target->bCanMapTravel)
	{
		LastOutcomeDetail = TEXT("MapTravelViewDataStaleOrUnavailable");
		if (Snapshot.IsValid() && Snapshot.FloorId == OpenedFloorId
			&& Snapshot.StateVersion == Coordinator->GetLastAppliedVersion())
		{
			RefreshFromSnapshot(Snapshot, BuildFailureText(LastOutcomeDetail));
		}
		else
		{
			CloseAndCleanup();
		}
		return;
	}

	bTravelSubmissionPending = true;
	const FWacomRunMapTravelPresentationResult Result =
		Coordinator->ApplyMapTravel(Request.Node);
	bTravelSubmissionPending = false;
	LastOutcomeDetail = Result.Detail;

	switch (Result.Outcome)
	{
	case EWacomRunMapTravelPresentationOutcome::Applied:
		CloseAndCleanup();
		return;
	case EWacomRunMapTravelPresentationOutcome::CommittedPresentationFailed:
		UE_LOG(LogTemp, Error,
			TEXT("[RunMapScreenFlow] MapTravel rules committed but scene relocation failed. Detail=%s Version=%d"),
			*Result.Detail.ToString(),
			Result.AppliedVersion);
		CloseAndCleanup();
		return;
	case EWacomRunMapTravelPresentationOutcome::Rejected:
	default:
		const FRunFloorMapSnapshot Refreshed = RunSession->BuildCurrentFloorMapSnapshot();
		if (Refreshed.IsValid()
			&& Refreshed.FloorId == OpenedFloorId
			&& Refreshed.StateVersion == Coordinator->GetLastAppliedVersion())
		{
			RefreshFromSnapshot(Refreshed, BuildFailureText(Result.Detail));
		}
		else
		{
			CloseAndCleanup();
		}
		return;
	}
}

void FWacomRunMapScreenFlow::CloseAndCleanup()
{
	UWacomRunMapScreen* ActiveScreen = Screen.Get();
	Cleanup(true);
	if (ActiveScreen && ActiveScreen->IsActivated())
	{
		ActiveScreen->DeactivateWidget();
	}
}

void FWacomRunMapScreenFlow::Cleanup(const bool bFocusGameViewport)
{
	if (bCleaningUp)
	{
		return;
	}
	bCleaningUp = true;
	if (UWacomRunMapScreen* ActiveScreen = Screen.Get())
	{
		if (ScreenActionHandle.IsValid())
		{
			ActiveScreen->OnRunMapActionNative.Remove(ScreenActionHandle);
		}
		if (ScreenDeactivatedHandle.IsValid())
		{
			ActiveScreen->OnRunMapDeactivatedNative.Remove(ScreenDeactivatedHandle);
		}
	}
	if (URunSession* RunSession = Session.Get(); RunSession && SessionChangedHandle.IsValid())
	{
		RunSession->OnRunStateChangedNative.Remove(SessionChangedHandle);
	}
	ScreenActionHandle.Reset();
	ScreenDeactivatedHandle.Reset();
	SessionChangedHandle.Reset();
	Session.Reset();
	Screen.Reset();
	SelectedNode.Reset();
	OpenedFloorId = NAME_None;
	LastPresentedVersion = 0;
	bPreferRecommended = false;
	bOpening = false;
	bTravelSubmissionPending = false;
	++Generation;
	bCleaningUp = false;

	if (bFocusGameViewport && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

FText FWacomRunMapScreenFlow::BuildFailureText(const FName Detail) const
{
	if (Detail == TEXT("MapTravelTargetAnchorMissing"))
	{
		return LOCTEXT("MissingAnchor", "目标位置尚未在场景中就绪");
	}
	if (Detail == TEXT("MapTravelTargetTransformInvalid"))
	{
		return LOCTEXT("InvalidTransform", "目标位置数据无效");
	}
	if (Detail == TEXT("MapTravelUnavailable")
		|| Detail == TEXT("MapTravelViewDataStaleOrUnavailable"))
	{
		return LOCTEXT("Unavailable", "地图状态已变化，请重新选择");
	}
	return LOCTEXT("Rejected", "暂时无法传送到该节点");
}

#undef LOCTEXT_NAMESPACE
