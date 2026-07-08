// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomZoneDropTarget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"

TSharedRef<SWidget> UWacomZoneDropTarget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DropTargetRoot"));
		RootBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
		RootBorder->SetPadding(FMargin(0.f));
		WidgetTree->RootWidget = RootBorder;
	}

	if (RootBorder && DropContent && DropContent->GetParent() != RootBorder)
	{
		RootBorder->ClearChildren();
		RootBorder->AddChild(DropContent);
	}

	return Super::RebuildWidget();
}

void UWacomZoneDropTarget::Configure(EZoneKind InZoneKind, FGuid InOwnerInstanceId)
{
	ZoneKind = InZoneKind;
	OwnerInstanceId = (ZoneKind == EZoneKind::SpecialZone) ? InOwnerInstanceId : FGuid();
}

void UWacomZoneDropTarget::SetOwnerScreen(UWacomBackpackScreen* InScreen)
{
	OwnerScreen = InScreen;
}

void UWacomZoneDropTarget::SetDropContent(UWidget* InContent)
{
	DropContent = InContent;
	if (RootBorder)
	{
		RootBorder->ClearChildren();
		if (DropContent)
		{
			RootBorder->AddChild(DropContent);
		}
	}
}

void UWacomZoneDropTarget::SetDropTargetState(EWacomDropTargetState InState)
{
	if (DropTargetState == InState)
	{
		return;
	}

	DropTargetState = InState;
	BP_OnDropTargetStateChanged(DropTargetState);
}

bool UWacomZoneDropTarget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	if (!CardOp || !CardOp->InstanceId.IsValid() || !Screen)
	{
		SetDropTargetState(EWacomDropTargetState::HoverInvalid);
		return false;
	}

	const bool bCanPreview = Screen->CanPreviewZoneDrop(*CardOp, ZoneKind, OwnerInstanceId);
	SetDropTargetState(bCanPreview ? EWacomDropTargetState::HoverValid : EWacomDropTargetState::HoverInvalid);
	return true;
}

void UWacomZoneDropTarget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	SetDropTargetState(EWacomDropTargetState::Normal);
}

bool UWacomZoneDropTarget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const bool bHandled = TryHandleDropOperation(InOperation);
	SetDropTargetState(bHandled ? EWacomDropTargetState::DropAccepted : EWacomDropTargetState::DropRejected);
	return bHandled;
}

bool UWacomZoneDropTarget::TryHandleDropOperation(UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	if (!CardOp || !CardOp->InstanceId.IsValid())
	{
		return false;
	}

	UWacomBackpackScreen* Screen = OwnerScreen.Get();
	if (!Screen)
	{
		return false;
	}

	return Screen->HandleZoneDropRequested(*CardOp, ZoneKind, OwnerInstanceId);
}
