// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardCameraLookBridge.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class AWacomPlayerController;
class UWacomFirstPersonCardAnchorComponent;

class FWacomRunFirstPersonCardDragController
{
public:
	explicit FWacomRunFirstPersonCardDragController(AWacomPlayerController& InPlayerController);

	void RefreshBinding();
	void UnbindCurrentBinding();

	void PumpActiveDragPointer();
	bool TryReleaseActiveDragPointer();
	bool TryCancelActiveGestureForTurnBoundaryShortcut();

	void HandleDragStarted(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	void HandleDragUpdated(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	void HandleDragReleased(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	void HandleDragCancelled(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	void HandlePointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandlePointerLeft();

private:
	void UnbindAnchor(UWacomFirstPersonCardAnchorComponent& Anchor);
	void HandleInspectDrag(const FWacomFirstPersonCardDragView& DragView);
	void HandleFormalDrag(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	static bool IsFormalDragGesture(EWacomFirstPersonCardGestureState GestureState);
	static bool IsNeutralGesture(EWacomFirstPersonCardGestureState GestureState);

	AWacomPlayerController& PlayerController;
	FWacomFirstPersonCardCameraLookBridge CameraLookBridge;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> BoundAnchor;
};
