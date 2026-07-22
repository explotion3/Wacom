// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackZonePileWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"

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

void UWacomBackpackZonePileWidget::SetVisualStyle(UWacomBackpackWorkspaceStyle* InStyle)
{
	VisualStyle = InStyle;
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

void UWacomBackpackZonePileWidget::SetNavigationFocused(bool bFocused)
{
	bNavigationFocused = bFocused;
	if (NavigationFocusIcon)
	{
		const UWacomBackpackWorkspaceStyle* Style = VisualStyle.IsValid()
			? VisualStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		NavigationFocusIcon->SetBrush(Style->FocusStateIconBrush);
		NavigationFocusIcon->SetVisibility(bNavigationFocused
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UWacomBackpackZonePileWidget::SetDropFeedbackView(
	const FWacomBackpackDropFeedbackView& InView)
{
	DropFeedbackView = InView;
	ApplyDropFeedback();
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

	AccentStrip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AccentStrip"));
	AccentStrip->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* AccentSlot = Root->AddChildToCanvas(AccentStrip))
	{
		AccentSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
		AccentSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		AccentSlot->SetZOrder(1);
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
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("ZoneIconSize"));
	IconSize->SetWidthOverride(26.0f);
	IconSize->SetHeightOverride(26.0f);
	if (UHorizontalBoxSlot* IconSlot = Header->AddChildToHorizontalBox(IconSize))
	{
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}
	ZoneIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ZoneIcon"));
	ZoneIcon->SetVisibility(ESlateVisibility::Collapsed);
	IconSize->AddChild(ZoneIcon);
	NavigationFocusIcon = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("NavigationFocusIcon"));
	NavigationFocusIcon->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* FocusSlot = Root->AddChildToCanvas(NavigationFocusIcon))
	{
		FocusSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		FocusSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		FocusSlot->SetPosition(FVector2D(-8.0f, 8.0f));
		FocusSlot->SetSize(FVector2D(32.0f, 32.0f));
		FocusSlot->SetZOrder(5);
	}
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	if (UHorizontalBoxSlot* HeaderSlot = Header->AddChildToHorizontalBox(TitleText))
	{
		HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	CountBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CountBadge"));
	CountBadge->SetPadding(FMargin(8.0f, 2.0f));
	if (UHorizontalBoxSlot* BadgeSlot = Header->AddChildToHorizontalBox(CountBadge))
	{
		BadgeSlot->SetVerticalAlignment(VAlign_Center);
	}
	CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
	CountBadge->AddChild(CountText);

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
	UVerticalBox* FeedbackContent = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("DropFeedbackContent"));
	DropFeedback->SetContent(FeedbackContent);
	DropFeedbackText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("DropFeedbackText"));
	DropFeedbackText->SetJustification(ETextJustify::Center);
	DropFeedbackText->SetAutoWrapText(true);
	FeedbackContent->AddChildToVerticalBox(DropFeedbackText);
	DropFeedbackCountText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("DropFeedbackCountText"));
	DropFeedbackCountText->SetJustification(ETextJustify::Center);
	FeedbackContent->AddChildToVerticalBox(DropFeedbackCountText);
}

void UWacomBackpackZonePileWidget::ApplyView()
{
	const UWacomBackpackWorkspaceStyle* Style = VisualStyle.IsValid()
		? VisualStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackZoneAppearance& Appearance = Style->ResolveZoneAppearance(PileView.Zone);
	if (FrameBorder)
	{
		if (Appearance.FrameBrush.GetResourceObject())
		{
			FrameBorder->SetBrush(Appearance.FrameBrush);
			FrameBorder->SetBrushColor(Appearance.AccentColor);
		}
		else
		{
			FrameBorder->SetBrushColor(Appearance.SurfaceColor);
		}
		FrameBorder->SetRenderOpacity(PileView.bExpanded
			? 1.0f
			: FMath::Clamp(Style->InactivePileFrameOpacity, 0.0f, 1.0f));
	}
	if (AccentStrip)
	{
		AccentStrip->SetBrushColor(Appearance.AccentColor);
	}
	if (ZoneIcon)
	{
		const bool bHasIcon = Appearance.IconBrush.GetResourceObject() != nullptr;
		if (bHasIcon)
		{
			ZoneIcon->SetBrush(Appearance.IconBrush);
			ZoneIcon->SetColorAndOpacity(Appearance.AccentColor);
		}
		ZoneIcon->SetVisibility(bHasIcon
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (NavigationFocusIcon)
	{
		NavigationFocusIcon->SetBrush(Style->FocusStateIconBrush);
		NavigationFocusIcon->SetVisibility(bNavigationFocused
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
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
		CountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.96f, 0.98f, 1.0f)));
	}
	if (CountBadge)
	{
		FLinearColor BadgeColor = Appearance.AccentColor;
		BadgeColor.A = PileView.bExpanded ? 0.34f : 0.22f;
		CountBadge->SetBrushColor(BadgeColor);
	}
	if (StatusText)
	{
		const bool bShowStatus = PileView.bWarning || PileView.CardCount <= 0;
		StatusText->SetText(PileView.bWarning
			? LOCTEXT("BurdenWarning", "负重警告")
			: LOCTEXT("EmptyPile", "暂无卡牌"));
		StatusText->SetVisibility(bShowStatus
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DragHandle)
	{
		FLinearColor HeaderColor = Appearance.SurfaceColor;
		if (PileView.bExpanded)
		{
			HeaderColor = FMath::Lerp(HeaderColor, Appearance.AccentColor, 0.28f);
		}
		if (PileView.bWarning)
		{
			HeaderColor = FMath::Lerp(HeaderColor, Style->RejectedTargetColor, 0.32f);
		}
		HeaderColor.A = 1.0f;
		DragHandle->SetBrushColor(HeaderColor);
	}
	ApplyDropFeedback();
}

void UWacomBackpackZonePileWidget::ApplyDropFeedback()
{
	if (!DropFeedback)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = VisualStyle.IsValid()
		? VisualStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	FLinearColor Color = Style->ResolveZoneAppearance(PileView.Zone).AccentColor;
	switch (DropFeedbackView.State)
	{
	case EWacomBackpackDropFeedbackState::Rejected:
		Color = Style->RejectedTargetColor;
		break;
	case EWacomBackpackDropFeedbackState::Destructive:
		Color = Style->DestructiveAppearance.AccentColor;
		break;
	case EWacomBackpackDropFeedbackState::Valid:
	case EWacomBackpackDropFeedbackState::None:
	default:
		break;
	}
	Color.A = FMath::Clamp(Style->DropFeedbackFillOpacity, 0.0f, 1.0f);
	DropFeedback->SetBrushColor(Color);
	DropFeedback->SetVisibility(DropFeedbackView.IsVisible()
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
	if (DropFeedbackText)
	{
		DropFeedbackText->SetText(DropFeedbackView.Message);
		DropFeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.98f, 0.96f, 1.0f)));
	}
	if (DropFeedbackCountText)
	{
		FText CapacityPreviewText = FText::GetEmpty();
		if (DropFeedbackView.bHasCapacity)
		{
			CapacityPreviewText = FText::Format(
				LOCTEXT("DropCapacityPreview", "{0} + {1} / {2}"),
				FText::AsNumber(DropFeedbackView.CurrentCount),
				FText::AsNumber(DropFeedbackView.IncomingCount),
				FText::AsNumber(DropFeedbackView.Capacity));
		}
		else if (DropFeedbackView.IncomingCount > 0)
		{
			CapacityPreviewText = FText::Format(
				LOCTEXT("DropCountPreview", "{0} 张卡牌"),
				FText::AsNumber(DropFeedbackView.IncomingCount));
		}
		DropFeedbackCountText->SetText(CapacityPreviewText);
		DropFeedbackCountText->SetVisibility(CapacityPreviewText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}

#undef LOCTEXT_NAMESPACE
