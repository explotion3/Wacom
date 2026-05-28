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

void UWacomFirstPersonCardLayerWidget::SetSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	SlotFeedbackConfig = InConfig;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
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
	PendingTransitionHintsByKey.Reset();
	LastMotionDebugView = FWacomFirstPersonCardLayerMotionDebugView();
}

void UWacomFirstPersonCardLayerWidget::SetCardTransitionHints(
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& InHints)
{
	PendingTransitionHintsByKey.Reset();
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : InHints)
	{
		if (!Hint.CardInstanceId.IsValid()
			|| Hint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Default)
		{
			continue;
		}

		PendingTransitionHintsByKey.Add(
			Hint.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
			Hint.TransitionKind);
	}
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

	LastMotionDebugView = FWacomFirstPersonCardLayerMotionDebugView();
	LastMotionDebugView.InputSlotCount = InSlots.Num();
	LastMotionDebugView.OutgoingFinishedThisUpdate += RemoveOutgoingFinishedSlots();

	TMap<FString, UWacomFirstPersonCardLayerSlotWidget*> ExistingByKey;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			if (!ExistingByKey.Contains(SlotWidget->GetSlotMotionKey()))
			{
				ExistingByKey.Add(SlotWidget->GetSlotMotionKey(), SlotWidget.Get());
			}
			else
			{
				LastMotionDebugView.bHadInvariantViolation = true;
			}
		}
	}
	TMap<FString, UWacomFirstPersonCardLayerSlotWidget*> ExistingOutgoingByKey;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			if (!ExistingOutgoingByKey.Contains(SlotWidget->GetSlotMotionKey()))
			{
				ExistingOutgoingByKey.Add(SlotWidget->GetSlotMotionKey(), SlotWidget.Get());
			}
			else
			{
				LastMotionDebugView.bHadInvariantViolation = true;
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
			++LastMotionDebugView.DuplicateKeyCount;
		}
		UsedKeys.Add(SlotKey);
		IncomingKeys.Add(SlotKey);

		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = ExistingByKey.FindRef(SlotKey);
		if (!SlotWidget)
		{
			SlotWidget = ExistingOutgoingByKey.FindRef(SlotKey);
			if (SlotWidget)
			{
				const int32 RemovedOutgoingReferences = OutgoingSlotWidgets.RemoveAllSwap(
					[SlotWidget](const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& OutgoingSlotWidget)
					{
						return OutgoingSlotWidget.Get() == SlotWidget;
					},
					EAllowShrinking::No);
				if (RemovedOutgoingReferences > 1)
				{
					LastMotionDebugView.bHadInvariantViolation = true;
				}
			}
		}
		const bool bIsNewSlotWidget = SlotWidget == nullptr;
		if (!SlotWidget)
		{
			SlotWidget = CreateSlotWidget();
			if (SlotWidget)
			{
				++LastMotionDebugView.CreatedThisUpdate;
			}
		}
		if (!SlotWidget)
		{
			continue;
		}
		if (!bIsNewSlotWidget)
		{
			++LastMotionDebugView.ReusedThisUpdate;
		}
		const EWacomFirstPersonCardSlotTransitionKind IncomingTransitionKind =
			PendingTransitionHintsByKey.FindRef(SlotKey);
		const TOptional<FVector2D> EnterOffsetOverride =
			GetEnterOffsetOverrideForTransition(IncomingTransitionKind);

		SlotWidget->SetSlotMotionKey(SlotKey);
		SlotWidget->SetCardViewClass(CardViewClass);
		SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
		SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
		SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
		const bool bShouldPlayProjectionExit =
			SlotMotionConfig.bEnabled
			&& !bIsNewSlotWidget
			&& !SlotView.bProjected
			&& SlotWidget->GetVisualSlotView().bProjected
			&& !SlotWidget->IsExitingForFirstPersonLayer();
		if (bShouldPlayProjectionExit)
		{
			SlotWidget->BeginExitMotionWithOffset(SlotWidget->GetVisualSlotView(), TOptional<FVector2D>());
		}
		else if (SlotMotionConfig.bEnabled && SlotWidget->IsExitingForFirstPersonLayer() && !SlotView.bProjected)
		{
			// Keep the current exit animation alive while projection is still unavailable.
		}
		else
		{
			SlotWidget->BeginSlotMotionWithEnterOffset(SlotView, bIsNewSlotWidget, EnterOffsetOverride);
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
			const EWacomFirstPersonCardSlotTransitionKind OutgoingTransitionKind =
				PendingTransitionHintsByKey.FindRef(SlotWidget->GetSlotMotionKey());
			const TOptional<FVector2D> ExitOffsetOverride =
				GetExitOffsetOverrideForTransition(OutgoingTransitionKind);
			SlotWidget->BeginExitMotionWithOffset(SlotWidget->GetSlotView(), ExitOffsetOverride);
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
			{
				CanvasSlot->SetZOrder(SlotWidget->GetSlotView().ZOrder);
			}
			OutgoingSlotWidgets.Add(SlotWidget);
			++LastMotionDebugView.OutgoingStartedThisUpdate;
		}
		else
		{
			UnbindSlotWidget(SlotWidget);
			SlotWidget->RemoveFromParent();
			++LastMotionDebugView.RemovedThisUpdate;
		}
	}
	SlotWidgets = MoveTemp(NewSlotWidgets);
	LastMotionDebugView.OutgoingFinishedThisUpdate += RemoveOutgoingFinishedSlots();
	RepairSlotMotionInvariants();
	EnforceOutgoingSlotLimit();
	LastMotionDebugView.UntrackedChildRemovedThisUpdate += RemoveUntrackedSlotChildren();
	RefreshSlotMotionDebugCounts();
	ReportSlotMotionDiagnosticsIfNeeded();
	PendingTransitionHintsByKey.Reset();
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

FString UWacomFirstPersonCardLayerWidget::GetSlotMotionDebugSummary() const
{
	return FString::Printf(
		TEXT("SlotMotion Input=%d Active=%d Outgoing=%d RootChildren=%d Ticking=%d DuplicateKeys=%d Created=%d Reused=%d Removed=%d OutStarted=%d OutFinished=%d UntrackedRemoved=%d Invariant=%s"),
		LastMotionDebugView.InputSlotCount,
		LastMotionDebugView.ActiveSlotCount,
		LastMotionDebugView.OutgoingSlotCount,
		LastMotionDebugView.RootCanvasChildCount,
		LastMotionDebugView.MotionTickSlotCount,
		LastMotionDebugView.DuplicateKeyCount,
		LastMotionDebugView.CreatedThisUpdate,
		LastMotionDebugView.ReusedThisUpdate,
		LastMotionDebugView.RemovedThisUpdate,
		LastMotionDebugView.OutgoingStartedThisUpdate,
		LastMotionDebugView.OutgoingFinishedThisUpdate,
		LastMotionDebugView.UntrackedChildRemovedThisUpdate,
		LastMotionDebugView.bHadInvariantViolation ? TEXT("true") : TEXT("false"));
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
	LastMotionDebugView.OutgoingFinishedThisUpdate += RemoveOutgoingFinishedSlots();
	RefreshSlotMotionDebugCounts();
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

void UWacomFirstPersonCardLayerWidget::AddUntrackedSlotChildForTest()
{
	if (!RootCanvas)
	{
		RebuildWidget();
	}
	if (!RootCanvas || !WidgetTree)
	{
		return;
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget =
		WidgetTree->ConstructWidget<UWacomFirstPersonCardLayerSlotWidget>(
			UWacomFirstPersonCardLayerSlotWidget::StaticClass());
	if (SlotWidget)
	{
		RootCanvas->AddChild(SlotWidget);
	}
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
	PendingTransitionHintsByKey.Reset();
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	LastMotionDebugView.OutgoingFinishedThisUpdate += RemoveOutgoingFinishedSlots();
	RefreshSlotMotionDebugCounts();
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
	SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
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

int32 UWacomFirstPersonCardLayerWidget::RemoveOutgoingFinishedSlots()
{
	int32 RemovedCount = 0;
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
			++RemovedCount;
		}
	}
	return RemovedCount;
}

int32 UWacomFirstPersonCardLayerWidget::RemoveUntrackedSlotChildren()
{
	if (!RootCanvas)
	{
		return 0;
	}

	int32 RemovedCount = 0;
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
		++RemovedCount;
	}
	if (RemovedCount > 0)
	{
		LastMotionDebugView.bHadInvariantViolation = true;
	}
	return RemovedCount;
}

void UWacomFirstPersonCardLayerWidget::EnforceOutgoingSlotLimit()
{
	const int32 MaxOutgoingSlots = FMath::Max(LastSlots.Num() * 2, 16);
	while (OutgoingSlotWidgets.Num() > MaxOutgoingSlots)
	{
		TObjectPtr<UWacomFirstPersonCardLayerSlotWidget> SlotWidget = OutgoingSlotWidgets[0];
		if (SlotWidget)
		{
			UnbindSlotWidget(SlotWidget);
			SlotWidget->RemoveFromParent();
		}
		OutgoingSlotWidgets.RemoveAt(0);
		++LastMotionDebugView.RemovedThisUpdate;
		LastMotionDebugView.bHadInvariantViolation = true;
	}
}

void UWacomFirstPersonCardLayerWidget::RepairSlotMotionInvariants()
{
	TSet<UWacomFirstPersonCardLayerSlotWidget*> ActiveSet;
	TSet<FString> ActiveKeys;
	for (int32 Index = SlotWidgets.Num() - 1; Index >= 0; --Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			SlotWidgets.RemoveAt(Index);
			LastMotionDebugView.bHadInvariantViolation = true;
			continue;
		}
		if (ActiveSet.Contains(SlotWidget))
		{
			SlotWidgets.RemoveAt(Index);
			LastMotionDebugView.bHadInvariantViolation = true;
			continue;
		}
		ActiveSet.Add(SlotWidget);
		ActiveKeys.Add(SlotWidget->GetSlotMotionKey());
	}

	TSet<UWacomFirstPersonCardLayerSlotWidget*> OutgoingSet;
	for (int32 Index = OutgoingSlotWidgets.Num() - 1; Index >= 0; --Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = OutgoingSlotWidgets[Index];
		const bool bSharesActivePointer = SlotWidget && ActiveSet.Contains(SlotWidget);
		const bool bDuplicateOutgoingPointer = SlotWidget && OutgoingSet.Contains(SlotWidget);
		const bool bSharesActiveKey = SlotWidget && ActiveKeys.Contains(SlotWidget->GetSlotMotionKey());
		if (!SlotWidget || bSharesActivePointer || bDuplicateOutgoingPointer || bSharesActiveKey)
		{
			if (SlotWidget && !bSharesActivePointer && !bDuplicateOutgoingPointer)
			{
				UnbindSlotWidget(SlotWidget);
				SlotWidget->RemoveFromParent();
				++LastMotionDebugView.RemovedThisUpdate;
			}
			OutgoingSlotWidgets.RemoveAt(Index);
			LastMotionDebugView.bHadInvariantViolation = true;
			continue;
		}
		OutgoingSet.Add(SlotWidget);
	}
}

void UWacomFirstPersonCardLayerWidget::RefreshSlotMotionDebugCounts()
{
	LastMotionDebugView.ActiveSlotCount = SlotWidgets.Num();
	LastMotionDebugView.OutgoingSlotCount = OutgoingSlotWidgets.Num();
	LastMotionDebugView.RootCanvasChildCount = CountRootCanvasSlotChildren();
	LastMotionDebugView.MotionTickSlotCount = CountMotionTickSlots();
}

int32 UWacomFirstPersonCardLayerWidget::CountRootCanvasSlotChildren() const
{
	if (!RootCanvas)
	{
		return 0;
	}

	int32 Count = 0;
	for (int32 Index = 0; Index < RootCanvas->GetChildrenCount(); ++Index)
	{
		if (Cast<UWacomFirstPersonCardLayerSlotWidget>(RootCanvas->GetChildAt(Index)))
		{
			++Count;
		}
	}
	return Count;
}

int32 UWacomFirstPersonCardLayerWidget::CountMotionTickSlots() const
{
	int32 Count = 0;
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget && SlotWidget->WantsSlotMotionTick())
		{
			++Count;
		}
	}
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget && SlotWidget->WantsSlotMotionTick())
		{
			++Count;
		}
	}
	return Count;
}

void UWacomFirstPersonCardLayerWidget::ReportSlotMotionDiagnosticsIfNeeded()
{
	if (!bLogSlotMotionDiagnostics || !LastMotionDebugView.bHadInvariantViolation)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCardLayer] %s"), *GetSlotMotionDebugSummary());
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

TOptional<FVector2D> UWacomFirstPersonCardLayerWidget::GetEnterOffsetOverrideForTransition(
	EWacomFirstPersonCardSlotTransitionKind TransitionKind) const
{
	if (!SlotMotionConfig.bEnableEventAwareTransitions)
	{
		return TOptional<FVector2D>();
	}

	switch (TransitionKind)
	{
	case EWacomFirstPersonCardSlotTransitionKind::Drawn:
		return SlotMotionConfig.DrawnEnterOffsetPixels;
	case EWacomFirstPersonCardSlotTransitionKind::Gained:
		return SlotMotionConfig.GainedEnterOffsetPixels;
	default:
		return TOptional<FVector2D>();
	}
}

TOptional<FVector2D> UWacomFirstPersonCardLayerWidget::GetExitOffsetOverrideForTransition(
	EWacomFirstPersonCardSlotTransitionKind TransitionKind) const
{
	if (!SlotMotionConfig.bEnableEventAwareTransitions)
	{
		return TOptional<FVector2D>();
	}

	switch (TransitionKind)
	{
	case EWacomFirstPersonCardSlotTransitionKind::Played:
		return SlotMotionConfig.PlayedExitOffsetPixels;
	case EWacomFirstPersonCardSlotTransitionKind::Discarded:
		return SlotMotionConfig.DiscardedExitOffsetPixels;
	default:
		return TOptional<FVector2D>();
	}
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
