// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackZonePileWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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

void UWacomBackpackZonePileWidget::SetPreviewWidgetClass(
	TSubclassOf<UWacomBackpackPilePreviewWidget> InClass)
{
	PreviewWidgetClass = InClass;
	RebuildPreviews();
}

void UWacomBackpackZonePileWidget::SetDropPreviewState(bool bVisible, bool bRejected)
{
	bDropPreviewVisible = bVisible;
	bDropPreviewRejected = bRejected;
	if (DropPreviewBorder)
	{
		DropPreviewBorder->SetBrushColor(bRejected
			? FLinearColor(1.0f, 0.12f, 0.08f, 0.78f)
			: FLinearColor(0.12f, 0.85f, 0.42f, 0.72f));
		DropPreviewBorder->SetVisibility(bVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

FReply UWacomBackpackZonePileWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& DragHandle
		&& DragHandle->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition())
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

	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PileSize"));
	RootSize->SetWidthOverride(260.0f);
	RootSize->SetHeightOverride(220.0f);
	WidgetTree->RootWidget = RootSize;
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PileRoot"));
	RootSize->AddChild(Root);

	UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PileBody"));
	Body->SetPadding(FMargin(10.0f));
	Body->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.98f));
	Root->AddChildToOverlay(Body);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PileColumn"));
	Body->AddChild(Column);

	DragHandle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DragHandle"));
	DragHandle->SetPadding(FMargin(8.0f, 6.0f));
	DragHandle->SetBrushColor(FLinearColor(0.08f, 0.13f, 0.19f, 1.0f));
	Column->AddChildToVerticalBox(DragHandle);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	DragHandle->AddChild(Header);
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	if (UHorizontalBoxSlot* HeaderSlot = Header->AddChildToHorizontalBox(TitleText))
	{
		HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
	Header->AddChildToHorizontalBox(CountText);

	PreviewHost = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PreviewHost"));
	if (UVerticalBoxSlot* PreviewSlot = Column->AddChildToVerticalBox(PreviewHost))
	{
		PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(StatusText);

	DropPreviewBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DropPreviewBorder"));
	DropPreviewBorder->SetVisibility(ESlateVisibility::Collapsed);
	DropPreviewBorder->SetPadding(FMargin(4.0f));
	if (UOverlaySlot* PreviewBorderSlot = Root->AddChildToOverlay(DropPreviewBorder))
	{
		PreviewBorderSlot->SetHorizontalAlignment(HAlign_Fill);
		PreviewBorderSlot->SetVerticalAlignment(VAlign_Fill);
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
			: (PileView.bExpanded ? LOCTEXT("CollapseHint", "点击标题收起") : LOCTEXT("ExpandHint", "点击标题展开")));
	}
	if (DragHandle)
	{
		DragHandle->SetBrushColor(PileView.bWarning
			? FLinearColor(0.34f, 0.07f, 0.075f, 1.0f)
			: (PileView.bExpanded
				? FLinearColor(0.08f, 0.38f, 0.52f, 1.0f)
				: FLinearColor(0.08f, 0.13f, 0.19f, 1.0f)));
	}
	RebuildPreviews();
	SetDropPreviewState(bDropPreviewVisible, bDropPreviewRejected);
}

void UWacomBackpackZonePileWidget::RebuildPreviews()
{
	if (!PreviewHost || !WidgetTree)
	{
		return;
	}
	PreviewHost->ClearChildren();
	UClass* ClassToUse = PreviewWidgetClass
		? PreviewWidgetClass.Get()
		: UWacomBackpackPilePreviewWidget::StaticClass();
	const int32 PreviewCount = FMath::Min(3, PileView.PreviewCards.Num());
	for (int32 Index = 0; Index < PreviewCount; ++Index)
	{
		UWacomBackpackPilePreviewWidget* Preview =
			CreateWidget<UWacomBackpackPilePreviewWidget>(this, ClassToUse);
		if (!Preview)
		{
			continue;
		}
		Preview->SetPreviewView(PileView.PreviewCards[Index]);
		Preview->SetRenderTransformAngle((Index - 1) * 4.0f);
		Preview->SetRenderTranslation(FVector2D((Index - 1) * 36.0f, FMath::Abs(Index - 1) * 5.0f));
		if (UOverlaySlot* PreviewSlot = PreviewHost->AddChildToOverlay(Preview))
		{
			PreviewSlot->SetHorizontalAlignment(HAlign_Center);
			PreviewSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

#undef LOCTEXT_NAMESPACE
