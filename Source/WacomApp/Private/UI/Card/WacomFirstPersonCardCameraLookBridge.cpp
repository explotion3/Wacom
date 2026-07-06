// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardCameraLookBridge.h"

void FWacomFirstPersonCardCameraLookBridge::ApplyDragView(
	const FWacomFirstPersonCardDragView& DragView,
	TFunctionRef<void(const FWacomFirstPersonCardDragView&)> ApplyDragCameraLook)
{
	bDragCameraLookActive = true;
	ApplyDragCameraLook(DragView);
}

void FWacomFirstPersonCardCameraLookBridge::ClearDragView(
	TFunctionRef<void()> ClearCameraLook)
{
	bDragCameraLookActive = false;
	ClearCameraLook();
}

void FWacomFirstPersonCardCameraLookBridge::HandlePointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView,
	TFunctionRef<void(const FWacomFirstPersonCardPointerView&)> ApplyPointerCameraLook)
{
	if (bDragCameraLookActive)
	{
		return;
	}

	ApplyPointerCameraLook(PointerView);
}

void FWacomFirstPersonCardCameraLookBridge::HandlePointerLeft(
	TFunctionRef<void()> ClearCameraLook)
{
	if (bDragCameraLookActive)
	{
		return;
	}

	ClearCameraLook();
}
