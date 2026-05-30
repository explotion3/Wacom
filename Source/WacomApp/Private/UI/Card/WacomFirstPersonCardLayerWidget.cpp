// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Rendering/DrawElements.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

namespace
{
	const FVector2D CardAlignment(0.5f, 0.5f);
	const FVector2D DefaultCardTargetHitSize(296.0f, 420.0f);

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

void UWacomFirstPersonCardLayerWidget::SetCardDragConfig(
	const FWacomFirstPersonCardDragConfig& InConfig)
{
	CardDragConfig = InConfig;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragConfig(CardDragConfig);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragConfig(CardDragConfig);
		}
	}
	if (!CardDragConfig.bEnableFirstPersonCardDragCommit)
	{
		ClearCurrentDragState(true);
	}
}

void UWacomFirstPersonCardLayerWidget::SetCardDragFeedbackTarget(
	const FWacomInteractionTargetHandle& TargetHandle,
	bool bValidTarget,
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	const TOptional<FVector2D>& FeedbackTargetScreenPosition,
	const FString& ResolvedIntentDebugSummary,
	const TArray<FWacomFirstPersonCardTargetAffordance>& CardTargetAffordances)
{
	if (!CardDragConfig.bEnableDragTargetFeedback)
	{
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	}
	CurrentDragResolvedIntentDebugSummary = ResolvedIntentDebugSummary;

	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}

		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (SlotCardId == CurrentDragView.CardInstanceId)
		{
			SlotWidget->SetCardDragFeedbackTarget(
				TargetHandle,
				bValidTarget,
				FeedbackState,
				FeedbackTargetScreenPosition);
		}
	}
	ApplyCardTargetAffordances(TargetHandle, FeedbackState, bValidTarget, CardTargetAffordances);
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
		}
	}

	ApplyDragFeedbackToCurrentDragView(TargetHandle, bValidTarget, FeedbackState, FeedbackTargetScreenPosition);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerWidget::CancelCardDragGesture(bool bBroadcastCancel)
{
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->CancelCardDragGesture(bBroadcastCancel);
		}
	}
	ClearCurrentDragState(bBroadcastCancel);
}

void UWacomFirstPersonCardLayerWidget::ClearSlotMotionState()
{
	ClearHoveredCardTargetState(true);
	ClearCurrentDragState(true);
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
	HoveredCardTargetHandle = FWacomInteractionTargetHandle();
	HoveredCardTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	LastMotionDebugView = FWacomFirstPersonCardLayerMotionDebugView();
}

void UWacomFirstPersonCardLayerWidget::SetCardTransitionHints(
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& InHints)
{
	PendingTransitionHintsByKey.Reset();
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : InHints)
	{
		if (!Hint.CardInstanceId.IsValid()
			|| (Hint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Default
				&& !Hint.bPlayCommitFeedback))
		{
			continue;
		}

		FWacomFirstPersonCardLayerResolvedTransitionHint ResolvedHint;
		ResolvedHint.TransitionKind = Hint.TransitionKind;
		ResolvedHint.bPlayCommitFeedback = Hint.bPlayCommitFeedback;
		ResolvedHint.bHasPlayedExitTargetWidgetPosition = Hint.bHasPlayedExitTargetWidgetPosition;
		ResolvedHint.PlayedExitTargetWidgetPosition = Hint.PlayedExitTargetWidgetPosition;
		PendingTransitionHintsByKey.Add(
			Hint.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
			ResolvedHint);
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
	bool bHoveredCardTargetUpdatedThisRefresh = false;
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
		const FWacomFirstPersonCardLayerResolvedTransitionHint IncomingTransitionHint =
			PendingTransitionHintsByKey.FindRef(SlotKey);
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile> EnterProfileOverride =
			GetEnterProfileForTransition(IncomingTransitionHint.TransitionKind, SlotView);

		SlotWidget->SetSlotMotionKey(SlotKey);
		SlotWidget->SetCardViewClass(CardViewClass);
		SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
		SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
		SlotWidget->SetCardDragConfig(CardDragConfig);
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
			SlotWidget->BeginSlotMotionWithEnterProfile(SlotView, bIsNewSlotWidget, EnterProfileOverride);
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(CardAlignment);
			const FWacomFirstPersonCardLayerSlotView& VisualSlotView = SlotWidget->GetVisualSlotView();
			CanvasSlot->SetPosition(VisualSlotView.ScreenPosition);
			CanvasSlot->SetZOrder(VisualSlotView.ZOrder);
		}

		if (SlotView.bIsHovered && SlotView.bProjected && SlotView.Entry.CardInstanceId.IsValid())
		{
			FWacomFirstPersonCardLayerSlotView HoveredVisualSlotView = SlotWidget->GetVisualSlotView();
			HoveredVisualSlotView.bIsHovered = SlotView.bIsHovered;
			OnHoveredCardSlotUpdatedNative.Broadcast(SlotView.Entry.CardInstanceId, HoveredVisualSlotView);
			const FWacomInteractionTargetHandle CardTargetHandle = SlotWidget->BuildCardTargetHandle();
			if (CardTargetHandle.IsValid())
			{
				HoveredCardTargetHandle = CardTargetHandle;
				HoveredCardTargetSlotView = HoveredVisualSlotView;
				OnHoveredCardTargetUpdatedNative.Broadcast(CardTargetHandle, HoveredVisualSlotView);
				bHoveredCardTargetUpdatedThisRefresh = true;
			}
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
			const FWacomFirstPersonCardLayerResolvedTransitionHint OutgoingTransitionHint =
				PendingTransitionHintsByKey.FindRef(SlotWidget->GetSlotMotionKey());
			const TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfileOverride =
				GetExitProfileForTransition(OutgoingTransitionHint, SlotWidget->GetVisualSlotView());
			SlotWidget->BeginExitMotionWithProfile(SlotWidget->GetSlotView(), ExitProfileOverride);
			if (OutgoingTransitionHint.bPlayCommitFeedback)
			{
				SlotWidget->TriggerCommitFeedback();
			}
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
			{
				CanvasSlot->SetZOrder(SlotWidget->GetVisualSlotView().ZOrder);
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
	if (HoveredCardTargetHandle.IsValid() && !bHoveredCardTargetUpdatedThisRefresh)
	{
		const bool bHoveredTargetStillActive = SlotWidgets.ContainsByPredicate(
			[this](const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget)
			{
				return SlotWidget
					&& SlotWidget->CanExposeCardTarget()
					&& SlotWidget->GetSlotView().Entry.CardInstanceId == HoveredCardTargetHandle.CardInstanceId;
			});
		if (!bHoveredTargetStillActive)
		{
			ClearHoveredCardTargetState(true);
		}
	}
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
	if (!bCardLayerInteractionEnabled)
	{
		ClearHoveredCardTargetState(true);
		ClearCurrentDragState(true);
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

FString UWacomFirstPersonCardLayerWidget::GetDragTargetDebugSummary() const
{
	int32 ValidAffordanceCount = 0;
	int32 InvalidAffordanceCount = 0;
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}

		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (!SlotCardId.IsValid() || SlotCardId == CurrentDragView.CardInstanceId)
		{
			continue;
		}

		const EWacomFirstPersonCardDragTargetFeedbackState SlotFeedbackState =
			SlotWidget->GetDragTargetFeedbackStateForFirstPersonLayer();
		if (SlotFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget)
		{
			++ValidAffordanceCount;
		}
		else if (SlotFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			++InvalidAffordanceCount;
		}
	}

	return FString::Printf(
		TEXT("DragTarget{CardId=%s Gesture=%d TargetKind=%d WorldTargetId=%s CardTargetId=%s Valid=%s State=%d HasTargetPos=%s TargetPos=%s Pointer=%s AffordanceValid=%d AffordanceInvalid=%d Resolved=%s}"),
		*CurrentDragView.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		static_cast<int32>(CurrentDragView.GestureState),
		static_cast<int32>(CurrentDragView.CurrentTarget.TargetKind),
		*CurrentDragView.CurrentTarget.WorldTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*CurrentDragView.CurrentTarget.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		CurrentDragView.bTargetValid ? TEXT("true") : TEXT("false"),
		static_cast<int32>(CurrentDragView.TargetFeedbackState),
		CurrentDragView.bHasFeedbackTargetScreenPosition ? TEXT("true") : TEXT("false"),
		*CurrentDragView.FeedbackTargetScreenPosition.ToString(),
		*CurrentDragView.CurrentScreenPosition.ToString(),
		ValidAffordanceCount,
		InvalidAffordanceCount,
		*CurrentDragResolvedIntentDebugSummary);
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

void UWacomFirstPersonCardLayerWidget::SetViewportSizeOverrideForTest(const FVector2D& WidgetViewportSize)
{
	WidgetViewportSizeOverrideForTest = WidgetViewportSize;
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
	ClearHoveredCardTargetState(false);
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
	OnCardTargetHoveredNative.Clear();
	OnCardTargetUnhoveredNative.Clear();
	OnHoveredCardTargetUpdatedNative.Clear();
	SlotWidgets.Reset();
	OutgoingSlotWidgets.Reset();
	HoveredCardTargetHandle = FWacomInteractionTargetHandle();
	HoveredCardTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	CurrentDragView = FWacomFirstPersonCardDragView();
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

int32 UWacomFirstPersonCardLayerWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 MaxLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	const bool bShouldDrawAimArrow =
		CardDragConfig.bEnableAimArrow
		&& CurrentDragView.CardInstanceId.IsValid()
		&& CurrentDragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard;
	if (!bShouldDrawAimArrow)
	{
		return MaxLayerId;
	}

	const FVector2D Start = CurrentDragView.SourceSlotView.ScreenPosition;
	const FVector2D End = ResolveAimArrowEnd();
	const FVector2D Direction = End - Start;
	if (Direction.SizeSquared() <= 4.0f)
	{
		return MaxLayerId;
	}

	const FLinearColor LineColor = ResolveAimArrowColor();
	const FVector2D UnitDirection = Direction.GetSafeNormal();
	const FVector2D Perpendicular(-UnitDirection.Y, UnitDirection.X);
	const float ArrowLength = 18.0f;
	const float ArrowWidth = 9.0f;
	TArray<FVector2D> MainLine;
	MainLine.Add(Start);
	MainLine.Add(End);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		MaxLayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		MainLine,
		ESlateDrawEffect::None,
		LineColor,
		true,
		3.0f);

	TArray<FVector2D> HeadLeft;
	HeadLeft.Add(End);
	HeadLeft.Add(End - UnitDirection * ArrowLength + Perpendicular * ArrowWidth);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		MaxLayerId + 2,
		AllottedGeometry.ToPaintGeometry(),
		HeadLeft,
		ESlateDrawEffect::None,
		LineColor,
		true,
		3.0f);

	TArray<FVector2D> HeadRight;
	HeadRight.Add(End);
	HeadRight.Add(End - UnitDirection * ArrowLength - Perpendicular * ArrowWidth);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		MaxLayerId + 2,
		AllottedGeometry.ToPaintGeometry(),
		HeadRight,
		ESlateDrawEffect::None,
		LineColor,
		true,
		3.0f);

	return MaxLayerId + 2;
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
	SlotWidget->SetCardDragConfig(CardDragConfig);
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
	SlotWidget->OnCardTargetHoveredNative.RemoveAll(this);
	SlotWidget->OnCardTargetUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardDragStartedNative.RemoveAll(this);
	SlotWidget->OnCardDragUpdatedNative.RemoveAll(this);
	SlotWidget->OnCardDragReleasedNative.RemoveAll(this);
	SlotWidget->OnCardDragCancelledNative.RemoveAll(this);
	SlotWidget->OnCardClickedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotClicked);
	SlotWidget->OnCardHoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotHovered);
	SlotWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotUnhovered);
	SlotWidget->OnCardTargetHoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotCardTargetHovered);
	SlotWidget->OnCardTargetUnhoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotCardTargetUnhovered);
	SlotWidget->OnCardDragStartedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragStarted);
	SlotWidget->OnCardDragUpdatedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragUpdated);
	SlotWidget->OnCardDragReleasedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragReleased);
	SlotWidget->OnCardDragCancelledNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragCancelled);
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
	SlotWidget->OnCardTargetHoveredNative.RemoveAll(this);
	SlotWidget->OnCardTargetUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardDragStartedNative.RemoveAll(this);
	SlotWidget->OnCardDragUpdatedNative.RemoveAll(this);
	SlotWidget->OnCardDragReleasedNative.RemoveAll(this);
	SlotWidget->OnCardDragCancelledNative.RemoveAll(this);
}

void UWacomFirstPersonCardLayerWidget::ClearHoveredCardTargetState(bool bBroadcastUnhover)
{
	if (!HoveredCardTargetHandle.IsValid())
	{
		return;
	}

	const FWacomInteractionTargetHandle PreviousHandle = HoveredCardTargetHandle;
	FWacomFirstPersonCardLayerSlotView PreviousSlotView = HoveredCardTargetSlotView;
	PreviousSlotView.bIsHovered = false;
	HoveredCardTargetHandle = FWacomInteractionTargetHandle();
	HoveredCardTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	if (bBroadcastUnhover)
	{
		OnCardTargetUnhoveredNative.Broadcast(PreviousHandle, PreviousSlotView);
	}
}

void UWacomFirstPersonCardLayerWidget::ClearCurrentDragState(bool bBroadcastCancel)
{
	const bool bHadDrag = CurrentDragView.CardInstanceId.IsValid()
		&& CurrentDragView.GestureState != EWacomFirstPersonCardGestureState::Idle
		&& CurrentDragView.GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	const FGuid PreviousCardId = CurrentDragView.CardInstanceId;
	FWacomFirstPersonCardDragView PreviousDragView = CurrentDragView;
	CurrentDragView = FWacomFirstPersonCardDragView();
	CurrentDragResolvedIntentDebugSummary.Reset();
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
		}
	}
	if (bHadDrag && bBroadcastCancel)
	{
		PreviousDragView.GestureState = EWacomFirstPersonCardGestureState::Cancelled;
		OnCardDragCancelledNative.Broadcast(PreviousCardId, PreviousDragView);
	}
	Invalidate(EInvalidateWidgetReason::Paint);
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

bool UWacomFirstPersonCardLayerWidget::ResolveViewportAnchorPosition(
	const FVector2D& NormalizedViewportAnchor,
	FVector2D& OutWidgetPosition) const
{
#if WITH_AUTOMATION_TESTS
	if (WidgetViewportSizeOverrideForTest.IsSet())
	{
		const FVector2D WidgetViewportSize = WidgetViewportSizeOverrideForTest.GetValue();
		if (WidgetViewportSize.X > 0.0f && WidgetViewportSize.Y > 0.0f)
		{
			OutWidgetPosition = FVector2D(
				WidgetViewportSize.X * FMath::Clamp(NormalizedViewportAnchor.X, 0.0f, 1.0f),
				WidgetViewportSize.Y * FMath::Clamp(NormalizedViewportAnchor.Y, 0.0f, 1.0f));
			return true;
		}
	}
#endif

	const UWorld* World = GetWorld();
	const UGameViewportClient* ViewportClient = World ? World->GetGameViewport() : nullptr;
	if (!ViewportClient)
	{
		return false;
	}

	FVector2D ViewportPixelSize = FVector2D::ZeroVector;
	ViewportClient->GetViewportSize(ViewportPixelSize);
	if (ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
	{
		return false;
	}

	const APlayerController* PC = GetOwningPlayer();
	const float ViewportScale = PC ? FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(PC)) : 1.0f;
	const FVector2D WidgetViewportSize = ViewportPixelSize / ViewportScale;
	OutWidgetPosition = FVector2D(
		WidgetViewportSize.X * FMath::Clamp(NormalizedViewportAnchor.X, 0.0f, 1.0f),
		WidgetViewportSize.Y * FMath::Clamp(NormalizedViewportAnchor.Y, 0.0f, 1.0f));
	return true;
}

TOptional<FWacomFirstPersonCardTransitionMotionProfile> UWacomFirstPersonCardLayerWidget::GetEnterProfileForTransition(
	EWacomFirstPersonCardSlotTransitionKind TransitionKind,
	const FWacomFirstPersonCardLayerSlotView& TargetSlotView) const
{
	if (!SlotMotionConfig.bEnableEventAwareTransitions)
	{
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	FWacomFirstPersonCardTransitionMotionProfile Profile;
	switch (TransitionKind)
	{
	case EWacomFirstPersonCardSlotTransitionKind::Drawn:
		Profile.OriginMode = SlotMotionConfig.DrawnEnterOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.DrawnEnterOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.DrawnEnterViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.DrawnEnterScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.DrawnEnterAngleOffsetDegrees;
		break;
	case EWacomFirstPersonCardSlotTransitionKind::Gained:
		Profile.OriginMode = SlotMotionConfig.GainedEnterOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.GainedEnterOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.GainedEnterViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.GainedEnterScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.GainedEnterAngleOffsetDegrees;
		break;
	default:
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	if (!SlotMotionConfig.bEnableReadableTransitionOrigins)
	{
		Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Profile.ScaleMultiplier = 1.0f;
		Profile.AngleOffsetDegrees = 0.0f;
		return Profile;
	}

	FVector2D OriginPosition = TargetSlotView.ScreenPosition;
	switch (Profile.OriginMode)
	{
	case EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset:
		OriginPosition = TargetSlotView.AnchorWidgetPosition;
		break;
	case EWacomFirstPersonCardTransitionOriginMode::ViewportAnchor:
		if (!ResolveViewportAnchorPosition(Profile.ViewportAnchor, OriginPosition))
		{
			OriginPosition = TargetSlotView.ScreenPosition;
		}
		break;
	case EWacomFirstPersonCardTransitionOriginMode::SlotOffset:
	default:
		OriginPosition = TargetSlotView.ScreenPosition;
		break;
	}

	Profile.OffsetPixels = (OriginPosition + Profile.OffsetPixels) - TargetSlotView.ScreenPosition;
	return Profile;
}

TOptional<FWacomFirstPersonCardTransitionMotionProfile> UWacomFirstPersonCardLayerWidget::GetExitProfileForTransition(
	const FWacomFirstPersonCardLayerResolvedTransitionHint& TransitionHint,
	const FWacomFirstPersonCardLayerSlotView& VisualSlotView) const
{
	if (!SlotMotionConfig.bEnableEventAwareTransitions)
	{
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	FWacomFirstPersonCardTransitionMotionProfile Profile;
	switch (TransitionHint.TransitionKind)
	{
	case EWacomFirstPersonCardSlotTransitionKind::Played:
		Profile.OriginMode = SlotMotionConfig.PlayedExitOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.PlayedExitOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.PlayedExitViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.PlayedExitScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.PlayedExitAngleOffsetDegrees;
		break;
	case EWacomFirstPersonCardSlotTransitionKind::Discarded:
		Profile.OriginMode = SlotMotionConfig.DiscardedExitOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.DiscardedExitOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.DiscardedExitViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.DiscardedExitScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.DiscardedExitAngleOffsetDegrees;
		break;
	default:
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	auto ApplyPlayedTargetBias = [&Profile, &TransitionHint, &VisualSlotView]()
	{
		if (!TransitionHint.bHasPlayedExitTargetWidgetPosition
			|| TransitionHint.TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Played)
		{
			return;
		}

		const FVector2D CurrentExitTarget = VisualSlotView.ScreenPosition + Profile.OffsetPixels;
		const FVector2D TargetDirection = TransitionHint.PlayedExitTargetWidgetPosition - VisualSlotView.ScreenPosition;
		if (!TargetDirection.IsNearlyZero())
		{
			const FVector2D BiasedExitTarget = CurrentExitTarget + TargetDirection.GetSafeNormal() * 48.0f;
			Profile.OffsetPixels = BiasedExitTarget - VisualSlotView.ScreenPosition;
		}
	};

	if (!SlotMotionConfig.bEnableReadableTransitionOrigins)
	{
		Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Profile.ScaleMultiplier = 1.0f;
		Profile.AngleOffsetDegrees = 0.0f;
		ApplyPlayedTargetBias();
		return Profile;
	}

	FVector2D ExitPosition = VisualSlotView.ScreenPosition;
	switch (Profile.OriginMode)
	{
	case EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset:
		ExitPosition = VisualSlotView.AnchorWidgetPosition + Profile.OffsetPixels;
		Profile.OffsetPixels = ExitPosition - VisualSlotView.ScreenPosition;
		break;
	case EWacomFirstPersonCardTransitionOriginMode::ViewportAnchor:
		if (ResolveViewportAnchorPosition(Profile.ViewportAnchor, ExitPosition))
		{
			Profile.OffsetPixels = (ExitPosition + Profile.OffsetPixels) - VisualSlotView.ScreenPosition;
		}
		break;
	case EWacomFirstPersonCardTransitionOriginMode::SlotOffset:
	default:
		break;
	}

	ApplyPlayedTargetBias();

	return Profile;
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

void UWacomFirstPersonCardLayerWidget::HandleSlotCardTargetHovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!CardTargetHandle.IsValid())
	{
		return;
	}

	HoveredCardTargetHandle = CardTargetHandle;
	HoveredCardTargetSlotView = SlotView;
	OnCardTargetHoveredNative.Broadcast(CardTargetHandle, SlotView);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotCardTargetUnhovered(
	const FWacomInteractionTargetHandle& CardTargetHandle,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (HoveredCardTargetHandle.IsValid()
		&& HoveredCardTargetHandle.CardInstanceId == CardTargetHandle.CardInstanceId)
	{
		HoveredCardTargetHandle = FWacomInteractionTargetHandle();
		HoveredCardTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	}
	OnCardTargetUnhoveredNative.Broadcast(CardTargetHandle, SlotView);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	CurrentDragView = DragView;
	if (CurrentDragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		&& CurrentDragView.bCommitArmed
		&& CardDragConfig.bEnableDragTargetFeedback)
	{
		CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CommitReady;
	}
	OnCardDragStartedNative.Broadcast(CardInstanceId, CurrentDragView);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	const FWacomFirstPersonCardDragView PreviousDragView = CurrentDragView;
	CurrentDragView = DragView;
	if (PreviousDragView.CardInstanceId == CurrentDragView.CardInstanceId)
	{
		if (PreviousDragView.CurrentTarget.IsValid()
			&& PreviousDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
		{
			CurrentDragView.CurrentTarget = PreviousDragView.CurrentTarget;
			CurrentDragView.bTargetValid = PreviousDragView.bTargetValid;
			CurrentDragView.TargetFeedbackState = PreviousDragView.TargetFeedbackState;
			CurrentDragView.bHasFeedbackTargetScreenPosition = PreviousDragView.bHasFeedbackTargetScreenPosition;
			CurrentDragView.FeedbackTargetScreenPosition = PreviousDragView.FeedbackTargetScreenPosition;
		}
		if (PreviousDragView.CurrentTarget.IsValid()
			&& PreviousDragView.CurrentTarget.TargetKind != EWacomInteractionTargetKind::Card)
		{
			CurrentDragView.CurrentTarget = PreviousDragView.CurrentTarget;
			CurrentDragView.bTargetValid = PreviousDragView.bTargetValid;
		}
		if (PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			CurrentDragView.TargetFeedbackState = PreviousDragView.TargetFeedbackState;
		}
		if (PreviousDragView.bHasFeedbackTargetScreenPosition
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			CurrentDragView.bHasFeedbackTargetScreenPosition = true;
			CurrentDragView.FeedbackTargetScreenPosition = PreviousDragView.FeedbackTargetScreenPosition;
		}
	}
	FWacomInteractionTargetHandle PointerCardTargetHandle;
	FWacomFirstPersonCardLayerSlotView PointerCardTargetSlotView;
	if (TryResolveCardTargetUnderDragPointer(
		CurrentDragView,
		PointerCardTargetHandle,
		PointerCardTargetSlotView))
	{
		CurrentDragView.CurrentTarget = PointerCardTargetHandle;
		CurrentDragView.bTargetValid = false;
		CurrentDragView.TargetFeedbackState = CardDragConfig.bEnableDragTargetFeedback
			? EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			: EWacomFirstPersonCardDragTargetFeedbackState::None;
		CurrentDragView.bHasFeedbackTargetScreenPosition = true;
		CurrentDragView.FeedbackTargetScreenPosition = PointerCardTargetSlotView.ScreenPosition;
	}
	else if (CurrentDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		CurrentDragView.CurrentTarget = FWacomInteractionTargetHandle();
		CurrentDragView.bTargetValid = false;
		if (CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		}
		CurrentDragView.bHasFeedbackTargetScreenPosition = false;
		CurrentDragView.FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	else if (!CurrentDragView.CurrentTarget.IsValid() && HoveredCardTargetHandle.IsValid())
	{
		CurrentDragView.CurrentTarget = HoveredCardTargetHandle;
		if (CardDragConfig.bEnableDragTargetFeedback)
		{
			CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CardProbe;
		}
	}
	if (CurrentDragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		&& CurrentDragView.bCommitArmed
		&& CardDragConfig.bEnableDragTargetFeedback)
	{
		CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CommitReady;
	}
	ApplyCardProbeFeedbackForCurrentDrag();
	OnCardDragUpdatedNative.Broadcast(CardInstanceId, CurrentDragView);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	const FWacomFirstPersonCardDragView PreviousDragView = CurrentDragView;
	CurrentDragView = DragView;
	FWacomInteractionTargetHandle PreservedCardTargetHandle;
	bool bPreservedCardTargetValid = false;
	EWacomFirstPersonCardDragTargetFeedbackState PreservedCardTargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	bool bPreservedCardTargetFeedbackPosition = false;
	FVector2D PreservedCardTargetScreenPosition = FVector2D::ZeroVector;
	if (PreviousDragView.CardInstanceId == CurrentDragView.CardInstanceId)
	{
		if (PreviousDragView.CurrentTarget.IsValid()
			&& PreviousDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
		{
			PreservedCardTargetHandle = PreviousDragView.CurrentTarget;
			bPreservedCardTargetValid = PreviousDragView.bTargetValid;
			PreservedCardTargetFeedbackState = PreviousDragView.TargetFeedbackState;
			bPreservedCardTargetFeedbackPosition = PreviousDragView.bHasFeedbackTargetScreenPosition;
			PreservedCardTargetScreenPosition = PreviousDragView.FeedbackTargetScreenPosition;
		}
		if (PreviousDragView.CurrentTarget.IsValid()
			&& PreviousDragView.CurrentTarget.TargetKind != EWacomInteractionTargetKind::Card)
		{
			CurrentDragView.CurrentTarget = PreviousDragView.CurrentTarget;
			CurrentDragView.bTargetValid = PreviousDragView.bTargetValid;
		}
		if (PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			CurrentDragView.TargetFeedbackState = PreviousDragView.TargetFeedbackState;
		}
		if (PreviousDragView.bHasFeedbackTargetScreenPosition
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			&& PreviousDragView.TargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			CurrentDragView.bHasFeedbackTargetScreenPosition = true;
			CurrentDragView.FeedbackTargetScreenPosition = PreviousDragView.FeedbackTargetScreenPosition;
		}
	}
	if (PreservedCardTargetHandle.IsValid())
	{
		CurrentDragView.CurrentTarget = PreservedCardTargetHandle;
		CurrentDragView.bTargetValid = bPreservedCardTargetValid;
		CurrentDragView.TargetFeedbackState = PreservedCardTargetFeedbackState;
		CurrentDragView.bHasFeedbackTargetScreenPosition = bPreservedCardTargetFeedbackPosition;
		CurrentDragView.FeedbackTargetScreenPosition = bPreservedCardTargetFeedbackPosition
			? PreservedCardTargetScreenPosition
			: FVector2D::ZeroVector;
	}
	FWacomInteractionTargetHandle PointerCardTargetHandle;
	FWacomFirstPersonCardLayerSlotView PointerCardTargetSlotView;
	if (TryResolveCardTargetUnderDragPointer(
		CurrentDragView,
		PointerCardTargetHandle,
		PointerCardTargetSlotView))
	{
		CurrentDragView.CurrentTarget = PointerCardTargetHandle;
		CurrentDragView.bTargetValid = false;
		CurrentDragView.TargetFeedbackState = CardDragConfig.bEnableDragTargetFeedback
			? EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			: EWacomFirstPersonCardDragTargetFeedbackState::None;
		CurrentDragView.bHasFeedbackTargetScreenPosition = true;
		CurrentDragView.FeedbackTargetScreenPosition = PointerCardTargetSlotView.ScreenPosition;
	}
	else if (CurrentDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		CurrentDragView.CurrentTarget = FWacomInteractionTargetHandle();
		CurrentDragView.bTargetValid = false;
		if (CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		}
		CurrentDragView.bHasFeedbackTargetScreenPosition = false;
		CurrentDragView.FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	else if (!CurrentDragView.CurrentTarget.IsValid() && HoveredCardTargetHandle.IsValid())
	{
		CurrentDragView.CurrentTarget = HoveredCardTargetHandle;
		if (CardDragConfig.bEnableDragTargetFeedback)
		{
			CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CardProbe;
		}
	}
	ApplyCardProbeFeedbackForCurrentDrag();
	OnCardDragReleasedNative.Broadcast(CardInstanceId, CurrentDragView);
	CurrentDragView = FWacomFirstPersonCardDragView();
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	CurrentDragView = DragView;
	CurrentDragView.GestureState = EWacomFirstPersonCardGestureState::Cancelled;
	OnCardDragCancelledNative.Broadcast(CardInstanceId, CurrentDragView);
	CurrentDragView = FWacomFirstPersonCardDragView();
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

bool UWacomFirstPersonCardLayerWidget::TryResolveCardTargetUnderDragPointer(
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	FWacomFirstPersonCardLayerSlotView& OutTargetSlotView) const
{
	OutTargetHandle = FWacomInteractionTargetHandle();
	OutTargetSlotView = FWacomFirstPersonCardLayerSlotView();

	if (!CardDragConfig.bEnableDragTargetFeedback || !DragView.CardInstanceId.IsValid())
	{
		return false;
	}

	const bool bCanProbeCardTarget =
		DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit;
	if (!bCanProbeCardTarget)
	{
		return false;
	}

	const FVector2D PointerPosition = DragView.CurrentScreenPosition;
	if (PointerPosition.ContainsNaN())
	{
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* BestSlotWidget = nullptr;
	FWacomFirstPersonCardLayerSlotView BestSlotView;
	float BestZOrder = TNumericLimits<float>::Lowest();
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget
			|| SlotWidget->IsExitingForFirstPersonLayer()
			|| !SlotWidget->CanExposeCardTarget())
		{
			continue;
		}

		const FWacomFirstPersonCardLayerSlotView& SlotView = SlotWidget->GetVisualSlotView();
		if (SlotView.Entry.CardInstanceId == DragView.CardInstanceId
			|| !SlotView.Entry.CardInstanceId.IsValid()
			|| !SlotView.bProjected)
		{
			continue;
		}

		FVector2D HitSize = DefaultCardTargetHitSize * FMath::Max(0.01f, SlotView.RenderScale);
		const FVector2D CachedSize = SlotWidget->GetCachedGeometry().GetLocalSize();
		if (CachedSize.X > 1.0f && CachedSize.Y > 1.0f)
		{
			HitSize = CachedSize * FMath::Max(0.01f, SlotView.RenderScale);
		}

		const FVector2D HalfSize = HitSize * 0.5f;
		const FVector2D Delta = PointerPosition - SlotView.ScreenPosition;
		if (FMath::Abs(Delta.X) > HalfSize.X || FMath::Abs(Delta.Y) > HalfSize.Y)
		{
			continue;
		}

		const float SlotZOrder = static_cast<float>(SlotView.ZOrder);
		const float DistanceSquared = Delta.SizeSquared();
		if (!BestSlotWidget
			|| SlotZOrder > BestZOrder
			|| (FMath::IsNearlyEqual(SlotZOrder, BestZOrder) && DistanceSquared < BestDistanceSquared))
		{
			BestSlotWidget = SlotWidget.Get();
			BestSlotView = SlotView;
			BestZOrder = SlotZOrder;
			BestDistanceSquared = DistanceSquared;
		}
	}

	if (!BestSlotWidget)
	{
		return false;
	}

	OutTargetSlotView = BestSlotView;
	OutTargetHandle = BestSlotWidget->BuildCardTargetHandle();
	return OutTargetHandle.IsValid();
}

void UWacomFirstPersonCardLayerWidget::ApplyCardProbeFeedbackForCurrentDrag()
{
	const bool bCardProbeActive =
		CardDragConfig.bEnableDragTargetFeedback
		&& (CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		&& CurrentDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card
		&& CurrentDragView.CurrentTarget.CardInstanceId.IsValid();

	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}

		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		const bool bProbeTarget =
			bCardProbeActive
			&& SlotCardId == CurrentDragView.CurrentTarget.CardInstanceId
			&& SlotCardId != CurrentDragView.CardInstanceId;
		SlotWidget->SetCardDragTargetAffordanceFeedback(
			bProbeTarget
				? CurrentDragView.TargetFeedbackState
				: EWacomFirstPersonCardDragTargetFeedbackState::None,
			CurrentDragView.bTargetValid);
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::ApplyCardTargetAffordances(
	const FWacomInteractionTargetHandle& FocusTargetHandle,
	EWacomFirstPersonCardDragTargetFeedbackState FocusFeedbackState,
	bool bFocusTargetValid,
	const TArray<FWacomFirstPersonCardTargetAffordance>& CardTargetAffordances)
{
	TMap<FGuid, FWacomFirstPersonCardTargetAffordance> AffordanceByCardId;
	AffordanceByCardId.Reserve(CardTargetAffordances.Num());
	for (const FWacomFirstPersonCardTargetAffordance& Affordance : CardTargetAffordances)
	{
		if (Affordance.CardInstanceId.IsValid()
			&& Affordance.CardInstanceId != CurrentDragView.CardInstanceId)
		{
			AffordanceByCardId.Add(Affordance.CardInstanceId, Affordance);
		}
	}

	const bool bHasCardFocus =
		FocusTargetHandle.TargetKind == EWacomInteractionTargetKind::Card
		&& FocusTargetHandle.CardInstanceId.IsValid()
		&& FocusTargetHandle.CardInstanceId != CurrentDragView.CardInstanceId;

	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}

		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (!SlotCardId.IsValid())
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(EWacomFirstPersonCardDragTargetFeedbackState::None, false);
			continue;
		}
		if (SlotCardId == CurrentDragView.CardInstanceId)
		{
			continue;
		}

		EWacomFirstPersonCardDragTargetFeedbackState State =
			EWacomFirstPersonCardDragTargetFeedbackState::None;
		bool bCanSubmit = false;
		if (const FWacomFirstPersonCardTargetAffordance* Affordance = AffordanceByCardId.Find(SlotCardId))
		{
			State = Affordance->FeedbackState;
			bCanSubmit = Affordance->bCanSubmit;
		}

		if (bHasCardFocus && SlotCardId == FocusTargetHandle.CardInstanceId)
		{
			State = FocusFeedbackState;
			if (State == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget)
			{
				State = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
			}
			if (State == EWacomFirstPersonCardDragTargetFeedbackState::Invalid)
			{
				State = EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
			}
			bCanSubmit = bFocusTargetValid;
		}

		SlotWidget->SetCardDragTargetAffordanceFeedback(State, bCanSubmit);
	}
}

void UWacomFirstPersonCardLayerWidget::ApplyDragFeedbackToCurrentDragView(
	const FWacomInteractionTargetHandle& TargetHandle,
	bool bValidTarget,
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	const TOptional<FVector2D>& FeedbackTargetScreenPosition)
{
	CurrentDragView.CurrentTarget = TargetHandle;
	CurrentDragView.bTargetValid = TargetHandle.IsValid() && bValidTarget;
	CurrentDragView.TargetFeedbackState = FeedbackState;
	if (FeedbackTargetScreenPosition.IsSet())
	{
		CurrentDragView.bHasFeedbackTargetScreenPosition = true;
		CurrentDragView.FeedbackTargetScreenPosition = FeedbackTargetScreenPosition.GetValue();
	}
	else
	{
		CurrentDragView.bHasFeedbackTargetScreenPosition = false;
		CurrentDragView.FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
}

FLinearColor UWacomFirstPersonCardLayerWidget::ResolveAimArrowColor() const
{
	FLinearColor LineColor = CardDragConfig.DragInvalidTargetColor;
	if (CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
		|| CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
		|| CurrentDragView.bTargetValid)
	{
		LineColor = CardDragConfig.DragValidTargetColor;
	}
	else if (CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe)
	{
		LineColor = CardDragConfig.DragCardProbeTargetColor;
	}
	else if (CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ZoneProbe)
	{
		LineColor = CardDragConfig.DragCardProbeTargetColor;
	}
	LineColor.A = FMath::Clamp(FMath::Max(LineColor.A, 0.01f), 0.0f, 1.0f);
	return LineColor;
}

FVector2D UWacomFirstPersonCardLayerWidget::ResolveAimArrowEnd() const
{
	if (CardDragConfig.bEnableDragTargetFeedback
		&& CardDragConfig.bSnapAimArrowToValidWorldTarget
		&& CurrentDragView.TargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
		&& CurrentDragView.bHasFeedbackTargetScreenPosition)
	{
		return FMath::Lerp(
			CurrentDragView.CurrentScreenPosition,
			CurrentDragView.FeedbackTargetScreenPosition,
			CardDragConfig.DragAimArrowSnapBlend);
	}
	return CurrentDragView.CurrentScreenPosition;
}

