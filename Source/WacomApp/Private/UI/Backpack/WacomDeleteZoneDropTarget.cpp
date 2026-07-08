// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomDeleteZoneDropTarget.h"

#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"

bool UWacomDeleteZoneDropTarget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	if (!CardOp || !CardOp->InstanceId.IsValid() || !Screen)
	{
		SetDropTargetState(EWacomDropTargetState::HoverInvalid);
		return false;
	}
	const bool bCanDrop = Screen->CanPreviewDeleteDrop(*CardOp);
	SetDropTargetState(bCanDrop ? EWacomDropTargetState::HoverValid : EWacomDropTargetState::HoverInvalid);
	return true;
}

bool UWacomDeleteZoneDropTarget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	if (!CardOp || !CardOp->InstanceId.IsValid() || !Screen)
	{
		SetDropTargetState(EWacomDropTargetState::Normal);
		return false;
	}

	const bool bDialogShown = Screen->HandleDeleteDropRequested(*CardOp);
	SetDropTargetState(bDialogShown ? EWacomDropTargetState::ConfirmPending : EWacomDropTargetState::Normal);
	return bDialogShown;
}
