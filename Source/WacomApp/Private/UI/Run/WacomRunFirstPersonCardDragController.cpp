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
		BoundAnchor = Anchor;
	}
}

void FWacomRunFirstPersonCardDragController::UnbindCurrentBinding()
{
	if (UWacomFirstPersonCardAnchorComponent* Anchor = BoundAnchor.Get())
	{
		UnbindAnchor(*Anchor);
	}
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

bool FWacomRunFirstPersonCardDragController::TryCancelKeyboardShortcutActiveDrag()
{
	UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	if (!Anchor || !Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive())
	{
		return false;
	}

	Anchor->CancelFirstPersonCardDragGesture(true);
	return true;
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
	if (IsNeutralGesture(DragView.GestureState))
	{
		if (DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting)
		{
			PlayerController.GetRunFirstPersonCardDetailController()
				.FinishInspectDetail(CardInstanceId);
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
	if (DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting)
	{
		PlayerController.GetRunFirstPersonCardDetailController()
			.FinishInspectDetail(CardInstanceId);
	}
	PlayerController.GetRunFirstPersonCardDropCoordinator().ClearAllDropProbes();
}

void FWacomRunFirstPersonCardDragController::UnbindAnchor(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	Anchor.OnFirstPersonCardLayerDragStarted.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerDragUpdated.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerDragReleased.RemoveAll(&PlayerController);
	Anchor.OnFirstPersonCardLayerDragCancelled.RemoveAll(&PlayerController);
}

void FWacomRunFirstPersonCardDragController::HandleFormalDrag(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	PlayerController.GetRunFirstPersonCardDetailController().HideForSource(CardInstanceId);

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
