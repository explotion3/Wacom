// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
private:
	void UnbindAnchor(UWacomFirstPersonCardAnchorComponent& Anchor);
	void HandleFormalDrag(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	static bool IsFormalDragGesture(EWacomFirstPersonCardGestureState GestureState);
	static bool IsNeutralGesture(EWacomFirstPersonCardGestureState GestureState);

	AWacomPlayerController& PlayerController;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> BoundAnchor;
};
