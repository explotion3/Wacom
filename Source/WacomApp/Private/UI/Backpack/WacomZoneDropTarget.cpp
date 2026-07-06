// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomZoneDropTarget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"

#define LOCTEXT_NAMESPACE "WacomZoneDropTarget"

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

FText UWacomZoneDropTarget::FormatZoneNameForToast(EZoneKind Zone)
{
	switch (Zone)
	{
	case EZoneKind::Backpack:
		return LOCTEXT("ZoneBackpack", "通量区");
	case EZoneKind::BattleDeck:
		return LOCTEXT("ZoneBattleDeck", "备战区");
	case EZoneKind::SpecialZone:
		return LOCTEXT("ZoneSpecialZone", "特殊存放区");
	case EZoneKind::BurdenZone:
		return LOCTEXT("ZoneBurden", "负重区");
	default:
		return LOCTEXT("ZoneUnknown", "未知区域");
	}
}

FText UWacomZoneDropTarget::FormatMoveFailureReasonForToast(FName DisabledReason)
{
	if (DisabledReason == TEXT("CardNotFound"))
	{
		return LOCTEXT("MoveFailCardNotFound", "无法移动：找不到这张卡牌。");
	}
	if (DisabledReason == TEXT("FluxFull"))
	{
		return LOCTEXT("MoveFailFluxFull", "无法移动：通量区已满。");
	}
	if (DisabledReason == TEXT("BattleDeckFull"))
	{
		return LOCTEXT("MoveFailBattleDeckFull", "无法移动：备战区已满。");
	}
	if (DisabledReason == TEXT("SpecialZoneMissing"))
	{
		return LOCTEXT("MoveFailSpecialZoneMissing", "无法移动：目标特殊存放区不存在。");
	}
	if (DisabledReason == TEXT("SelfSpecialZone"))
	{
		return LOCTEXT("MoveFailSelfSpecialZone", "无法移动：主卡不能放进自己的特殊存放区。");
	}
	if (DisabledReason == TEXT("TypeBInSpecialZone"))
	{
		return LOCTEXT("MoveFailTypeBInSpecialZone", "无法移动：特殊存放区不能收纳另一张主卡。");
	}
	if (DisabledReason == TEXT("SpecialZoneFull"))
	{
		return LOCTEXT("MoveFailSpecialZoneFull", "无法移动：特殊存放区已满。");
	}
	if (DisabledReason == TEXT("TypeBInBurdenZone"))
	{
		return LOCTEXT("MoveFailTypeBInBurden", "无法移动：主卡不能进入负重区。");
	}
	if (DisabledReason == TEXT("InvalidTargetZone"))
	{
		return LOCTEXT("MoveFailInvalidTarget", "无法移动：目标区域无效。");
	}
	return LOCTEXT("MoveFailUnknown", "无法移动：当前规则不允许。");
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

#undef LOCTEXT_NAMESPACE
