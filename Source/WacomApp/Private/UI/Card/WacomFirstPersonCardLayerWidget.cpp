// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

namespace
{
	const FVector2D CardAlignment(0.5f, 0.5f);
}

void UWacomFirstPersonCardLayerWidget::SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass)
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
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardViewClass(CardViewClass);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::SetCardSlots(
	const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots)
{
	LastSlots = InSlots;
	EnsureSlotWidgetCount(InSlots.Num());

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		if (!InSlots.IsValidIndex(Index))
		{
			SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FWacomFirstPersonCardLayerSlotView& SlotView = InSlots[Index];
		SlotWidget->SetCardViewClass(CardViewClass);
		SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
		SlotWidget->SetSlotView(SlotView);
		SlotWidget->SetRenderOpacity(FMath::Clamp(SlotView.RenderOpacity, 0.0f, 1.0f));
		SlotWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		FWidgetTransform CardRenderTransform;
		CardRenderTransform.Scale = FVector2D(FMath::Max(0.01f, SlotView.RenderScale));
		CardRenderTransform.Angle = SlotView.RenderAngleDegrees;
		SlotWidget->SetRenderTransform(CardRenderTransform);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(CardAlignment);
			CanvasSlot->SetPosition(SlotView.ScreenPosition);
			CanvasSlot->SetZOrder(SlotView.ZOrder);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::SetStaticCardSlots(
	const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots)
{
	SetCardSlots(InSlots);
}

void UWacomFirstPersonCardLayerWidget::SetCardLayerInteractionEnabled(bool bEnabled)
{
	if (bCardLayerInteractionEnabled == bEnabled)
	{
		return;
	}

	bCardLayerInteractionEnabled = bEnabled;
	ApplyLayerVisibility();
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
		}
	}
}

UWacomCardView* UWacomFirstPersonCardLayerWidget::GetCardViewAt(int32 Index) const
{
	const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = GetSlotWidgetAt(Index);
	return SlotWidget ? SlotWidget->GetCardView() : nullptr;
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::GetSlotWidgetAt(int32 Index) const
{
	return SlotWidgets.IsValidIndex(Index) ? SlotWidgets[Index].Get() : nullptr;
}

bool UWacomFirstPersonCardLayerWidget::IsCardSlotVisible(int32 Index) const
{
	const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = GetSlotWidgetAt(Index);
	return SlotWidget && SlotWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

FWidgetTransform UWacomFirstPersonCardLayerWidget::GetCardRenderTransformAt(int32 Index) const
{
	const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = GetSlotWidgetAt(Index);
	return SlotWidget ? SlotWidget->GetRenderTransform() : FWidgetTransform();
}

float UWacomFirstPersonCardLayerWidget::GetCardRenderOpacityAt(int32 Index) const
{
	const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = GetSlotWidgetAt(Index);
	return SlotWidget ? SlotWidget->GetRenderOpacity() : 0.0f;
}

int32 UWacomFirstPersonCardLayerWidget::GetCardZOrderAt(int32 Index) const
{
	const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = GetSlotWidgetAt(Index);
	const UCanvasPanelSlot* CanvasSlot = SlotWidget ? Cast<UCanvasPanelSlot>(SlotWidget->Slot) : nullptr;
	return CanvasSlot ? CanvasSlot->GetZOrder() : INDEX_NONE;
}

TSharedRef<SWidget> UWacomFirstPersonCardLayerWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("FirstPersonCardLayerRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	else
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}

	ApplyLayerVisibility();
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardLayerWidget::NativeDestruct()
{
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->OnCardClickedNative.RemoveAll(this);
			SlotWidget->OnCardHoveredNative.RemoveAll(this);
			SlotWidget->OnCardUnhoveredNative.RemoveAll(this);
		}
	}
	OnCardClickedNative.Clear();
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	SlotWidgets.Reset();
	RootCanvas = nullptr;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerWidget::EnsureSlotWidgetCount(int32 DesiredCount)
{
	if (!RootCanvas)
	{
		RebuildWidget();
	}
	if (!RootCanvas)
	{
		return;
	}

	while (SlotWidgets.Num() > DesiredCount)
	{
		TObjectPtr<UWacomFirstPersonCardLayerSlotWidget> SlotWidget = SlotWidgets.Pop();
		if (SlotWidget)
		{
			SlotWidget->OnCardClickedNative.RemoveAll(this);
			SlotWidget->OnCardHoveredNative.RemoveAll(this);
			SlotWidget->OnCardUnhoveredNative.RemoveAll(this);
			SlotWidget->RemoveFromParent();
		}
	}

	while (SlotWidgets.Num() < DesiredCount)
	{
		SlotWidgets.Add(CreateSlotWidget(SlotWidgets.Num()));
	}
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::CreateSlotWidget(int32 Index)
{
	if (!RootCanvas || !WidgetTree)
	{
		return nullptr;
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget =
		WidgetTree->ConstructWidget<UWacomFirstPersonCardLayerSlotWidget>(
			UWacomFirstPersonCardLayerSlotWidget::StaticClass(),
			*FString::Printf(TEXT("FirstPersonCardLayerSlot_%02d"), Index));
	if (!SlotWidget)
	{
		return nullptr;
	}

	SlotWidget->SetCardViewClass(CardViewClass);
	SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
	BindSlotWidget(SlotWidget);
	RootCanvas->AddChild(SlotWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(CardAlignment);
		CanvasSlot->SetZOrder(Index);
	}
	return SlotWidget;
}

void UWacomFirstPersonCardLayerWidget::ApplyLayerVisibility()
{
	const ESlateVisibility LayerVisibility = bCardLayerInteractionEnabled
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::HitTestInvisible;
	SetVisibility(LayerVisibility);
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(LayerVisibility);
	}
}

void UWacomFirstPersonCardLayerWidget::BindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget)
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->OnCardClickedNative.RemoveAll(this);
	SlotWidget->OnCardHoveredNative.RemoveAll(this);
	SlotWidget->OnCardUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardClickedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotClicked);
	SlotWidget->OnCardHoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotHovered);
	SlotWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotUnhovered);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotClicked(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnCardClickedNative.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnCardHoveredNative.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnCardUnhoveredNative.Broadcast(CardInstanceId, SlotView);
}
