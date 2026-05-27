// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "InputCoreTypes.h"
#include "UI/Card/WacomCardView.h"

void UWacomFirstPersonCardLayerSlotWidget::SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass)
{
	TSubclassOf<UWacomCardView> NewCardViewClass = InCardViewClass;
	if (!NewCardViewClass)
	{
		NewCardViewClass = UWacomCardView::StaticClass();
	}

	if (CardViewClass == NewCardViewClass)
	{
		return;
	}

	CardViewClass = NewCardViewClass;
	if (CardView)
	{
		CardView->RemoveFromParent();
		CardView = nullptr;
	}
	EnsureCardView();
	ApplyCurrentSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotView(const FWacomFirstPersonCardLayerSlotView& InSlotView)
{
	if (bIsHoveredForFirstPersonLayer
		&& (CurrentSlotView.Entry.CardInstanceId != InSlotView.Entry.CardInstanceId
			|| !InSlotView.bProjected
			|| !InSlotView.Entry.CardInstanceId.IsValid()))
	{
		SetHoveredForFirstPersonLayer(false);
	}

	CurrentSlotView = InSlotView;
	ApplyCurrentSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardLayerInteractionEnabled(bool bEnabled)
{
	if (bCardLayerInteractionEnabled == bEnabled)
	{
		return;
	}

	if (!bEnabled)
	{
		SetHoveredForFirstPersonLayer(false);
	}

	bCardLayerInteractionEnabled = bEnabled;
	UpdateVisibilityForInteractionMode();
}

TSharedRef<SWidget> UWacomFirstPersonCardLayerSlotWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			TEXT("FirstPersonCardLayerSlotRoot"));
		WidgetTree->RootWidget = RootOverlay;
	}
	else
	{
		RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	}

	EnsureCardView();
	UpdateVisibilityForInteractionMode();
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardLayerSlotWidget::NativeDestruct()
{
	SetHoveredForFirstPersonLayer(false);
	OnCardClickedNative.Clear();
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	CardView = nullptr;
	RootOverlay = nullptr;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (CanInteractWithCurrentSlot())
	{
		SetHoveredForFirstPersonLayer(true);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	SetHoveredForFirstPersonLayer(false);
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanClickCurrentSlot())
	{
		OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UWacomFirstPersonCardLayerSlotWidget::EnsureCardView()
{
	if (CardView)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree)
	{
		return;
	}

	if (!RootOverlay)
	{
		if (WidgetTree->RootWidget)
		{
			RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
		}
		else
		{
			RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("FirstPersonCardLayerSlotRoot"));
			WidgetTree->RootWidget = RootOverlay;
		}
	}
	if (!RootOverlay)
	{
		return;
	}

	UClass* ClassToUse = CardViewClass ? CardViewClass.Get() : UWacomCardView::StaticClass();
	CardView = WidgetTree->ConstructWidget<UWacomCardView>(ClassToUse, TEXT("CardView"));
	if (!CardView)
	{
		return;
	}

	CardView->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* CardSlot = RootOverlay->AddChildToOverlay(CardView))
	{
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCurrentSlotView()
{
	EnsureCardView();
	if (CardView)
	{
		CardView->SetCardViewData(CurrentSlotView.Entry.CardViewData);
	}
	UpdateVisibilityForInteractionMode();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& CurrentSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanClickCurrentSlot() const
{
	return CanInteractWithCurrentSlot() && CurrentSlotView.Entry.bIsPlayable;
}

#if WITH_AUTOMATION_TESTS
bool UWacomFirstPersonCardLayerSlotWidget::RequestHoverForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	SetHoveredForFirstPersonLayer(true);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::RequestUnhoverForTest()
{
	SetHoveredForFirstPersonLayer(false);
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestClickForTest()
{
	if (!CanClickCurrentSlot())
	{
		return false;
	}

	OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
	return true;
}
#endif

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredForFirstPersonLayer(bool bHovered)
{
	if (bIsHoveredForFirstPersonLayer == bHovered)
	{
		return;
	}

	bIsHoveredForFirstPersonLayer = bHovered;
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		if (bIsHoveredForFirstPersonLayer)
		{
			OnCardHoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		}
		else
		{
			OnCardUnhoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		}
	}
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateVisibilityForInteractionMode()
{
	const bool bVisible = CurrentSlotView.bProjected;
	SetVisibility(bVisible
		? (bCardLayerInteractionEnabled ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible)
		: ESlateVisibility::Collapsed);
	if (CardView)
	{
		CardView->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
