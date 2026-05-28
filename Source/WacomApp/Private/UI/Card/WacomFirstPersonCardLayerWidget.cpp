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

	bool ContainsSlotWidget(
		const TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>>& SlotWidgets,
		const UWacomFirstPersonCardLayerSlotWidget* Candidate)
	{
		return Candidate && SlotWidgets.ContainsByPredicate(
			[Candidate](const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget)
			{
				return SlotWidget.Get() == Candidate;
			});
	}
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
	ClearSlotMotionState();
}

void UWacomFirstPersonCardLayerWidget::SetSlotMotionConfig(
	const FWacomFirstPersonCardSlotMotionConfig& InConfig)
{
	SlotMotionConfig = InConfig;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::ClearSlotMotionState()
{
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			UnbindSlotWidget(SlotWidget);
			SlotWidget->RemoveFromParent();
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			UnbindSlotWidget(SlotWidget);
			SlotWidget->RemoveFromParent();
		}
	}
	SlotWidgets.Reset();
	OutgoingSlotWidgets.Reset();
	LastSlots.Reset();
}

void UWacomFirstPersonCardLayerWidget::SetCardSlots(
	const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots)
{
	LastSlots = InSlots;
	if (!RootCanvas)
	{
		RebuildWidget();
	}
	if (!RootCanvas)
	{
		return;
	}

	RemoveOutgoingFinishedSlots();
	TMap<FString, UWacomFirstPersonCardLayerSlotWidget*> ExistingByKey;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			if (!ExistingByKey.Contains(SlotWidget->GetSlotMotionKey()))
			{
				ExistingByKey.Add(SlotWidget->GetSlotMotionKey(), SlotWidget.Get());
			}
		}
	}

	TSet<FString> IncomingKeys;
	TSet<FString> UsedKeys;
	TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>> NewSlotWidgets;
	NewSlotWidgets.Reserve(InSlots.Num());
	for (int32 Index = 0; Index < InSlots.Num(); ++Index)
	{
		const FWacomFirstPersonCardLayerSlotView& SlotView = InSlots[Index];
		const FString BaseSlotKey = MakeSlotMotionKey(SlotView);
		FString SlotKey = BaseSlotKey;
		if (UsedKeys.Contains(SlotKey))
		{
			SlotKey = FString::Printf(TEXT("%s#SlotIndex:%d"), *BaseSlotKey, Index);
		}
		UsedKeys.Add(SlotKey);
		IncomingKeys.Add(SlotKey);

		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = ExistingByKey.FindRef(SlotKey);
		const bool bIsNewSlotWidget = SlotWidget == nullptr;
		if (!SlotWidget)
		{
			SlotWidget = CreateSlotWidget();
		}
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotMotionKey(SlotKey);
		SlotWidget->SetCardViewClass(CardViewClass);
		SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
		SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
		const bool bShouldPlayProjectionExit =
			SlotMotionConfig.bEnabled
			&& !bIsNewSlotWidget
			&& !SlotView.bProjected
			&& SlotWidget->GetVisualSlotView().bProjected
			&& !SlotWidget->IsExitingForFirstPersonLayer();
		if (bShouldPlayProjectionExit)
		{
			SlotWidget->BeginExitMotion(SlotWidget->GetVisualSlotView());
		}
		else if (SlotMotionConfig.bEnabled && SlotWidget->IsExitingForFirstPersonLayer() && !SlotView.bProjected)
		{
			// Keep the current exit animation alive while projection is still unavailable.
		}
		else
		{
			SlotWidget->BeginSlotMotion(SlotView, bIsNewSlotWidget);
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(CardAlignment);
			CanvasSlot->SetPosition(SlotWidget->GetVisualSlotView().ScreenPosition);
			CanvasSlot->SetZOrder(SlotView.ZOrder);
		}

		if (SlotView.bIsHovered && SlotView.bProjected && SlotView.Entry.CardInstanceId.IsValid())
		{
			FWacomFirstPersonCardLayerSlotView HoveredVisualSlotView = SlotWidget->GetVisualSlotView();
			HoveredVisualSlotView.bIsHovered = SlotView.bIsHovered;
			OnHoveredCardSlotUpdatedNative.Broadcast(SlotView.Entry.CardInstanceId, HoveredVisualSlotView);
		}
		NewSlotWidgets.Add(SlotWidget);
	}

	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget || IncomingKeys.Contains(SlotWidget->GetSlotMotionKey()))
		{
			continue;
		}
		if (SlotMotionConfig.bEnabled && SlotWidget->GetSlotView().bProjected)
		{
			SlotWidget->BeginExitMotion(SlotWidget->GetSlotView());
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
			{
				CanvasSlot->SetZOrder(SlotWidget->GetSlotView().ZOrder);
			}
			OutgoingSlotWidgets.Add(SlotWidget);
		}
		else
		{
			UnbindSlotWidget(SlotWidget);
			SlotWidget->RemoveFromParent();
		}
	}
	SlotWidgets = MoveTemp(NewSlotWidgets);
	RemoveOutgoingFinishedSlots();
	RemoveUntrackedSlotChildren();
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

#if WITH_AUTOMATION_TESTS
void UWacomFirstPersonCardLayerWidget::TickSlotMotionForTest(float DeltaTime)
{
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->TickSlotMotionForTest(DeltaTime);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->TickSlotMotionForTest(DeltaTime);
		}
	}
	RemoveOutgoingFinishedSlots();
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::FindSlotWidgetByKeyForTest(
	const FString& SlotKey) const
{
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget> SlotWidget : SlotWidgets)
	{
		if (SlotWidget && SlotWidget->GetSlotMotionKey() == SlotKey)
		{
			return SlotWidget.Get();
		}
	}
	return nullptr;
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::GetOutgoingSlotWidgetAtForTest(
	int32 Index) const
{
	return OutgoingSlotWidgets.IsValidIndex(Index) ? OutgoingSlotWidgets[Index].Get() : nullptr;
}
#endif

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
			UnbindSlotWidget(SlotWidget);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			UnbindSlotWidget(SlotWidget);
		}
	}
	OnCardClickedNative.Clear();
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	OnHoveredCardSlotUpdatedNative.Clear();
	SlotWidgets.Reset();
	OutgoingSlotWidgets.Reset();
	RootCanvas = nullptr;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RemoveOutgoingFinishedSlots();
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::CreateSlotWidget()
{
	if (!RootCanvas || !WidgetTree)
	{
		return nullptr;
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget =
		WidgetTree->ConstructWidget<UWacomFirstPersonCardLayerSlotWidget>(
			UWacomFirstPersonCardLayerSlotWidget::StaticClass());
	if (!SlotWidget)
	{
		return nullptr;
	}

	SlotWidget->SetCardViewClass(CardViewClass);
	SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
	SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
	BindSlotWidget(SlotWidget);
	RootCanvas->AddChild(SlotWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(CardAlignment);
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

void UWacomFirstPersonCardLayerWidget::UnbindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget)
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->OnCardClickedNative.RemoveAll(this);
	SlotWidget->OnCardHoveredNative.RemoveAll(this);
	SlotWidget->OnCardUnhoveredNative.RemoveAll(this);
}

void UWacomFirstPersonCardLayerWidget::RemoveOutgoingFinishedSlots()
{
	for (int32 Index = OutgoingSlotWidgets.Num() - 1; Index >= 0; --Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = OutgoingSlotWidgets[Index];
		if (!SlotWidget || SlotWidget->IsExitMotionFinished())
		{
			if (SlotWidget)
			{
				UnbindSlotWidget(SlotWidget);
				SlotWidget->RemoveFromParent();
			}
			OutgoingSlotWidgets.RemoveAt(Index);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::RemoveUntrackedSlotChildren()
{
	if (!RootCanvas)
	{
		return;
	}

	for (int32 Index = RootCanvas->GetChildrenCount() - 1; Index >= 0; --Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* ChildSlot =
			Cast<UWacomFirstPersonCardLayerSlotWidget>(RootCanvas->GetChildAt(Index));
		if (!ChildSlot)
		{
			continue;
		}

		if (ContainsSlotWidget(SlotWidgets, ChildSlot) || ContainsSlotWidget(OutgoingSlotWidgets, ChildSlot))
		{
			continue;
		}

		UnbindSlotWidget(ChildSlot);
		ChildSlot->RemoveFromParent();
	}
}

FString UWacomFirstPersonCardLayerWidget::MakeSlotMotionKey(
	const FWacomFirstPersonCardLayerSlotView& SlotView) const
{
	if (SlotView.Entry.CardInstanceId.IsValid())
	{
		return SlotView.Entry.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower);
	}
	return FString::Printf(TEXT("StaticIndex:%d"), SlotView.Index);
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
