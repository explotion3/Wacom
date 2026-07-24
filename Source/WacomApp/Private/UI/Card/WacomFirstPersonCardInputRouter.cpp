// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardInputRouter.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Battle/BattleHUD.h"

FWacomFirstPersonCardInputRouter::FWacomFirstPersonCardInputRouter(
	AWacomPlayerController& InPlayerController)
	: PlayerController(InPlayerController)
{
}

bool FWacomFirstPersonCardInputRouter::RouteInput(
	const FWacomFirstPersonCardInputEvent& Input)
{
	const bool bPressed = Input.Event == IE_Pressed;
	if (bPressed
		&& (Input.Key == EKeys::Tab
			|| Input.Key == EKeys::Gamepad_RightShoulder))
	{
		return TryToggleLockedInspection();
	}

	if (bPressed
		&& (Input.Key == EKeys::Escape
			|| Input.Key == EKeys::Gamepad_FaceButton_Right))
	{
		return TryCloseLockedInspection()
			|| TryCancelKeyboardShortcutDrag();
	}

	if (bPressed && Input.Key == EKeys::RightMouseButton)
	{
		return TryCancelKeyboardShortcutDrag();
	}

	if (Input.Key != EKeys::LeftMouseButton)
	{
		return false;
	}

	if (bPressed)
	{
		const bool bViewportPointerAdapter =
			Input.Adapter == EWacomFirstPersonCardInputAdapter::SlatePreprocessor
			|| Input.Adapter == EWacomFirstPersonCardInputAdapter::ViewportReroute;
		return bViewportPointerAdapter
			&& Input.AbsoluteScreenPosition.IsSet()
			&& TryRouteLockedPointerPress(Input.AbsoluteScreenPosition.GetValue());
	}

	if (Input.Event != IE_Released)
	{
		return false;
	}

	if (TryConsumeLockedPointerRelease())
	{
		return true;
	}

	if (Input.Adapter == EWacomFirstPersonCardInputAdapter::SlatePreprocessor
		|| Input.Adapter == EWacomFirstPersonCardInputAdapter::ViewportReroute)
	{
		// Do not steal a captured UMG mouse drag before its owning widget sees
		// release. Only externally driven shortcut drags need this fallback.
		return TryReleaseKeyboardShortcutDrag();
	}

	return TryReleaseActiveDrag();
}

void FWacomFirstPersonCardInputRouter::PumpActivePointer()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	if (!Anchor || !Anchor->IsFirstPersonCardDragGestureActive())
	{
		return;
	}

	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (PlayerController.TryGetMouseWidgetPosition(WidgetPosition))
	{
		Anchor->UpdateFirstPersonCardDragPointer(WidgetPosition);
	}
}

bool FWacomFirstPersonCardInputRouter::TryStartBattleHandShortcut(
	int32 OneBasedIndex)
{
	UBattleHUD* HUD = PlayerController.GetActiveBattleHUD();
	if (!HUD)
	{
		return false;
	}

	TOptional<FVector2D> PointerWidgetPosition;
	FVector2D MouseWidgetPosition = FVector2D::ZeroVector;
	if (PlayerController.TryGetMouseWidgetPosition(MouseWidgetPosition))
	{
		PointerWidgetPosition = MouseWidgetPosition;
	}
	return HUD->TryStartFirstPersonBattleHandDragByIndex(
		OneBasedIndex,
		PointerWidgetPosition);
}

bool FWacomFirstPersonCardInputRouter::TryCancelForTurnBoundary()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	if (!Anchor || !Anchor->IsFirstPersonCardDragGestureActive())
	{
		return false;
	}

	Anchor->CancelFirstPersonCardDragGesture(true);
	return true;
}

void FWacomFirstPersonCardInputRouter::ResetTransientState(
	bool bBroadcastCancel)
{
	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor())
	{
		Anchor->CancelFirstPersonCardDragGesture(bBroadcastCancel);
		Anchor->ConsumePendingFirstPersonCardLockedInspectionPointerRelease();
	}
}

UWacomFirstPersonCardAnchorComponent*
FWacomFirstPersonCardInputRouter::ResolveAnchor() const
{
	return PlayerController.ResolveFirstPersonCardAnchor();
}

bool FWacomFirstPersonCardInputRouter::TryToggleLockedInspection()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	return Anchor && Anchor->TryToggleFirstPersonCardLockedFace();
}

bool FWacomFirstPersonCardInputRouter::TryCloseLockedInspection()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	return Anchor && Anchor->TryCloseFirstPersonCardLockedInspection();
}

bool FWacomFirstPersonCardInputRouter::TryRouteLockedPointerPress(
	const FVector2D& AbsoluteScreenPosition)
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	return Anchor
		&& Anchor->TryRouteFirstPersonCardLockedInspectionPointerPress(
			AbsoluteScreenPosition);
}

bool FWacomFirstPersonCardInputRouter::TryConsumeLockedPointerRelease()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	return Anchor
		&& Anchor->ConsumePendingFirstPersonCardLockedInspectionPointerRelease();
}

bool FWacomFirstPersonCardInputRouter::TryReleaseActiveDrag()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	if (!Anchor || !Anchor->IsFirstPersonCardDragGestureActive())
	{
		return false;
	}

	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (PlayerController.TryGetMouseWidgetPosition(WidgetPosition))
	{
		Anchor->UpdateFirstPersonCardDragPointer(WidgetPosition);
		return Anchor->ReleaseFirstPersonCardDragGesture(WidgetPosition);
	}

	return Anchor->ReleaseFirstPersonCardDragGestureAtCurrentPointer();
}

bool FWacomFirstPersonCardInputRouter::TryReleaseKeyboardShortcutDrag()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	return Anchor && Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive()
		&& TryReleaseActiveDrag();
}

bool FWacomFirstPersonCardInputRouter::TryCancelKeyboardShortcutDrag()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveAnchor();
	if (!Anchor
		|| !Anchor->IsFirstPersonCardKeyboardShortcutDragGestureActive())
	{
		return false;
	}

	Anchor->CancelFirstPersonCardDragGesture(true);
	return true;
}
