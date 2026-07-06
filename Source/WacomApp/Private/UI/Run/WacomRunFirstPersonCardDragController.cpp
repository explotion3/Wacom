// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunFirstPersonCardDragController.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Run/WacomRunFirstPersonCardDetailController.h"
#include "UI/Run/WacomRunFirstPersonCardDropCoordinator.h"

FWacomRunFirstPersonCardDragController::FWacomRunFirstPersonCardDragController(
	AWacomPlayerController& InPlayerController)
	: PlayerController(InPlayerController)
{
}

void FWacomRunFirstPersonCardDragController::RefreshBinding()
{
	UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	UWacomFirstPersonCardAnchorComponent* CurrentBoundAnchor = BoundAnchor.Get();

	const bool bShouldBind =
		Anchor
		&& PlayerController.GetRunFirstPersonCardDropCoordinator()
			.ShouldBindRunFirstPersonCardDropDelegates();

	if ((!bShouldBind || CurrentBoundAnchor != Anchor) && CurrentBoundAnchor)
	{
		UnbindAnchor(*CurrentBoundAnchor);
		CameraLookBridge.ClearDragView(
			[this]()
			{
				PlayerController.ClearRunFirstPersonCardDragCameraLookOverride();
			});
		BoundAnchor.Reset();
	}

	if (bShouldBind && Anchor && BoundAnchor.Get() != Anchor)
	{
		UnbindAnchor(*Anchor);
		Anchor->OnFirstPersonCardLayerDragStarted.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragStarted);
		Anchor->OnFirstPersonCardLayerDragUpdated.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragUpdated);
		Anchor->OnFirstPersonCardLayerDragReleased.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragReleased);
		Anchor->OnFirstPersonCardLayerDragCancelled.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragCancelled);
		Anchor->OnFirstPersonCardLayerPointerMoved.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerPointerMoved);
		Anchor->OnFirstPersonCardLayerPointerLeft.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerPointerLeft);
		BoundAnchor = Anchor;
	}
}

void FWacomRunFirstPersonCardDragController::UnbindCurrentBinding()
{
	if (UWacomFirstPersonCardAnchorComponent* Anchor = BoundAnchor.Get())
	{
		UnbindAnchor(*Anchor);
	}
	CameraLookBridge.ClearDragView(
		[this]()
		{
			PlayerController.ClearRunFirstPersonCardDragCameraLookOverride();
		});
	BoundAnchor.Reset();
}

void FWacomRunFirstPersonCardDragController::PumpActiveDragPointer()
{
	UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	if (!Anchor || !Anchor->IsFirstPersonCardDragGestureActive())
	{
		return;
	}

	FVector2D MouseWidgetPosition = FVector2D::ZeroVector;
	if (!PlayerController.TryGetMouseWidgetPosition(MouseWidgetPosition))
	{
		return;
	}

	Anchor->UpdateFirstPersonCardDragPointer(MouseWidgetPosition);
}

bool FWacomRunFirstPersonCardDragController::TryReleaseActiveDragPointer()
{
	UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	if (!Anchor || !Anchor->IsFirstPersonCardDragGestureActive())
	{
		return false;
	}

	FVector2D MouseWidgetPosition = FVector2D::ZeroVector;
	if (PlayerController.TryGetMouseWidgetPosition(MouseWidgetPosition))
	{
		Anchor->UpdateFirstPersonCardDragPointer(MouseWidgetPosition);
		return Anchor->ReleaseFirstPersonCardDragGesture(MouseWidgetPosition);
	}

	return Anchor->ReleaseFirstPersonCardDragGestureAtCurrentPointer();
}

bool FWacomRunFirstPersonCardDragController::TryCancelActiveGestureForTurnBoundaryShortcut()
{
	UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	if (!Anchor || !Anchor->IsFirstPersonCardDragGestureActive())
	{
		return false;
	}

	Anchor->CancelFirstPersonCardDragGesture(true);
	return true;
}

void FWacomRunFirstPersonCardDragController::HandleDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (PlayerController.GetRunFirstPersonCardDetailController()
		.HandleInspectDragStartedOrUpdated(CardInstanceId, DragView))
	{
		HandleInspectDrag(DragView);
		return;
	}

	if (!IsFormalDragGesture(DragView.GestureState))
	{
		return;
	}

	HandleFormalDrag(CardInstanceId, DragView, /*bReleased*/ false);
}

void FWacomRunFirstPersonCardDragController::HandleDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (PlayerController.GetRunFirstPersonCardDetailController()
		.HandleInspectDragStartedOrUpdated(CardInstanceId, DragView))
	{
		HandleInspectDrag(DragView);
		return;
	}

	if (!IsFormalDragGesture(DragView.GestureState))
	{
		return;
	}

	HandleFormalDrag(CardInstanceId, DragView, /*bReleased*/ false);
}

void FWacomRunFirstPersonCardDragController::HandleDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	CameraLookBridge.ClearDragView(
		[this]()
		{
			PlayerController.ClearRunFirstPersonCardDragCameraLookOverride();
		});
	if (IsNeutralGesture(DragView.GestureState))
	{
		if (DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting)
		{
			PlayerController.GetRunFirstPersonCardDetailController().FinishInspectDetail(CardInstanceId);
		}
		PlayerController.GetRunFirstPersonCardDropCoordinator().ClearAllDropProbes();
		return;
	}

	if (!IsFormalDragGesture(DragView.GestureState))
	{
		PlayerController.GetRunFirstPersonCardDropCoordinator().ClearAllDropProbes();
		return;
	}

	HandleFormalDrag(CardInstanceId, DragView, /*bReleased*/ true);
}

void FWacomRunFirstPersonCardDragController::HandleDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	CameraLookBridge.ClearDragView(
		[this]()
		{
			PlayerController.ClearRunFirstPersonCardDragCameraLookOverride();
		});
	if (DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting)
	{
		PlayerController.GetRunFirstPersonCardDetailController().FinishInspectDetail(CardInstanceId);
	}
	PlayerController.GetRunFirstPersonCardDropCoordinator().ClearAllDropProbes();
}

void FWacomRunFirstPersonCardDragController::HandlePointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	CameraLookBridge.HandlePointerMoved(
		PointerView,
		[this](const FWacomFirstPersonCardPointerView& AppliedPointerView)
		{
			PlayerController.ApplyRunFirstPersonCardPointerCameraLookOverride(AppliedPointerView);
		});
}

void FWacomRunFirstPersonCardDragController::HandlePointerLeft()
{
	CameraLookBridge.HandlePointerLeft(
		[this]()
		{
			PlayerController.ClearRunFirstPersonCardDragCameraLookOverride();
		});
}

void FWacomRunFirstPersonCardDragController::UnbindAnchor(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	Anchor.OnFirstPersonCardLayerDragStarted.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerDragUpdated.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerDragReleased.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerDragCancelled.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerPointerMoved.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerPointerLeft.RemoveAll(&PlayerController);
}

void FWacomRunFirstPersonCardDragController::HandleInspectDrag(
	const FWacomFirstPersonCardDragView& DragView)
{
	CameraLookBridge.ApplyDragView(
		DragView,
		[this](const FWacomFirstPersonCardDragView& AppliedDragView)
		{
			PlayerController.ApplyRunFirstPersonCardDragCameraLookOverride(AppliedDragView);
		});
}

void FWacomRunFirstPersonCardDragController::HandleFormalDrag(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	PlayerController.GetRunFirstPersonCardDetailController().HideForSource(CardInstanceId);
	if (!bReleased)
	{
		CameraLookBridge.ApplyDragView(
			DragView,
			[this](const FWacomFirstPersonCardDragView& AppliedDragView)
			{
				PlayerController.ApplyRunFirstPersonCardDragCameraLookOverride(AppliedDragView);
			});
	}

	PlayerController.GetRunFirstPersonCardDropCoordinator()
		.HandleFormalDrag(CardInstanceId, DragView, bReleased);
}

bool FWacomRunFirstPersonCardDragController::IsFormalDragGesture(
	EWacomFirstPersonCardGestureState GestureState)
{
	return GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard
		|| GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit;
}

bool FWacomRunFirstPersonCardDragController::IsNeutralGesture(
	EWacomFirstPersonCardGestureState GestureState)
{
	return GestureState == EWacomFirstPersonCardGestureState::Pressed
		|| GestureState == EWacomFirstPersonCardGestureState::Inspecting;
}
