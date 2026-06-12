// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class UWacomFirstPersonCardLayerWidget;

struct FWacomFirstPersonCardLayerDelegateRouterCallbacks
{
	TFunction<void(const FGuid&, const FWacomFirstPersonCardLayerSlotView&)> CardHovered;
	TFunction<void(const FGuid&, const FWacomFirstPersonCardLayerSlotView&)> CardUnhovered;
	TFunction<void(const FGuid&, const FWacomFirstPersonCardLayerSlotView&)> HoveredCardLayoutUpdated;
	TFunction<void(const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&)> CardTargetHovered;
	TFunction<void(const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&)> CardTargetUnhovered;
	TFunction<void(const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&)> HoveredCardTargetUpdated;
	TFunction<void(const FGuid&, const FWacomFirstPersonCardDragView&)> DragStarted;
	TFunction<void(const FGuid&, const FWacomFirstPersonCardDragView&)> DragUpdated;
	TFunction<void(const FGuid&, const FWacomFirstPersonCardDragView&)> DragReleased;
	TFunction<void(const FGuid&, const FWacomFirstPersonCardDragView&)> DragCancelled;
	TFunction<void(const FWacomFirstPersonCardPointerView&)> PointerMoved;
	TFunction<void()> PointerLeft;
	TFunction<FGuid()> GetHoveredCardInstanceId;
	TFunction<void(const FGuid&)> SetHoveredCardInstanceId;
	TFunction<void()> ClearHoveredCardInstanceId;
	TFunction<FWacomInteractionTargetHandle()> GetHoveredCardTargetHandle;
	TFunction<void(const FWacomInteractionTargetHandle&)> SetHoveredCardTargetHandle;
	TFunction<void()> ClearHoveredCardTargetHandle;
};

class FWacomFirstPersonCardLayerDelegateRouter
{
public:
	void SetCallbacks(FWacomFirstPersonCardLayerDelegateRouterCallbacks InCallbacks);
	void Bind(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void Unbind(UWacomFirstPersonCardLayerWidget* LayerWidget);

private:
	void HandleCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleHoveredCardSlotUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardTargetHovered(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardTargetUnhovered(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleHoveredCardTargetUpdated(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandlePointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandlePointerLeft();

	FWacomFirstPersonCardLayerDelegateRouterCallbacks Callbacks;
};
