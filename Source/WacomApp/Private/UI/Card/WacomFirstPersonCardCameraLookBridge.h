// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomFirstPersonCardCameraLookBridge
{
public:
	bool IsDragCameraLookActive() const { return bDragCameraLookActive; }

	void ApplyDragView(
		const FWacomFirstPersonCardDragView& DragView,
		TFunctionRef<void(const FWacomFirstPersonCardDragView&)> ApplyDragCameraLook);
	void ClearDragView(TFunctionRef<void()> ClearCameraLook);
	void HandlePointerMoved(
		const FWacomFirstPersonCardPointerView& PointerView,
		TFunctionRef<void(const FWacomFirstPersonCardPointerView&)> ApplyPointerCameraLook);
	void HandlePointerLeft(TFunctionRef<void()> ClearCameraLook);

private:
	bool bDragCameraLookActive = false;
};
