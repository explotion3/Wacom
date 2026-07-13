// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerDelegateRouter.h"

#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

void FWacomFirstPersonCardLayerDelegateRouter::SetCallbacks(
	FWacomFirstPersonCardLayerDelegateRouterCallbacks InCallbacks)
{
	Callbacks = MoveTemp(InCallbacks);
}

void FWacomFirstPersonCardLayerDelegateRouter::Bind(UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (!LayerWidget)
	{
		return;
	}

	Unbind(LayerWidget);
	LayerWidget->OnCardHoveredNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandleCardHovered);
	LayerWidget->OnCardUnhoveredNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandleCardUnhovered);
	LayerWidget->OnHoveredCardSlotUpdatedNative.AddRaw(
		this,
		&FWacomFirstPersonCardLayerDelegateRouter::HandleHoveredCardSlotUpdated);
	LayerWidget->OnCardTargetHoveredNative.AddRaw(
		this,
		&FWacomFirstPersonCardLayerDelegateRouter::HandleCardTargetHovered);
	LayerWidget->OnCardTargetUnhoveredNative.AddRaw(
		this,
		&FWacomFirstPersonCardLayerDelegateRouter::HandleCardTargetUnhovered);
	LayerWidget->OnHoveredCardTargetUpdatedNative.AddRaw(
		this,
		&FWacomFirstPersonCardLayerDelegateRouter::HandleHoveredCardTargetUpdated);
	LayerWidget->OnCardDragStartedNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandleDragStarted);
	LayerWidget->OnCardDragUpdatedNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandleDragUpdated);
	LayerWidget->OnCardDragReleasedNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandleDragReleased);
	LayerWidget->OnCardDragCancelledNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandleDragCancelled);
	LayerWidget->OnCardPointerMovedNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandlePointerMoved);
	LayerWidget->OnCardPointerLeftNative.AddRaw(this, &FWacomFirstPersonCardLayerDelegateRouter::HandlePointerLeft);
	LayerWidget->OnEnterTransitionStartedNative.AddRaw(
		this,
		&FWacomFirstPersonCardLayerDelegateRouter::HandleEnterTransitionStarted);
	LayerWidget->OnPileTransferProgressNative.AddRaw(
		this,
		&FWacomFirstPersonCardLayerDelegateRouter::HandlePileTransferProgress);
}

void FWacomFirstPersonCardLayerDelegateRouter::Unbind(UWacomFirstPersonCardLayerWidget* LayerWidget)
{
	if (!LayerWidget)
	{
		return;
	}

	LayerWidget->OnCardHoveredNative.RemoveAll(this);
	LayerWidget->OnCardUnhoveredNative.RemoveAll(this);
	LayerWidget->OnHoveredCardSlotUpdatedNative.RemoveAll(this);
	LayerWidget->OnCardTargetHoveredNative.RemoveAll(this);
	LayerWidget->OnCardTargetUnhoveredNative.RemoveAll(this);
	LayerWidget->OnHoveredCardTargetUpdatedNative.RemoveAll(this);
	LayerWidget->OnCardDragStartedNative.RemoveAll(this);
	LayerWidget->OnCardDragUpdatedNative.RemoveAll(this);
	LayerWidget->OnCardDragReleasedNative.RemoveAll(this);
	LayerWidget->OnCardDragCancelledNative.RemoveAll(this);
	LayerWidget->OnCardPointerMovedNative.RemoveAll(this);
	LayerWidget->OnCardPointerLeftNative.RemoveAll(this);
	LayerWidget->OnEnterTransitionStartedNative.RemoveAll(this);
	LayerWidget->OnPileTransferProgressNative.RemoveAll(this);
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (Callbacks.SetHoveredCardInstanceId)
	{
		Callbacks.SetHoveredCardInstanceId(CardInstanceId);
	}
	if (Callbacks.CardHovered)
	{
		Callbacks.CardHovered(CardInstanceId, SlotView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (Callbacks.GetHoveredCardInstanceId
		&& Callbacks.GetHoveredCardInstanceId() == CardInstanceId)
	{
		if (Callbacks.ClearHoveredCardInstanceId)
		{
			Callbacks.ClearHoveredCardInstanceId();
		}
		if (Callbacks.ClearHoveredCardTargetHandle)
		{
			Callbacks.ClearHoveredCardTargetHandle();
		}
	}
	if (Callbacks.CardUnhovered)
	{
		Callbacks.CardUnhovered(CardInstanceId, SlotView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleHoveredCardSlotUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (Callbacks.HoveredCardLayoutUpdated)
	{
		Callbacks.HoveredCardLayoutUpdated(CardInstanceId, SlotView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleCardTargetHovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!CardTargetHandle.IsValid())
	{
		return;
	}

	if (Callbacks.SetHoveredCardTargetHandle)
	{
		Callbacks.SetHoveredCardTargetHandle(CardTargetHandle);
	}
	if (Callbacks.CardTargetHovered)
	{
		Callbacks.CardTargetHovered(CardTargetHandle, SlotView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleCardTargetUnhovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (Callbacks.GetHoveredCardTargetHandle)
	{
		const FWacomInteractionTargetHandle HoveredHandle = Callbacks.GetHoveredCardTargetHandle();
		if (HoveredHandle.IsValid()
			&& HoveredHandle.CardInstanceId == CardTargetHandle.CardInstanceId
			&& Callbacks.ClearHoveredCardTargetHandle)
		{
			Callbacks.ClearHoveredCardTargetHandle();
		}
	}
	if (Callbacks.CardTargetUnhovered)
	{
		Callbacks.CardTargetUnhovered(CardTargetHandle, SlotView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleHoveredCardTargetUpdated(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!CardTargetHandle.IsValid())
	{
		return;
	}

	if (Callbacks.SetHoveredCardTargetHandle)
	{
		Callbacks.SetHoveredCardTargetHandle(CardTargetHandle);
	}
	if (Callbacks.HoveredCardTargetUpdated)
	{
		Callbacks.HoveredCardTargetUpdated(CardTargetHandle, SlotView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (Callbacks.DragStarted)
	{
		Callbacks.DragStarted(CardInstanceId, DragView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (Callbacks.DragUpdated)
	{
		Callbacks.DragUpdated(CardInstanceId, DragView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (Callbacks.DragReleased)
	{
		Callbacks.DragReleased(CardInstanceId, DragView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (Callbacks.DragCancelled)
	{
		Callbacks.DragCancelled(CardInstanceId, DragView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandlePointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	if (Callbacks.PointerMoved)
	{
		Callbacks.PointerMoved(PointerView);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandlePointerLeft()
{
	if (Callbacks.PointerLeft)
	{
		Callbacks.PointerLeft();
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandleEnterTransitionStarted(
	const FWacomFirstPersonCardEnterTransitionStartedView& View)
{
	if (Callbacks.EnterTransitionStarted)
	{
		Callbacks.EnterTransitionStarted(View);
	}
}

void FWacomFirstPersonCardLayerDelegateRouter::HandlePileTransferProgress(
	const FWacomFirstPersonCardPileTransferProgressView& Progress)
{
	if (Callbacks.PileTransferProgress)
	{
		Callbacks.PileTransferProgress(Progress);
	}
}
