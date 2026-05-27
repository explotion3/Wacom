// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Card/WacomCardView.h"

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
	if (CardViews.Num() > 0)
	{
		for (TObjectPtr<UWacomCardView>& CardView : CardViews)
		{
			if (CardView)
			{
				CardView->RemoveFromParent();
			}
		}
		CardViews.Reset();
		SetStaticCardSlots(LastSlots);
	}
}

void UWacomFirstPersonCardLayerWidget::SetStaticCardSlots(
	const TArray<FWacomFirstPersonStaticCardSlotView>& InSlots)
{
	LastSlots = InSlots;
	EnsureCardViewCount(InSlots.Num());

	for (int32 Index = 0; Index < CardViews.Num(); ++Index)
	{
		UWacomCardView* CardView = CardViews[Index];
		if (!CardView)
		{
			continue;
		}

		if (!InSlots.IsValidIndex(Index))
		{
			CardView->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FWacomFirstPersonStaticCardSlotView& SlotView = InSlots[Index];
		CardView->SetCardViewData(SlotView.CardViewData);
		CardView->SetVisibility(SlotView.bProjected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		CardView->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		FWidgetTransform CardRenderTransform;
		CardRenderTransform.Scale = FVector2D(FMath::Max(0.01f, SlotView.RenderScale));
		CardRenderTransform.Angle = SlotView.RenderAngleDegrees;
		CardView->SetRenderTransform(CardRenderTransform);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CardView->Slot))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(CardAlignment);
			CanvasSlot->SetPosition(SlotView.ScreenPosition);
			CanvasSlot->SetZOrder(Index);
		}
	}
}

UWacomCardView* UWacomFirstPersonCardLayerWidget::GetCardViewAt(int32 Index) const
{
	return CardViews.IsValidIndex(Index) ? CardViews[Index].Get() : nullptr;
}

bool UWacomFirstPersonCardLayerWidget::IsCardSlotVisible(int32 Index) const
{
	const UWacomCardView* CardView = GetCardViewAt(Index);
	return CardView && CardView->GetVisibility() == ESlateVisibility::HitTestInvisible;
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
		RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootCanvas;
	}
	else
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardLayerWidget::NativeDestruct()
{
	CardViews.Reset();
	RootCanvas = nullptr;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerWidget::EnsureCardViewCount(int32 DesiredCount)
{
	if (!RootCanvas)
	{
		RebuildWidget();
	}
	if (!RootCanvas)
	{
		return;
	}

	while (CardViews.Num() > DesiredCount)
	{
		TObjectPtr<UWacomCardView> CardView = CardViews.Pop();
		if (CardView)
		{
			CardView->RemoveFromParent();
		}
	}

	while (CardViews.Num() < DesiredCount)
	{
		CardViews.Add(CreateCardView(CardViews.Num()));
	}
}

UWacomCardView* UWacomFirstPersonCardLayerWidget::CreateCardView(int32 Index)
{
	if (!RootCanvas || !WidgetTree)
	{
		return nullptr;
	}

	UClass* ClassToUse = CardViewClass ? CardViewClass.Get() : UWacomCardView::StaticClass();
	UWacomCardView* CardView = WidgetTree->ConstructWidget<UWacomCardView>(
		ClassToUse,
		*FString::Printf(TEXT("StaticCardView_%02d"), Index));
	if (!CardView)
	{
		return nullptr;
	}

	CardView->SetVisibility(ESlateVisibility::HitTestInvisible);
	RootCanvas->AddChild(CardView);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CardView->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(CardAlignment);
		CanvasSlot->SetZOrder(Index);
	}
	return CardView;
}
