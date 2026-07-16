// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackZonePileWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "WacomBackpackZonePile"

TSharedRef<SWidget> UWacomBackpackZonePileWidget::RebuildWidget()
{
	EnsureFallbackTree();
	ApplyView();
	return Super::RebuildWidget();
}

void UWacomBackpackZonePileWidget::SetPileView(const FWacomBackpackZonePileView& InView)
{
	PileView = InView;
	ApplyView();
}

void UWacomBackpackZonePileWidget::SetResolvedGeometry(
	const FSlateRect& InFrameRect,
	const FSlateRect& InHeaderRect)
{
	ResolvedFrameRect = InFrameRect;
	ResolvedHeaderRect = InHeaderRect;
	if (UCanvasPanelSlot* HeaderSlot = DragHandle
		? Cast<UCanvasPanelSlot>(DragHandle->Slot)
		: nullptr)
	{
		HeaderSlot->SetPosition(FVector2D(
			InHeaderRect.Left - InFrameRect.Left,
			InHeaderRect.Top - InFrameRect.Top));
		HeaderSlot->SetSize(FVector2D(
			InHeaderRect.Right - InHeaderRect.Left,
			InHeaderRect.Bottom - InHeaderRect.Top));
	}
}

void UWacomBackpackZonePileWidget::SetDropPreviewState(bool bVisible, bool bRejected)
{
	bDropPreviewVisible = bVisible;
	bDropPreviewRejected = bRejected;
	if (DropFeedback)
	{
		DropFeedback->SetBrushColor(bRejected
			? FLinearColor(1.0f, 0.12f, 0.08f, 0.78f)
			: FLinearColor(0.12f, 0.85f, 0.42f, 0.72f));
		DropFeedback->SetVisibility(bVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

FReply UWacomBackpackZonePileWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	bLastPointerDownOnDragHandle = DragHandle
		&& DragHandle->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& OnPilePointerDownNative.IsBound())
	{
		return OnPilePointerDownNative.Execute(this, InGeometry, InMouseEvent);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWacomBackpackZonePileWidget::EnsureFallbackTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("PileRoot"));
	WidgetTree->RootWidget = Root;

	FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameBorder"));
	FrameBorder->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.74f));
	FrameBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(FrameBorder))
	{
		FrameSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FrameSlot->SetOffsets(FMargin(0.0f));
	}

	DragHandle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DragHandle"));
	DragHandle->SetPadding(FMargin(8.0f, 6.0f));
	DragHandle->SetBrushColor(FLinearColor(0.08f, 0.13f, 0.19f, 1.0f));
	if (UCanvasPanelSlot* HeaderSlot = Root->AddChildToCanvas(DragHandle))
	{
		HeaderSlot->SetPosition(FVector2D::ZeroVector);
		HeaderSlot->SetSize(FVector2D(260.0f, 48.0f));
		HeaderSlot->SetZOrder(2);
	}
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("Header"));
	DragHandle->AddChild(Header);
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	if (UHorizontalBoxSlot* HeaderSlot = Header->AddChildToHorizontalBox(TitleText))
	{
		HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
	Header->AddChildToHorizontalBox(CountText);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* StatusSlot = Root->AddChildToCanvas(StatusText))
	{
		StatusSlot->SetAnchors(FAnchors(0.0f, 1.0f, 1.0f, 1.0f));
		StatusSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		StatusSlot->SetOffsets(FMargin(8.0f, -32.0f, 8.0f, 28.0f));
		StatusSlot->SetZOrder(2);
	}

	DropFeedback = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("DropFeedback"));
	DropFeedback->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* DropSlot = Root->AddChildToCanvas(DropFeedback))
	{
		DropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DropSlot->SetOffsets(FMargin(0.0f));
		DropSlot->SetZOrder(3);
	}
}

void UWacomBackpackZonePileWidget::ApplyView()
{
	if (TitleText)
	{
		TitleText->SetText(PileView.Title);
	}
	if (CountText)
	{
		FText Count = PileView.bHasCapacity
			? FText::Format(LOCTEXT("CapacityCount", "{0}/{1}"),
				FText::AsNumber(PileView.CardCount), FText::AsNumber(PileView.Capacity))
			: FText::AsNumber(PileView.CardCount);
		if (PileView.ProjectedCount > 0)
		{
			Count = FText::Format(LOCTEXT("ProjectedCount", "{0}  投影+{1}"),
				Count, FText::AsNumber(PileView.ProjectedCount));
		}
		CountText->SetText(Count);
	}
	if (StatusText)
	{
		StatusText->SetText(PileView.bWarning
			? LOCTEXT("BurdenWarning", "负重警告")
			: (PileView.bExpanded
				? LOCTEXT("CollapseHint", "点击标题收起")
				: LOCTEXT("ExpandHint", "点击牌堆展开")));
	}
	if (DragHandle)
	{
		DragHandle->SetBrushColor(PileView.bWarning
			? FLinearColor(0.34f, 0.07f, 0.075f, 1.0f)
			: (PileView.bExpanded
				? FLinearColor(0.08f, 0.38f, 0.52f, 1.0f)
				: FLinearColor(0.08f, 0.13f, 0.19f, 1.0f)));
	}
	SetDropPreviewState(bDropPreviewVisible, bDropPreviewRejected);
}

#undef LOCTEXT_NAMESPACE
