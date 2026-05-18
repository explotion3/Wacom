// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomZoneDropTarget.h"

#include "Blueprint/DragDropOperation.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"

void UWacomZoneDropTarget::Configure(EZoneKind InZoneKind, FGuid InOwnerInstanceId)
{
	ZoneKind = InZoneKind;
	OwnerInstanceId = (ZoneKind == EZoneKind::SpecialZone) ? InOwnerInstanceId : FGuid();
}

void UWacomZoneDropTarget::SetOwnerScreen(UWacomBackpackScreen* InScreen)
{
	OwnerScreen = InScreen;
}

bool UWacomZoneDropTarget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!Cast<UWacomCardDragOperation>(InOperation))
	{
		return false;
	}

	return OwnerScreen.IsValid() && OwnerScreen->GetRunSession() != nullptr;
}

bool UWacomZoneDropTarget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	if (!CardOp || !CardOp->InstanceId.IsValid())
	{
		return false;
	}

	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	URunSession* Run = Screen ? Screen->GetRunSession() : nullptr;
	if (!Run)
	{
		return false;
	}

	return Run->MoveInstance(CardOp->InstanceId, ZoneKind, OwnerInstanceId);
}
