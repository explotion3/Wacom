// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomZoneDropTarget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "RunSession.h"
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

bool UWacomZoneDropTarget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UWacomCardDragOperation* CardOp = Cast<UWacomCardDragOperation>(InOperation);
	if (!CardOp)
	{
		return false;
	}

	return CanPreviewDrop(*CardOp);
}

bool UWacomZoneDropTarget::CanPreviewDrop(const UWacomCardDragOperation& CardOp) const
{
	URunSession* Run = OwnerScreen.IsValid() ? OwnerScreen->GetRunSession() : nullptr;
	if (!Run)
	{
		return false;
	}

	if (!ShouldPreviewDrop(ZoneKind, CardOp.FromZone, Run->GetBattleDeck().Num(), Run->GetBattleDeckCapacity()))
	{
		return false;
	}

	return true;
}

bool UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind TargetZone, EZoneKind SourceZone, int32 BattleDeckCount, int32 BattleDeckCapacity)
{
	if (TargetZone == EZoneKind::BattleDeck
		&& SourceZone == EZoneKind::Backpack
		&& BattleDeckCount >= BattleDeckCapacity)
	{
		// 视觉预判：从通量区拖入已满备战区时拒绝接收。
		// 最终规则仍以 NativeOnDrop 调用 RunSession::MoveInstance 的返回值为准。
		return false;
	}

	return true;
}

bool UWacomZoneDropTarget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	return TryHandleDropOperation(InOperation);
}

bool UWacomZoneDropTarget::TryHandleDropOperation(UDragDropOperation* InOperation)
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

#undef LOCTEXT_NAMESPACE
