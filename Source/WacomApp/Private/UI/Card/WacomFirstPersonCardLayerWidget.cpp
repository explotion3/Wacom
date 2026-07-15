// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Rendering/DrawElements.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerConfigUtils.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardPileTransferWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

namespace
{
	const FVector2D CardAlignment(0.5f, 0.5f);
	constexpr float SlotRefreshFloatTolerance = 0.01f;
	constexpr float AuthoredAimArrowLineThickness = 3.0f;
	constexpr float AuthoredAimArrowHeadLength = 18.0f;
	constexpr float AuthoredAimArrowHeadWidth = 9.0f;

	bool IsValidPresentationAnchorPoint(
		const FWacomFirstPersonCardPresentationAnchorPoint& AnchorPoint)
	{
		return AnchorPoint.bValid
			&& FMath::IsFinite(AnchorPoint.WidgetPosition.X)
			&& FMath::IsFinite(AnchorPoint.WidgetPosition.Y);
	}

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

	bool AreFirstPersonSlotFloatsEquivalent(float A, float B)
	{
		return FMath::IsNearlyEqual(A, B, SlotRefreshFloatTolerance);
	}

	bool AreFirstPersonSlotVectorsEquivalent(const FVector2D& A, const FVector2D& B)
	{
		return A.Equals(B, SlotRefreshFloatTolerance);
	}

	bool AreTextsEquivalent(const FText& A, const FText& B)
	{
		return A.EqualTo(B);
	}

	bool AreFirstPersonEffectBadgesEquivalent(
		const TArray<FWacomCardViewEffectBadge>& A,
		const TArray<FWacomCardViewEffectBadge>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].Kind != B[Index].Kind
				|| A[Index].Value != B[Index].Value
				|| !AreTextsEquivalent(A[Index].DisplayText, B[Index].DisplayText))
			{
				return false;
			}
		}
		return true;
	}

	bool AreFirstPersonCardViewDataEquivalent(const FWacomCardViewData& A, const FWacomCardViewData& B)
	{
		return AreTextsEquivalent(A.Name, B.Name)
			&& AreTextsEquivalent(A.TypeText, B.TypeText)
			&& AreTextsEquivalent(A.Description, B.Description)
			&& A.Cost == B.Cost
			&& A.bShowCost == B.bShowCost
			&& A.Rarity == B.Rarity
			&& A.Value == B.Value
			&& A.bShowValue == B.bShowValue
			&& AreTextsEquivalent(A.PhysiqueText, B.PhysiqueText)
			&& A.bShowPhysique == B.bShowPhysique
			&& AreFirstPersonEffectBadgesEquivalent(A.EffectBadges, B.EffectBadges)
			&& A.bDisabled == B.bDisabled
			&& A.Durability == B.Durability
			&& A.bShowDurability == B.bShowDurability
			&& A.Art == B.Art;
	}

	bool AreLayerEntriesEquivalent(
		const FWacomFirstPersonCardLayerEntry& A,
		const FWacomFirstPersonCardLayerEntry& B)
	{
		return A.CardInstanceId == B.CardInstanceId
			&& AreFirstPersonCardViewDataEquivalent(A.CardViewData, B.CardViewData)
			&& A.Zone == B.Zone
			&& A.bIsHandAnchor == B.bIsHandAnchor
			&& A.bIsPlayable == B.bIsPlayable
			&& A.bIsPendingTargeting == B.bIsPendingTargeting
			&& A.InteractionIntent == B.InteractionIntent;
	}

	FVector2D ResolveInputHitCenter(const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		return SlotView.InputHitCenter.IsNearlyZero() ? SlotView.ScreenPosition : SlotView.InputHitCenter;
	}

	float ResolveInputHitScale(const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		return FMath::Max(0.01f, SlotView.InputHitScale > 0.0f ? SlotView.InputHitScale : SlotView.RenderScale);
	}

	float ResolveInputHitAngleDegrees(const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		return SlotView.InputHitAngleDegrees;
	}

	int32 ResolveInputHitOrder(const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		return SlotView.InputHitOrder != INDEX_NONE ? SlotView.InputHitOrder : SlotView.Index;
	}

	bool IsWidgetPositionInsideStableCardBody(
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FVector2D& BodySize,
		const FVector2D& WidgetPosition)
	{
		if (WidgetPosition.ContainsNaN() || BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
		{
			return false;
		}

		FVector2D LocalDelta = (WidgetPosition - ResolveInputHitCenter(SlotView)) / ResolveInputHitScale(SlotView);
		const float InverseAngleRadians = FMath::DegreesToRadians(-ResolveInputHitAngleDegrees(SlotView));
		const float CosAngle = FMath::Cos(InverseAngleRadians);
		const float SinAngle = FMath::Sin(InverseAngleRadians);
		LocalDelta = FVector2D(
			LocalDelta.X * CosAngle - LocalDelta.Y * SinAngle,
			LocalDelta.X * SinAngle + LocalDelta.Y * CosAngle);

		const FVector2D HalfBodySize = BodySize * 0.5f;
		return FMath::Abs(LocalDelta.X) <= HalfBodySize.X
			&& FMath::Abs(LocalDelta.Y) <= HalfBodySize.Y;
	}

	void AccumulateStableCardBodyBounds(
		const FWacomFirstPersonCardLayerSlotView& SlotView,
		const FVector2D& BodySize,
		FVector2D& InOutMin,
		FVector2D& InOutMax,
		bool& bInOutHasBounds)
	{
		if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
		{
			return;
		}

		const FVector2D Center = ResolveInputHitCenter(SlotView);
		const float Scale = ResolveInputHitScale(SlotView);
		const float AngleRadians = FMath::DegreesToRadians(ResolveInputHitAngleDegrees(SlotView));
		const float CosAngle = FMath::Cos(AngleRadians);
		const float SinAngle = FMath::Sin(AngleRadians);
		const FVector2D HalfBodySize = BodySize * 0.5f;
		const FVector2D LocalCorners[4] =
		{
			FVector2D(-HalfBodySize.X, -HalfBodySize.Y),
			FVector2D(HalfBodySize.X, -HalfBodySize.Y),
			FVector2D(HalfBodySize.X, HalfBodySize.Y),
			FVector2D(-HalfBodySize.X, HalfBodySize.Y)
		};

		for (const FVector2D& LocalCorner : LocalCorners)
		{
			const FVector2D ScaledCorner = LocalCorner * Scale;
			const FVector2D RotatedCorner(
				ScaledCorner.X * CosAngle - ScaledCorner.Y * SinAngle,
				ScaledCorner.X * SinAngle + ScaledCorner.Y * CosAngle);
			const FVector2D WorldCorner = Center + RotatedCorner;
			if (!bInOutHasBounds)
			{
				InOutMin = WorldCorner;
				InOutMax = WorldCorner;
				bInOutHasBounds = true;
				continue;
			}

			InOutMin.X = FMath::Min(InOutMin.X, WorldCorner.X);
			InOutMin.Y = FMath::Min(InOutMin.Y, WorldCorner.Y);
			InOutMax.X = FMath::Max(InOutMax.X, WorldCorner.X);
			InOutMax.Y = FMath::Max(InOutMax.Y, WorldCorner.Y);
		}
	}

	bool IsResolvedCardTargetFeedbackState(EWacomFirstPersonCardDragTargetFeedbackState FeedbackState)
	{
		return FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	}

	bool IsCardTargetFeedbackState(EWacomFirstPersonCardDragTargetFeedbackState FeedbackState)
	{
		return FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| IsResolvedCardTargetFeedbackState(FeedbackState);
	}

	void ApplyPointerCardTargetToDragView(
		FWacomFirstPersonCardDragView& DragView,
		const FWacomFirstPersonCardDragView& PreviousDragView,
		const FWacomInteractionTargetHandle& PointerCardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& PointerCardTargetSlotView)
	{
		const bool bSameResolvedCardTarget =
			PreviousDragView.CardInstanceId == DragView.CardInstanceId
			&& PreviousDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card
			&& PreviousDragView.CurrentTarget.CardInstanceId.IsValid()
			&& PreviousDragView.CurrentTarget.CardInstanceId == PointerCardTargetHandle.CardInstanceId
			&& IsResolvedCardTargetFeedbackState(PreviousDragView.TargetFeedbackState);

		DragView.CurrentTarget = PointerCardTargetHandle;
		DragView.bHasFeedbackTargetScreenPosition = true;
		DragView.FeedbackTargetScreenPosition = PointerCardTargetSlotView.ScreenPosition;
		if (bSameResolvedCardTarget)
		{
			DragView.bTargetValid = PreviousDragView.bTargetValid;
			DragView.TargetFeedbackState = PreviousDragView.TargetFeedbackState;
			if (PreviousDragView.bHasFeedbackTargetScreenPosition)
			{
				DragView.FeedbackTargetScreenPosition = PreviousDragView.FeedbackTargetScreenPosition;
			}
			return;
		}

		DragView.bTargetValid = false;
		DragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::CardProbe;
	}
}

void UWacomFirstPersonCardLayerWidget::SetCardViewClass(
	TSubclassOf<UWacomFirstPersonCardViewWidget> InCardViewClass)
{
	TSubclassOf<UWacomFirstPersonCardViewWidget> NewCardViewClass = InCardViewClass;
	if (!NewCardViewClass)
	{
		NewCardViewClass = UWacomFirstPersonCardViewWidget::StaticClass();
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
	const FWacomFirstPersonCardSlotMotionConfig NewConfig = NormalizeSlotMotionConfig(InConfig);
	if (AreSlotMotionConfigsEquivalent(SlotMotionConfig, NewConfig))
	{
		return;
	}

	SlotMotionConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++SlotMotionConfigPropagationCountForTest;
#endif
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

void UWacomFirstPersonCardLayerWidget::SetSlotVisualConfig(
	const FWacomFirstPersonCardSlotVisualConfig& InConfig)
{
	const FWacomFirstPersonCardSlotVisualConfig NewConfig = NormalizeSlotVisualConfig(InConfig);
	if (AreSlotVisualConfigsEquivalent(SlotVisualConfig, NewConfig))
	{
		return;
	}

	SlotVisualConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++SlotVisualConfigPropagationCountForTest;
#endif
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetSlotVisualConfig(SlotVisualConfig);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetSlotVisualConfig(SlotVisualConfig);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::SetSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	const FWacomFirstPersonCardSlotFeedbackConfig NewConfig = NormalizeSlotFeedbackConfig(InConfig);
	if (AreSlotFeedbackConfigsEquivalent(SlotFeedbackConfig, NewConfig))
	{
		return;
	}

	SlotFeedbackConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++SlotFeedbackConfigPropagationCountForTest;
#endif
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
	const FWacomFirstPersonCardDragConfig NewConfig = NormalizeCardDragConfig(InConfig);
	if (AreCardDragConfigsEquivalent(CardDragConfig, NewConfig))
	{
		return;
	}

	CardDragConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++CardDragConfigPropagationCountForTest;
#endif
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
	CurrentDragResolvedIntentDebugSummary = ResolvedIntentDebugSummary;

	ApplyDragFeedbackToCurrentDragView(TargetHandle, bValidTarget, FeedbackState, FeedbackTargetScreenPosition);
	TMap<FGuid, FWacomFirstPersonCardTargetAffordance> AffordanceByCardId;
	for (const FWacomFirstPersonCardTargetAffordance& Affordance : CardTargetAffordances)
	{
		if (Affordance.CardInstanceId.IsValid())
		{
			AffordanceByCardId.Add(Affordance.CardInstanceId, Affordance);
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}
		const FGuid CardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (CardId.IsValid() && CardId == CurrentDragView.CardInstanceId)
		{
			// The Layer owns target resolution, but ReleaseGesture executes on the
			// source Slot. Keep the source's release snapshot in sync so a valid
			// target cannot be lost during synchronous HUD refresh/re-entry.
			SlotWidget->SetCardDragFeedbackTarget(
				TargetHandle,
				bValidTarget,
				FeedbackState,
				FeedbackTargetScreenPosition);
		}
		const FWacomFirstPersonCardTargetAffordance* Affordance = AffordanceByCardId.Find(CardId);
		if (Affordance)
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(
				Affordance->FeedbackState,
				Affordance->bCanSubmit);
		}
		else
		{
			SlotWidget->SetCardDragTargetAffordanceFeedback(
				EWacomFirstPersonCardDragTargetFeedbackState::None,
				false);
		}

		const bool bIsFocusedCardTarget =
			TargetHandle.TargetKind == EWacomInteractionTargetKind::Card
			&& TargetHandle.CardInstanceId.IsValid()
			&& TargetHandle.CardInstanceId == CardId
			&& IsCardTargetFeedbackState(FeedbackState);
		SlotWidget->SetCardDragTargetFocusFeedback(
			bIsFocusedCardTarget
				? FeedbackState
				: EWacomFirstPersonCardDragTargetFeedbackState::None,
			bIsFocusedCardTarget && bValidTarget);
	}
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

bool UWacomFirstPersonCardLayerWidget::TryStartCardDragGesture(const FGuid& CardInstanceId)
{
	return TryStartCardDragGesture(CardInstanceId, TOptional<FVector2D>());
}

bool UWacomFirstPersonCardLayerWidget::TryStartCardDragGesture(
	const FGuid& CardInstanceId,
	const TOptional<FVector2D>& InitialPointerWidgetPosition)
{
	if (!CardInstanceId.IsValid())
	{
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* TargetSlotWidget = nullptr;
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget
			|| SlotWidget->GetSlotView().Entry.CardInstanceId != CardInstanceId)
		{
			continue;
		}
		TargetSlotWidget = SlotWidget.Get();
		break;
	}
	if (!TargetSlotWidget)
	{
		return false;
	}

	if (UWacomFirstPersonCardLayerSlotWidget* ExistingGestureSlot = FindActiveGestureSlot())
	{
		ExistingGestureSlot->CancelCardDragGesture(true);
		ClearCurrentDragState(true);
	}

	const FVector2D GestureOriginPosition = TargetSlotWidget->GetSlotView().ScreenPosition;
	const FVector2D InitialPointerPosition = InitialPointerWidgetPosition.IsSet()
		? InitialPointerWidgetPosition.GetValue()
		: GestureOriginPosition;

	PressedSlotWidget = TargetSlotWidget;
	const bool bStarted = TargetSlotWidget->BeginDragGestureFromFirstPersonLayer(
		GestureOriginPosition,
		InitialPointerPosition);
	if (!bStarted)
	{
		PressedSlotWidget.Reset();
	}
	return bStarted;
}

bool UWacomFirstPersonCardLayerWidget::UpdateActiveDragPointerFromWidgetPosition(
	const FVector2D& WidgetPosition)
{
	return RouteExternalPointerToActiveGestureSlot(WidgetPosition);
}

bool UWacomFirstPersonCardLayerWidget::ReleaseActiveDragGestureFromWidgetPosition(
	const FVector2D& WidgetPosition)
{
	UWacomFirstPersonCardLayerSlotWidget* GestureSlot = FindActiveGestureSlot();
	if (!GestureSlot)
	{
		return false;
	}

	const bool bSuppressHoverAfterRelease = ShouldSuppressOrdinaryHoverForDrag();
	const bool bSuppressInspectDragPromotion =
		GestureSlot->IsInspectScrubActiveForFirstPersonLayer()
		&& IsWidgetPositionInsideInspectScrubArea(WidgetPosition);
	const bool bReleased = GestureSlot->ReleaseGestureFromFirstPersonLayer(
		WidgetPosition,
		bSuppressInspectDragPromotion);
	PressedSlotWidget.Reset();
	if (bSuppressHoverAfterRelease)
	{
		ClearHoveredSlotState(true);
		BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
	}
	else
	{
		UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
	}
	return bReleased;
}

bool UWacomFirstPersonCardLayerWidget::ReleaseActiveDragGestureAtCurrentPointer()
{
	UWacomFirstPersonCardLayerSlotWidget* GestureSlot = FindActiveGestureSlot();
	if (!GestureSlot)
	{
		return false;
	}

	return ReleaseActiveDragGestureFromWidgetPosition(
		GestureSlot->BuildDragView().CurrentScreenPosition);
}

bool UWacomFirstPersonCardLayerWidget::IsCardDragGestureActive() const
{
	return FindActiveGestureSlot() != nullptr;
}

bool UWacomFirstPersonCardLayerWidget::IsKeyboardShortcutCardDragGestureActive() const
{
	return IsCardDragGestureActive()
		&& CurrentDragView.GestureSource
			== EWacomFirstPersonCardGestureSource::KeyboardShortcut;
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
	PendingFeedbackHintsByKey.Reset();
	PlayedPileTransferKeys.Reset();
	DeferredPileTransferHints.Reset();
	if (PileTransferWidget)
	{
		PileTransferWidget->ResetPlayback();
	}
	HoveredCardTargetHandle = FWacomInteractionTargetHandle();
	HoveredCardTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	LastMotionDebugView = FWacomFirstPersonCardLayerMotionDebugView();
}

void UWacomFirstPersonCardLayerWidget::SetPresentationAnchors(
	const FWacomFirstPersonCardPresentationAnchorSet& InAnchors)
{
	PresentationAnchors = InAnchors;
}

void UWacomFirstPersonCardLayerWidget::SetPileTransferConfig(
	const FWacomFirstPersonCardPileTransferConfig& InConfig)
{
	PileTransferConfig = InConfig;
	EnsurePileTransferWidget();
	if (PileTransferWidget)
	{
		PileTransferWidget->SetConfig(PileTransferConfig);
	}
}

void UWacomFirstPersonCardLayerWidget::SetPileTransferHints(
	const TArray<FWacomFirstPersonCardPileTransferHint>& InHints)
{
	EnsurePileTransferWidget();
	if (!PileTransferWidget)
	{
		for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& OutgoingSlot : OutgoingSlotWidgets)
		{
			if (OutgoingSlot && OutgoingSlot->IsHandTargetImpactDeparturePending())
			{
				OutgoingSlot->ReleaseDeferredHandTargetExitNow();
			}
		}
		return;
	}

	for (const FWacomFirstPersonCardPileTransferHint& Hint : InHints)
	{
		const uint64 PlaybackKey = (static_cast<uint64>(Hint.TransferKind) << 32)
			| static_cast<uint32>(Hint.EventSequence);
		if (Hint.EventSequence == INDEX_NONE
			|| Hint.CardInstanceIds.IsEmpty()
			|| PlayedPileTransferKeys.Contains(PlaybackKey))
		{
			continue;
		}
		PlayedPileTransferKeys.Add(PlaybackKey);

		const FWacomFirstPersonCardPresentationAnchorPoint& TargetAnchor =
			PresentationAnchors.Get(Hint.TargetAnchorKind);
		TArray<FVector2D> SourcePositions;
		SourcePositions.Reserve(Hint.CardInstanceIds.Num());
		TArray<UWacomFirstPersonCardLayerSlotWidget*> DiscardSlots;
		const bool bDiscardToPile = Hint.TransferKind
			== FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
		bool bCanPlay = IsValidPresentationAnchorPoint(TargetAnchor)
			&& (!bDiscardToPile || PileTransferConfig.bDiscardToPileEnabled);
		if (bDiscardToPile)
		{
			for (const FGuid& CardInstanceId : Hint.CardInstanceIds)
			{
				UWacomFirstPersonCardLayerSlotWidget* FoundSlot = nullptr;
				for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& OutgoingSlot : OutgoingSlotWidgets)
				{
					if (OutgoingSlot && OutgoingSlot->GetSlotView().Entry.CardInstanceId == CardInstanceId)
					{
						FoundSlot = OutgoingSlot.Get();
						break;
					}
				}
				if (!FoundSlot)
				{
					bCanPlay = false;
					break;
				}
				DiscardSlots.Add(FoundSlot);
				SourcePositions.Add(FoundSlot->GetVisualSlotView().ScreenPosition);
			}
		}
		else if (bCanPlay)
		{
			const FWacomFirstPersonCardPresentationAnchorPoint& SourceAnchor =
				PresentationAnchors.Get(Hint.SourceAnchorKind);
			bCanPlay = IsValidPresentationAnchorPoint(SourceAnchor);
			if (bCanPlay)
			{
				SourcePositions.Init(SourceAnchor.WidgetPosition, Hint.CardInstanceIds.Num());
			}
		}

		bool bWaitingForHandTargetGate = false;
		if (bCanPlay && bDiscardToPile)
		{
			for (UWacomFirstPersonCardLayerSlotWidget* DiscardSlot : DiscardSlots)
			{
				if (DiscardSlot && DiscardSlot->IsHandTargetImpactDeparturePending())
				{
					DiscardSlot->SetHandTargetImpactDepartureOwnedByPileTransfer(true);
					bWaitingForHandTargetGate = bWaitingForHandTargetGate
						|| !DiscardSlot->IsHandTargetImpactDepartureGateOpen();
				}
			}
		}
		if (bWaitingForHandTargetGate)
		{
			DeferredPileTransferHints.Add(Hint);
			continue;
		}

		if (bCanPlay && bDiscardToPile)
		{
			for (UWacomFirstPersonCardLayerSlotWidget* DiscardSlot : DiscardSlots)
			{
				DiscardSlot->SetHandTargetImpactDepartureOwnedByPileTransfer(false);
				const FWacomFirstPersonCardLayerSlotView& Visual = DiscardSlot->GetVisualSlotView();
				const FVector2D CardBodySize = DiscardSlot->GetCardBodyHitSizeForFirstPersonLayer();
				FWacomFirstPersonCardTransitionMotionProfile CollapseProfile;
				CollapseProfile.ScaleMultiplier = PileTransferConfig.bReducedMotion
					? 1.0f
					: PileTransferConfig.Style.GlyphSize.Y / FMath::Max(1.0f, CardBodySize.Y);
				CollapseProfile.AngleOffsetDegrees = -Visual.RenderAngleDegrees;
				CollapseProfile.DurationSeconds = PileTransferConfig.bReducedMotion
					? 0.10f
					: PileTransferConfig.Style.DiscardCollapseSeconds;
				CollapseProfile.EasePower = 2.4f;
				DiscardSlot->BeginExitMotionWithProfile(
					DiscardSlot->GetSlotView(),
					CollapseProfile,
					EWacomFirstPersonCardSlotTransitionKind::Discarded);
			}
		}

		if (!bCanPlay
			|| !PileTransferWidget->Play(Hint, SourcePositions, TargetAnchor.WidgetPosition))
		{
			for (UWacomFirstPersonCardLayerSlotWidget* DiscardSlot : DiscardSlots)
			{
				if (DiscardSlot)
				{
					DiscardSlot->SetHandTargetImpactDepartureOwnedByPileTransfer(false);
					DiscardSlot->ReleaseDeferredHandTargetExitNow();
				}
			}
			FWacomFirstPersonCardPileTransferProgressView Progress;
			Progress.EventSequence = Hint.EventSequence;
			Progress.TransferKind = Hint.TransferKind;
			Progress.ArrivedCount = Hint.CardInstanceIds.Num();
			Progress.TotalCount = Hint.CardInstanceIds.Num();
			Progress.bCompleted = true;
			Progress.bReducedMotion = PileTransferConfig.bReducedMotion;
			Progress.bWasForceCompleted = true;
			HandlePileTransferProgress(Progress);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::ProcessDeferredPileTransferHints()
{
	if (DeferredPileTransferHints.IsEmpty())
	{
		return;
	}

	TArray<FWacomFirstPersonCardPileTransferHint> RetryHints =
		MoveTemp(DeferredPileTransferHints);
	DeferredPileTransferHints.Reset();
	for (const FWacomFirstPersonCardPileTransferHint& Hint : RetryHints)
	{
		const uint64 PlaybackKey = (static_cast<uint64>(Hint.TransferKind) << 32)
			| static_cast<uint32>(Hint.EventSequence);
		PlayedPileTransferKeys.Remove(PlaybackKey);
	}
	SetPileTransferHints(RetryHints);
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
		ResolvedHint.SequenceIndex = FMath::Max(0, Hint.SequenceIndex);
		ResolvedHint.SequenceCount = FMath::Max(1, Hint.SequenceCount);
		ResolvedHint.bPlayCommitFeedback = Hint.bPlayCommitFeedback;
		ResolvedHint.bHasPlayedExitTargetWidgetPosition = Hint.bHasPlayedExitTargetWidgetPosition;
		ResolvedHint.PlayedExitTargetWidgetPosition = Hint.PlayedExitTargetWidgetPosition;
		PendingTransitionHintsByKey.Add(
			Hint.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
			ResolvedHint);
	}
}

void UWacomFirstPersonCardLayerWidget::SetCardFeedbackHints(
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& InHints)
{
	PendingFeedbackHintsByKey.Reset();
	for (const FWacomFirstPersonCardLayerFeedbackHint& Hint : InHints)
	{
		if (!Hint.CardInstanceId.IsValid()
			|| Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::None)
		{
			continue;
		}
		FWacomFirstPersonCardLayerResolvedFeedbackHint ResolvedHint;
		ResolvedHint.FeedbackKind = Hint.FeedbackKind;
		ResolvedHint.SequenceIndex = FMath::Max(0, Hint.SequenceIndex);
		ResolvedHint.SequenceCount = FMath::Max(1, Hint.SequenceCount);
		ResolvedHint.DataRewriteFieldMask = Hint.DataRewriteFieldMask;
		ResolvedHint.DataRewriteTone = Hint.DataRewriteTone;
		ResolvedHint.DataRewriteSeed = Hint.DataRewriteSeed;
		ResolvedHint.bHasDataRewriteCostValues = Hint.bHasDataRewriteCostValues;
		ResolvedHint.DataRewriteCostBefore = Hint.DataRewriteCostBefore;
		ResolvedHint.DataRewriteCostAfter = Hint.DataRewriteCostAfter;
		ResolvedHint.bBlocksPresentationPhase = Hint.bBlocksPresentationPhase;
		FWacomFirstPersonCardLayerResolvedFeedbackBundle& Bundle =
			PendingFeedbackHintsByKey.FindOrAdd(
				Hint.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower));
		if (FWacomFirstPersonCardLayerResolvedFeedbackHint* Existing =
			Bundle.Hints.FindByPredicate(
				[&ResolvedHint](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Candidate)
				{
					return Candidate.FeedbackKind == ResolvedHint.FeedbackKind;
				}))
		{
			if (ResolvedHint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite)
			{
				Existing->DataRewriteFieldMask |= ResolvedHint.DataRewriteFieldMask;
				Existing->DataRewriteTone = ResolvedHint.DataRewriteTone;
				Existing->DataRewriteSeed = ResolvedHint.DataRewriteSeed;
				if (ResolvedHint.bHasDataRewriteCostValues)
				{
					Existing->bHasDataRewriteCostValues = true;
					Existing->DataRewriteCostBefore = ResolvedHint.DataRewriteCostBefore;
					Existing->DataRewriteCostAfter = ResolvedHint.DataRewriteCostAfter;
				}
				Existing->bBlocksPresentationPhase =
					Existing->bBlocksPresentationPhase || ResolvedHint.bBlocksPresentationPhase;
				Existing->SequenceIndex = FMath::Min(
					Existing->SequenceIndex,
					ResolvedHint.SequenceIndex);
				Existing->SequenceCount = FMath::Max(
					Existing->SequenceCount,
					ResolvedHint.SequenceCount);
			}
			else
			{
				*Existing = ResolvedHint;
			}
		}
		else
		{
			Bundle.Hints.Add(ResolvedHint);
		}
	}
}

void UWacomFirstPersonCardLayerWidget::SetCardSlots(
	const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots)
{
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
	if (CanSkipEquivalentSlotRefresh(InSlots))
	{
		LastSlots = InSlots;
		RefreshSlotMotionDebugCounts();
#if WITH_AUTOMATION_TESTS
		++SkippedEquivalentSlotRefreshCountForTest;
#endif
		return;
	}

	LastSlots = InSlots;

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
	TSet<FString> AppliedTransitionHintKeys;
	TSet<FString> AppliedFeedbackHintKeys;
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
		const FWacomFirstPersonCardLayerResolvedTransitionHint* IncomingTransitionHint =
			PendingTransitionHintsByKey.Find(SlotKey);
		const FWacomFirstPersonCardLayerResolvedFeedbackBundle* IncomingFeedbackBundle =
			PendingFeedbackHintsByKey.Find(SlotKey);
		const FWacomFirstPersonCardLayerResolvedFeedbackHint* CardUseReformHint =
			IncomingFeedbackBundle
				? IncomingFeedbackBundle->Hints.FindByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
					})
				: nullptr;
		const FWacomFirstPersonCardLayerResolvedFeedbackHint* CardUseReformOutHint =
			IncomingFeedbackBundle
				? IncomingFeedbackBundle->Hints.FindByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::CardUseReformOut;
					})
				: nullptr;
		const FWacomFirstPersonCardLayerResolvedFeedbackHint* CardUseReformInHint =
			IncomingFeedbackBundle
				? IncomingFeedbackBundle->Hints.FindByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::CardUseReformIn;
					})
				: nullptr;
		const bool bHasCardUseReformHint = CardUseReformHint
			|| CardUseReformOutHint
			|| CardUseReformInHint;
		const FWacomFirstPersonCardLayerResolvedFeedbackHint* DataRewriteHint =
			IncomingFeedbackBundle
				? IncomingFeedbackBundle->Hints.FindByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
					})
				: nullptr;
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile> EnterProfileOverride =
			(IncomingTransitionHint && SlotView.bProjected)
				? GetEnterProfileForTransition(*IncomingTransitionHint, SlotView)
				: TOptional<FWacomFirstPersonCardTransitionMotionProfile>();

		SlotWidget->SetSlotMotionKey(SlotKey);
		SlotWidget->SetCardViewClass(CardViewClass);
		SlotWidget->SetSlotMotionConfig(SlotMotionConfig);
		SlotWidget->SetSlotVisualConfig(SlotVisualConfig);
		SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
		SlotWidget->SetCardDragConfig(CardDragConfig);
		SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
		SlotWidget->SetOwningFirstPersonCardLayer(this);
		const bool bDataRewritePrepared =
			!bIsNewSlotWidget
			&& SlotView.bProjected
			&& DataRewriteHint
			&& !bHasCardUseReformHint
			&& SlotWidget->PrepareCardDataRewriteForSlotView(SlotView, *DataRewriteHint);
		const bool bHasEnterPresentation =
			EnterProfileOverride.IsSet();
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
			SlotWidget->BeginSlotMotionWithEnterProfile(
				SlotView,
				bIsNewSlotWidget || bHasEnterPresentation,
				EnterProfileOverride);
			if (bHasEnterPresentation)
			{
				AppliedTransitionHintKeys.Add(SlotKey);
			}
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotWidget->Slot))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(CardAlignment);
			const FWacomFirstPersonCardLayerSlotView& VisualSlotView = SlotWidget->GetVisualSlotView();
			CanvasSlot->SetPosition(VisualSlotView.ScreenPosition);
			CanvasSlot->SetZOrder(VisualSlotView.ZOrder);
		}

		if (IncomingFeedbackBundle && SlotView.bProjected)
		{
			const FWacomFirstPersonCardLayerResolvedFeedbackHint* RetainedHint =
				IncomingFeedbackBundle->Hints.FindByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::Retained;
					});
			const FWacomFirstPersonCardLayerResolvedFeedbackHint* HandTargetImpactHint =
				IncomingFeedbackBundle->Hints.FindByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
					});
			if (RetainedHint)
			{
				SlotWidget->TriggerRetainedFeedback(
					RetainedHint->SequenceIndex,
					RetainedHint->SequenceCount);
			}
			if (CardUseReformHint)
			{
				SlotWidget->TriggerCardUseReformFeedback();
			}
			if (CardUseReformOutHint)
			{
				SlotWidget->TriggerCardUseReformOutFeedback();
			}
			if (CardUseReformInHint)
			{
				SlotWidget->TriggerCardUseReformInFeedback();
			}
			if (HandTargetImpactHint)
			{
				SlotWidget->TriggerHandTargetImpactFeedback();
			}
			if (DataRewriteHint && !bHasCardUseReformHint && bDataRewritePrepared)
			{
				SlotWidget->TriggerCardDataRewriteFeedback(
					DataRewriteHint->DataRewriteFieldMask,
					DataRewriteHint->DataRewriteTone,
					DataRewriteHint->DataRewriteSeed,
					DataRewriteHint->SequenceIndex,
					DataRewriteHint->SequenceCount,
					DataRewriteHint->bBlocksPresentationPhase);
			}
			AppliedFeedbackHintKeys.Add(SlotKey);
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
			const FString OutgoingSlotKey = SlotWidget->GetSlotMotionKey();
			const FWacomFirstPersonCardLayerResolvedTransitionHint OutgoingTransitionHint =
				PendingTransitionHintsByKey.FindRef(OutgoingSlotKey);
			const FWacomFirstPersonCardLayerResolvedFeedbackBundle* OutgoingFeedbackBundle =
				PendingFeedbackHintsByKey.Find(OutgoingSlotKey);
			const TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfileOverride =
				GetExitProfileForTransition(OutgoingTransitionHint, SlotWidget->GetVisualSlotView());
			const bool bHasHandTargetImpact = OutgoingFeedbackBundle
				&& OutgoingFeedbackBundle->Hints.ContainsByPredicate(
					[](const FWacomFirstPersonCardLayerResolvedFeedbackHint& Hint)
					{
						return Hint.FeedbackKind
							== EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
					});
			if (bHasHandTargetImpact)
			{
				SlotWidget->BeginDeferredExitWithHandTargetImpact(
					SlotWidget->GetSlotView(),
					ExitProfileOverride,
					OutgoingTransitionHint.TransitionKind);
				AppliedFeedbackHintKeys.Add(OutgoingSlotKey);
			}
			else
			{
				SlotWidget->BeginExitMotionWithProfile(
					SlotWidget->GetSlotView(),
					ExitProfileOverride,
					OutgoingTransitionHint.TransitionKind);
			}
			if (ExitProfileOverride.IsSet() || OutgoingTransitionHint.bPlayCommitFeedback)
			{
				AppliedTransitionHintKeys.Add(OutgoingSlotKey);
			}
			if (OutgoingTransitionHint.bPlayCommitFeedback)
			{
				SlotWidget->TriggerCommitFeedback();
			}
			if (OutgoingFeedbackBundle)
			{
				// Outgoing cards never play a local data rewrite. Consume the whole
				// bundle so a later lifecycle refresh cannot replay stale feedback.
				AppliedFeedbackHintKeys.Add(OutgoingSlotKey);
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
	if (HoveredCardInstanceId.IsValid())
	{
		UWacomFirstPersonCardLayerSlotWidget* ActiveHoveredSlot = nullptr;
		for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
		{
			if (SlotWidget
				&& SlotWidget->GetSlotView().Entry.CardInstanceId == HoveredCardInstanceId
				&& !SlotWidget->IsExitingForFirstPersonLayer()
				&& SlotWidget->GetSlotView().bProjected)
			{
				ActiveHoveredSlot = SlotWidget.Get();
				break;
			}
		}
		if (ActiveHoveredSlot)
		{
			HoveredSlotWidget = ActiveHoveredSlot;
			ActiveHoveredSlot->SetHoveredFromFirstPersonLayer(true);
		}
		else
		{
			ClearHoveredSlotState(true);
		}
	}
	if (bHasCurrentPointerView)
	{
		const bool bPointerCardStillActive = SlotWidgets.ContainsByPredicate(
			[this](const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget)
			{
				return SlotWidget
					&& !SlotWidget->IsExitingForFirstPersonLayer()
					&& SlotWidget->GetSlotView().bProjected
					&& SlotWidget->GetSlotView().Entry.CardInstanceId == CurrentPointerView.CardInstanceId;
			});
		if (!bPointerCardStillActive)
		{
			ClearCardPointerView(true);
		}
	}
	LastMotionDebugView.OutgoingFinishedThisUpdate += RemoveOutgoingFinishedSlots();
	RepairSlotMotionInvariants();
	EnforceOutgoingSlotLimit();
	LastMotionDebugView.UntrackedChildRemovedThisUpdate += RemoveUntrackedSlotChildren();
	RefreshSlotMotionDebugCounts();
	ReportSlotMotionDiagnosticsIfNeeded();
	for (const FString& AppliedTransitionHintKey : AppliedTransitionHintKeys)
	{
		PendingTransitionHintsByKey.Remove(AppliedTransitionHintKey);
	}
	for (const FString& AppliedFeedbackHintKey : AppliedFeedbackHintKeys)
	{
		PendingFeedbackHintsByKey.Remove(AppliedFeedbackHintKey);
	}
	for (auto It = PendingFeedbackHintsByKey.CreateIterator(); It; ++It)
	{
		if (!IncomingKeys.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

bool UWacomFirstPersonCardLayerWidget::HasActivePresentationPlayback() const
{
	if (PileTransferWidget && PileTransferWidget->IsPlaybackActive())
	{
		return true;
	}
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget && SlotWidget->HasActivePresentationPlayback())
		{
			return true;
		}
	}

	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget && SlotWidget->HasActivePresentationPlayback())
		{
			return true;
		}
	}

	return false;
}

bool UWacomFirstPersonCardLayerWidget::HasHandTargetImpactReachedPeak(
	const FGuid& CardInstanceId) const
{
	if (!CardInstanceId.IsValid())
	{
		return false;
	}
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget
			&& SlotWidget->GetSlotView().Entry.CardInstanceId == CardInstanceId)
		{
			return SlotWidget->HasHandTargetImpactReachedPeak();
		}
	}
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget
			&& SlotWidget->GetSlotView().Entry.CardInstanceId == CardInstanceId)
		{
			return SlotWidget->HasHandTargetImpactReachedPeak();
		}
	}
	return false;
}

void UWacomFirstPersonCardLayerWidget::ForceSettlePresentationPlayback()
{
	PendingTransitionHintsByKey.Reset();
	PendingFeedbackHintsByKey.Reset();
	DeferredPileTransferHints.Reset();
	if (PileTransferWidget)
	{
		PileTransferWidget->ForceComplete();
	}
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ForceCompletePresentationPlayback();
		}
	}
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ForceCompletePresentationPlayback();
		}
	}
	LastMotionDebugView.OutgoingFinishedThisUpdate += RemoveOutgoingFinishedSlots();
}

bool UWacomFirstPersonCardLayerWidget::CanSkipEquivalentSlotRefresh(
	const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots) const
{
	if (PendingTransitionHintsByKey.Num() > 0
		|| PendingFeedbackHintsByKey.Num() > 0
		|| OutgoingSlotWidgets.Num() > 0
		|| SlotWidgets.Num() != InSlots.Num()
		|| LastSlots.Num() != InSlots.Num()
		|| CountRootCanvasSlotChildren() != SlotWidgets.Num())
	{
		return false;
	}

	TSet<FString> UsedKeys;
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

		const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = SlotWidgets.IsValidIndex(Index)
			? SlotWidgets[Index].Get()
			: nullptr;
		if (!SlotWidget
			|| SlotWidget->GetSlotMotionKey() != SlotKey
			|| SlotWidget->IsExitingForFirstPersonLayer()
			|| !AreSlotViewsEquivalentForRefresh(LastSlots[Index], SlotView)
			|| !AreSlotViewsEquivalentForRefresh(SlotWidget->GetSlotView(), SlotView))
		{
			return false;
		}
	}
	return true;
}

bool UWacomFirstPersonCardLayerWidget::AreSlotViewsEquivalentForRefresh(
	const FWacomFirstPersonCardLayerSlotView& A,
	const FWacomFirstPersonCardLayerSlotView& B) const
{
	return A.Index == B.Index
		&& AreLayerEntriesEquivalent(A.Entry, B.Entry)
		&& AreFirstPersonSlotVectorsEquivalent(A.ScreenPosition, B.ScreenPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.InputHitCenter, B.InputHitCenter)
		&& AreFirstPersonSlotFloatsEquivalent(A.InputHitScale, B.InputHitScale)
		&& AreFirstPersonSlotFloatsEquivalent(A.InputHitAngleDegrees, B.InputHitAngleDegrees)
		&& A.InputHitOrder == B.InputHitOrder
		&& AreFirstPersonSlotVectorsEquivalent(A.RawScreenPosition, B.RawScreenPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.WidgetPosition, B.WidgetPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.UnclampedWidgetPosition, B.UnclampedWidgetPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.SnappedWidgetPosition, B.SnappedWidgetPosition)
		&& A.ProjectionMode == B.ProjectionMode
		&& A.ViewportClampMode == B.ViewportClampMode
		&& AreFirstPersonSlotVectorsEquivalent(A.AnchorWidgetPosition, B.AnchorWidgetPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.UnsmoothedAnchorWidgetPosition, B.UnsmoothedAnchorWidgetPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.SmoothedAnchorWidgetPosition, B.SmoothedAnchorWidgetPosition)
		&& AreFirstPersonSlotVectorsEquivalent(A.AuthoredLayoutOffset, B.AuthoredLayoutOffset)
		&& AreFirstPersonSlotFloatsEquivalent(A.NormalizedHandOffset, B.NormalizedHandOffset)
		&& AreFirstPersonSlotFloatsEquivalent(A.RenderAngleDegrees, B.RenderAngleDegrees)
		&& AreFirstPersonSlotFloatsEquivalent(A.RenderScale, B.RenderScale)
		&& AreFirstPersonSlotFloatsEquivalent(A.PresentationScale, B.PresentationScale)
		&& AreFirstPersonSlotFloatsEquivalent(A.RenderOpacity, B.RenderOpacity)
		&& A.ZOrder == B.ZOrder
		&& AreFirstPersonSlotFloatsEquivalent(A.ViewportScale, B.ViewportScale)
		&& AreFirstPersonSlotFloatsEquivalent(A.OffscreenDistancePixels, B.OffscreenDistancePixels)
		&& AreFirstPersonSlotFloatsEquivalent(A.AnchorScreenSmoothingDistancePixels, B.AnchorScreenSmoothingDistancePixels)
		&& A.bProjected == B.bProjected
		&& A.bClamped == B.bClamped
		&& A.bOutsideViewport == B.bOutsideViewport
		&& A.bPixelSnapped == B.bPixelSnapped
		&& A.bIsHovered == B.bIsHovered
		&& A.bHasPendingTargetingCardInHand == B.bHasPendingTargetingCardInHand
		&& A.bAnchorScreenSmoothed == B.bAnchorScreenSmoothed
		&& A.bBodyLockedLayout == B.bBodyLockedLayout
		&& A.bCurrentCameraProjection == B.bCurrentCameraProjection
		&& A.bLookOffsetAppliedToLayout == B.bLookOffsetAppliedToLayout
		&& A.GestureState == B.GestureState;
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
		ClearCardPointerView(true);
		ClearHoveredSlotState(true);
		ClearCurrentDragState(true);
	}
}

UWacomCardView* UWacomFirstPersonCardLayerWidget::GetCardViewAt(int32 Index) const
{
	const UWacomFirstPersonCardLayerSlotWidget* SlotWidget = GetSlotWidgetAt(Index);
	return SlotWidget ? SlotWidget->GetInnerCardView() : nullptr;
}

UWacomFirstPersonCardViewWidget* UWacomFirstPersonCardLayerWidget::GetFirstPersonCardViewAt(int32 Index) const
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
			SlotWidget->GetCardDragTargetAffordanceFeedbackStateForFirstPersonLayer();
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
FWacomFirstPersonCardLayerAutomationTestView UWacomFirstPersonCardLayerWidget::GetAutomationTestViewForTest() const
{
	FWacomFirstPersonCardLayerAutomationTestView View;
	View.SkippedEquivalentSlotRefreshCount = SkippedEquivalentSlotRefreshCountForTest;
	View.SlotMotionConfigPropagationCount = SlotMotionConfigPropagationCountForTest;
	View.SlotVisualConfigPropagationCount = SlotVisualConfigPropagationCountForTest;
	View.SlotFeedbackConfigPropagationCount = SlotFeedbackConfigPropagationCountForTest;
	View.CardDragConfigPropagationCount = CardDragConfigPropagationCountForTest;
	View.SlotMotionConfig = SlotMotionConfig;
	View.SlotVisualConfig = SlotVisualConfig;
	View.SlotFeedbackConfig = SlotFeedbackConfig;
	View.CardDragConfig = CardDragConfig;
	View.CurrentDragView = CurrentDragView;
	View.CurrentPointerView = CurrentPointerView;
	View.bHasCurrentPointerView = bHasCurrentPointerView;
	View.HoveredCardInstanceId = HoveredCardInstanceId;
	View.AimArrowColor = ResolveAimArrowColor();
	View.AimArrowStart = ResolveAimArrowStart();
	View.AimArrowEnd = ResolveAimArrowEnd();
	const float AimArrowScale = ResolveAimArrowPresentationScale();
	View.AimArrowLineThickness = AuthoredAimArrowLineThickness * AimArrowScale;
	View.AimArrowHeadLength = AuthoredAimArrowHeadLength * AimArrowScale;
	View.AimArrowHeadWidth = AuthoredAimArrowHeadWidth * AimArrowScale;
	View.CardViewClass = CardViewClass;
	return View;
}

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

FGuid UWacomFirstPersonCardLayerWidget::ResolveHoveredCardAtWidgetPositionForTest(
	const FVector2D& WidgetPosition)
{
	BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
	UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
	return HoveredCardInstanceId;
}

bool UWacomFirstPersonCardLayerWidget::HandleSlotPointerEnteredAtWidgetPositionForTest(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& WidgetPosition)
{
	if (RoutePointerToActiveGestureSlot(WidgetPosition))
	{
		if (ShouldSuppressOrdinaryHoverForDrag())
		{
			ClearCardPointerView(true);
			ClearHoveredSlotState(true);
		}
		return true;
	}
	BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
	return UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
}

bool UWacomFirstPersonCardLayerWidget::HandleSlotPointerMovedAtWidgetPositionForTest(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& WidgetPosition)
{
	return HandleSlotPointerMovedRouteActionAtWidgetPositionForTest(SourceSlot, WidgetPosition)
		!= EWacomFirstPersonCardPointerRouteAction::Unhandled;
}

EWacomFirstPersonCardPointerRouteAction
UWacomFirstPersonCardLayerWidget::HandleSlotPointerMovedRouteActionAtWidgetPositionForTest(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& WidgetPosition)
{
	(void)SourceSlot;
	return RouteSlotPointerMovedAtWidgetPosition(WidgetPosition).Action;
}

bool UWacomFirstPersonCardLayerWidget::RequestPressAtWidgetPositionForTest(
	const FVector2D& WidgetPosition)
{
	return RequestPressRouteActionAtWidgetPositionForTest(WidgetPosition)
		!= EWacomFirstPersonCardPointerRouteAction::Unhandled;
}

EWacomFirstPersonCardPointerRouteAction
UWacomFirstPersonCardLayerWidget::RequestPressRouteActionAtWidgetPositionForTest(
	const FVector2D& WidgetPosition)
{
	return RouteSlotPointerPressedAtWidgetPosition(WidgetPosition).Action;
}

bool UWacomFirstPersonCardLayerWidget::RequestReleaseAtWidgetPositionForTest(
	const FVector2D& WidgetPosition)
{
	return RequestReleaseRouteActionAtWidgetPositionForTest(WidgetPosition)
		!= EWacomFirstPersonCardPointerRouteAction::Unhandled;
}

EWacomFirstPersonCardPointerRouteAction
UWacomFirstPersonCardLayerWidget::RequestReleaseRouteActionAtWidgetPositionForTest(
	const FVector2D& WidgetPosition)
{
	return RouteSlotPointerReleasedAtWidgetPosition(WidgetPosition).Action;
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

	EnsurePileTransferWidget();
	ApplyLayerVisibility();
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardLayerWidget::NativeDestruct()
{
	ReleaseOwnedSlateMouseCapture();
	ClearCardPointerView(true);
	ClearHoveredSlotState(false);
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
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	OnHoveredCardSlotUpdatedNative.Clear();
	OnCardTargetHoveredNative.Clear();
	OnCardTargetUnhoveredNative.Clear();
	OnHoveredCardTargetUpdatedNative.Clear();
	OnEnterTransitionStartedNative.Clear();
	OnPileTransferProgressNative.Clear();
	if (PileTransferWidget)
	{
		PileTransferWidget->OnProgressNative.RemoveAll(this);
		PileTransferWidget->ResetPlayback();
	}
	SlotWidgets.Reset();
	OutgoingSlotWidgets.Reset();
	HoveredSlotWidget.Reset();
	HoveredCardInstanceId.Invalidate();
	PressedSlotWidget.Reset();
	HoveredCardTargetHandle = FWacomInteractionTargetHandle();
	HoveredCardTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	CurrentDragView = FWacomFirstPersonCardDragView();
	RootCanvas = nullptr;
	PileTransferWidget = nullptr;
	PendingTransitionHintsByKey.Reset();
	PlayedPileTransferKeys.Reset();
	DeferredPileTransferHints.Reset();
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ProcessDeferredPileTransferHints();
	if (PileTransferWidget)
	{
		PileTransferWidget->TickPlayback(InDeltaTime);
	}
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

	const FVector2D Start = ResolveAimArrowStart();
	const FVector2D End = ResolveAimArrowEnd();
	const FVector2D Direction = End - Start;
	if (Direction.SizeSquared() <= 4.0f)
	{
		return MaxLayerId;
	}

	const FLinearColor LineColor = ResolveAimArrowColor();
	const FVector2D UnitDirection = Direction.GetSafeNormal();
	const FVector2D Perpendicular(-UnitDirection.Y, UnitDirection.X);
	const float PresentationScale = ResolveAimArrowPresentationScale();
	const float ArrowLength = AuthoredAimArrowHeadLength * PresentationScale;
	const float ArrowWidth = AuthoredAimArrowHeadWidth * PresentationScale;
	const float LineThickness = AuthoredAimArrowLineThickness * PresentationScale;
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
		LineThickness);

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
		LineThickness);

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
		LineThickness);

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
	SlotWidget->SetSlotVisualConfig(SlotVisualConfig);
	SlotWidget->SetSlotFeedbackConfig(SlotFeedbackConfig);
	SlotWidget->SetCardDragConfig(CardDragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(bCardLayerInteractionEnabled);
	SlotWidget->SetOwningFirstPersonCardLayer(this);
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

void UWacomFirstPersonCardLayerWidget::EnsurePileTransferWidget()
{
	if (!RootCanvas || PileTransferWidget)
	{
		return;
	}
	PileTransferWidget = WidgetTree->ConstructWidget<UWacomFirstPersonCardPileTransferWidget>(
		UWacomFirstPersonCardPileTransferWidget::StaticClass(),
		TEXT("FirstPersonCardPileTransfer"));
	if (!PileTransferWidget)
	{
		return;
	}
	PileTransferWidget->SetConfig(PileTransferConfig);
	PileTransferWidget->OnProgressNative.AddUObject(
		this,
		&UWacomFirstPersonCardLayerWidget::HandlePileTransferProgress);
	RootCanvas->AddChild(PileTransferWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PileTransferWidget->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetZOrder(1000000);
	}
}

void UWacomFirstPersonCardLayerWidget::HandlePileTransferProgress(
	const FWacomFirstPersonCardPileTransferProgressView& Progress)
{
	OnPileTransferProgressNative.Broadcast(Progress);
}

void UWacomFirstPersonCardLayerWidget::ReleaseOwnedSlateMouseCapture()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	const TSharedPtr<FSlateUser> CursorUser = FSlateApplication::Get().GetCursorUser();
	if (!CursorUser)
	{
		return;
	}
	const FWidgetPath CursorCapturePath = CursorUser->GetCursorCaptorPath();

	auto SlotOwnsMouseCapture = [&CursorCapturePath](const UWacomFirstPersonCardLayerSlotWidget* SlotWidget)
	{
		if (!SlotWidget)
		{
			return false;
		}

		const TSharedPtr<SWidget> CachedSlotWidget = SlotWidget->GetCachedWidget();
		if (CachedSlotWidget.IsValid() && CursorCapturePath.ContainsWidget(CachedSlotWidget.Get()))
		{
			return true;
		}
		const TSharedRef<SWidget> SlotSlateWidget = const_cast<UWacomFirstPersonCardLayerSlotWidget*>(SlotWidget)->TakeWidget();
		return CursorCapturePath.ContainsWidget(&SlotSlateWidget.Get());
	};

	// PressedSlotWidget is assigned only when this layer returns CaptureMouse,
	// so it is the authoritative ownership marker while a gesture is active.
	bool bOwnsMouseCapture = PressedSlotWidget != nullptr;
	if (!bOwnsMouseCapture)
	{
		bOwnsMouseCapture = SlotWidgets.ContainsByPredicate(
			[&SlotOwnsMouseCapture](const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget)
			{
				return SlotOwnsMouseCapture(SlotWidget.Get());
			});
	}
	if (!bOwnsMouseCapture)
	{
		bOwnsMouseCapture = OutgoingSlotWidgets.ContainsByPredicate(
			[&SlotOwnsMouseCapture](const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget)
			{
				return SlotOwnsMouseCapture(SlotWidget.Get());
			});
	}
	if (!bOwnsMouseCapture)
	{
		return;
	}

	FSlateApplication::Get().ReleaseAllPointerCapture(CursorUser->GetUserIndex());
}

void UWacomFirstPersonCardLayerWidget::BindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget)
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->OnCardHoveredNative.RemoveAll(this);
	SlotWidget->OnCardUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardVisualSlotUpdatedNative.RemoveAll(this);
	SlotWidget->OnCardTargetHoveredNative.RemoveAll(this);
	SlotWidget->OnCardTargetUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardDragStartedNative.RemoveAll(this);
	SlotWidget->OnCardDragUpdatedNative.RemoveAll(this);
	SlotWidget->OnCardDragReleasedNative.RemoveAll(this);
	SlotWidget->OnCardDragCancelledNative.RemoveAll(this);
	SlotWidget->OnEnterTransitionStartedNative.RemoveAll(this);
	SlotWidget->SetOwningFirstPersonCardLayer(this);
	SlotWidget->OnCardHoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotHovered);
	SlotWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotUnhovered);
	SlotWidget->OnCardVisualSlotUpdatedNative.AddUObject(
		this,
		&UWacomFirstPersonCardLayerWidget::HandleSlotVisualSlotUpdated);
	SlotWidget->OnCardTargetHoveredNative.AddUObject(
		this,
		&UWacomFirstPersonCardLayerWidget::HandleSlotCardTargetHovered);
	SlotWidget->OnCardTargetUnhoveredNative.AddUObject(
		this,
		&UWacomFirstPersonCardLayerWidget::HandleSlotCardTargetUnhovered);
	SlotWidget->OnCardDragStartedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragStarted);
	SlotWidget->OnCardDragUpdatedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragUpdated);
	SlotWidget->OnCardDragReleasedNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragReleased);
	SlotWidget->OnCardDragCancelledNative.AddUObject(this, &UWacomFirstPersonCardLayerWidget::HandleSlotDragCancelled);
	SlotWidget->OnEnterTransitionStartedNative.AddUObject(
		this,
		&UWacomFirstPersonCardLayerWidget::HandleSlotEnterTransitionStarted);
}

void UWacomFirstPersonCardLayerWidget::UnbindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget)
{
	if (!SlotWidget)
	{
		return;
	}
	if (PressedSlotWidget.Get() == SlotWidget)
	{
		ReleaseOwnedSlateMouseCapture();
	}

	SlotWidget->OnCardHoveredNative.RemoveAll(this);
	SlotWidget->OnCardUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardVisualSlotUpdatedNative.RemoveAll(this);
	SlotWidget->OnCardTargetHoveredNative.RemoveAll(this);
	SlotWidget->OnCardTargetUnhoveredNative.RemoveAll(this);
	SlotWidget->OnCardDragStartedNative.RemoveAll(this);
	SlotWidget->OnCardDragUpdatedNative.RemoveAll(this);
	SlotWidget->OnCardDragReleasedNative.RemoveAll(this);
	SlotWidget->OnCardDragCancelledNative.RemoveAll(this);
	SlotWidget->OnEnterTransitionStartedNative.RemoveAll(this);
	SlotWidget->SetOwningFirstPersonCardLayer(nullptr);
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

void UWacomFirstPersonCardLayerWidget::ClearHoveredSlotState(bool bBroadcastUnhover)
{
	UWacomFirstPersonCardLayerSlotWidget* PreviousSlotWidget = HoveredSlotWidget.Get();
	if (!PreviousSlotWidget)
	{
		HoveredSlotWidget.Reset();
		HoveredCardInstanceId.Invalidate();
		ClearHoveredCardTargetState(bBroadcastUnhover);
		return;
	}

	FWacomFirstPersonCardLayerSlotView PreviousSlotView = PreviousSlotWidget->GetVisualSlotView();
	PreviousSlotView.bIsHovered = false;
	const FGuid PreviousCardId = PreviousSlotWidget->GetSlotView().Entry.CardInstanceId;
	HoveredSlotWidget.Reset();
	HoveredCardInstanceId.Invalidate();
	PreviousSlotWidget->SetHoveredFromFirstPersonLayer(false);
	if (bBroadcastUnhover && PreviousCardId.IsValid())
	{
		OnCardUnhoveredNative.Broadcast(PreviousCardId, PreviousSlotView);
	}
	ClearHoveredCardTargetState(bBroadcastUnhover);
}

void UWacomFirstPersonCardLayerWidget::ClearCardPointerView(bool bBroadcastPointerLeft)
{
	if (!bHasCurrentPointerView)
	{
		return;
	}

	bHasCurrentPointerView = false;
	CurrentPointerView = FWacomFirstPersonCardPointerView();
	if (bBroadcastPointerLeft)
	{
		OnCardPointerLeftNative.Broadcast();
	}
}

void UWacomFirstPersonCardLayerWidget::ClearCurrentDragState(bool bBroadcastCancel)
{
	ReleaseOwnedSlateMouseCapture();
	const bool bHadDrag = CurrentDragView.CardInstanceId.IsValid()
		&& CurrentDragView.GestureState != EWacomFirstPersonCardGestureState::Idle
		&& CurrentDragView.GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	const FGuid PreviousCardId = CurrentDragView.CardInstanceId;
	FWacomFirstPersonCardDragView PreviousDragView = CurrentDragView;
	CurrentDragView = FWacomFirstPersonCardDragView();
	CurrentDragResolvedIntentDebugSummary.Reset();
	PressedSlotWidget.Reset();
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ClearCardDragTargetFeedback();
		}
	}
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ClearCardDragTargetFeedback();
		}
	}
	if (bHadDrag && bBroadcastCancel)
	{
		PreviousDragView.GestureState = EWacomFirstPersonCardGestureState::Cancelled;
		OnCardDragCancelledNative.Broadcast(PreviousCardId, PreviousDragView);
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

bool UWacomFirstPersonCardLayerWidget::ShouldSuppressOrdinaryHoverForDrag() const
{
	if (CurrentDragView.CardInstanceId.IsValid())
	{
		return true;
	}

	const UWacomFirstPersonCardLayerSlotWidget* ActiveGestureSlot = FindActiveGestureSlot();
	return ActiveGestureSlot
		&& ActiveGestureSlot->GetGestureStateForFirstPersonLayer() != EWacomFirstPersonCardGestureState::Pressed;
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

bool UWacomFirstPersonCardLayerWidget::ResolvePointerViewportPosition(
	const FVector2D& WidgetPosition,
	FVector2D& OutPointerViewportPosition,
	FVector2D& OutPointerNormalizedViewportPosition) const
{
	FVector2D WidgetViewportSize = FVector2D::ZeroVector;
#if WITH_AUTOMATION_TESTS
	if (WidgetViewportSizeOverrideForTest.IsSet())
	{
		WidgetViewportSize = WidgetViewportSizeOverrideForTest.GetValue();
	}
#endif

	if (WidgetViewportSize.X <= 0.0f || WidgetViewportSize.Y <= 0.0f)
	{
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
		WidgetViewportSize = ViewportPixelSize / ViewportScale;
	}

	if (WidgetViewportSize.X <= 0.0f || WidgetViewportSize.Y <= 0.0f)
	{
		return false;
	}

	OutPointerViewportPosition = WidgetPosition;
	OutPointerNormalizedViewportPosition = FVector2D(
		FMath::Clamp((WidgetPosition.X / WidgetViewportSize.X) * 2.0f - 1.0f, -1.0f, 1.0f),
		FMath::Clamp((WidgetPosition.Y / WidgetViewportSize.Y) * 2.0f - 1.0f, -1.0f, 1.0f));
	return true;
}

TOptional<FWacomFirstPersonCardTransitionMotionProfile> UWacomFirstPersonCardLayerWidget::GetEnterProfileForTransition(
	const FWacomFirstPersonCardLayerResolvedTransitionHint& TransitionHint,
	const FWacomFirstPersonCardLayerSlotView& TargetSlotView) const
{
	if (!SlotMotionConfig.bEnableEventAwareTransitions)
	{
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	FWacomFirstPersonCardTransitionMotionProfile Profile;
	switch (TransitionHint.TransitionKind)
	{
	case EWacomFirstPersonCardSlotTransitionKind::Drawn:
	case EWacomFirstPersonCardSlotTransitionKind::RunHandEntered:
		Profile.OriginMode = SlotMotionConfig.DrawnEnterOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.DrawnEnterOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.DrawnEnterViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.DrawnEnterScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.DrawnEnterAngleOffsetDegrees;
		Profile.StartDelaySeconds =
			FMath::Max(0, TransitionHint.SequenceIndex) * SlotMotionConfig.DrawnEnterStaggerSeconds;
		Profile.DurationSeconds = SlotMotionConfig.DrawnEnterDurationSeconds;
		Profile.ArcLiftPixels = SlotMotionConfig.DrawnEnterArcLiftPixels;
		Profile.EasePower = SlotMotionConfig.DrawnEnterEasePower;
		Profile.bBlockInteractionDuringPlayback = SlotMotionConfig.bBlockInteractionDuringDrawnEnter;
		break;
	case EWacomFirstPersonCardSlotTransitionKind::Gained:
		Profile.OriginMode = SlotMotionConfig.GainedEnterOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.GainedEnterOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.GainedEnterViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.GainedEnterScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.GainedEnterAngleOffsetDegrees;
		Profile.StartDelaySeconds =
			FMath::Max(0, TransitionHint.SequenceIndex) * SlotMotionConfig.GainedEnterStaggerSeconds;
		Profile.DurationSeconds = SlotMotionConfig.GainedEnterDurationSeconds;
		Profile.ArcLiftPixels = SlotMotionConfig.GainedEnterArcLiftPixels;
		Profile.EasePower = SlotMotionConfig.GainedEnterEasePower;
		Profile.bBlockInteractionDuringPlayback = SlotMotionConfig.bBlockInteractionDuringGainedEnter;
		break;
	case EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered:
		Profile.OriginMode = SlotMotionConfig.HandAnchorEnterOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.HandAnchorEnterOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.HandAnchorEnterViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.HandAnchorEnterScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.HandAnchorEnterAngleOffsetDegrees;
		Profile.StartDelaySeconds =
			FMath::Max(0, TransitionHint.SequenceIndex) * SlotMotionConfig.HandAnchorEnterStaggerSeconds;
		Profile.DurationSeconds = SlotMotionConfig.HandAnchorEnterDurationSeconds;
		Profile.ArcLiftPixels = SlotMotionConfig.HandAnchorEnterArcLiftPixels;
		Profile.EasePower = SlotMotionConfig.HandAnchorEnterEasePower;
		Profile.bBlockInteractionDuringPlayback = SlotMotionConfig.bBlockInteractionDuringHandAnchorEnter;
		break;
	default:
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	Profile.TransitionKind = TransitionHint.TransitionKind;
	if (SlotMotionConfig.bEnableEnterSounds)
	{
		switch (TransitionHint.TransitionKind)
		{
		case EWacomFirstPersonCardSlotTransitionKind::Drawn:
			Profile.StartSound = SlotMotionConfig.DrawnEnterSound;
			break;
		case EWacomFirstPersonCardSlotTransitionKind::RunHandEntered:
			Profile.StartSound = SlotMotionConfig.RunHandEnterSound;
			break;
		case EWacomFirstPersonCardSlotTransitionKind::Gained:
			Profile.StartSound = SlotMotionConfig.GainedEnterSound;
			break;
		case EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered:
			Profile.StartSound = SlotMotionConfig.HandAnchorEnterSound;
			break;
		default:
			break;
		}
		Profile.StartSoundVolumeMultiplier = SlotMotionConfig.EnterSoundVolumeMultiplier;
		Profile.StartSoundPitchMultiplier = SlotMotionConfig.EnterSoundPitchMultiplier;
	}

	if (TransitionHint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Drawn)
	{
		const FWacomFirstPersonCardPresentationAnchorPoint& DrawPileAnchor =
			PresentationAnchors.Get(EWacomFirstPersonCardPresentationAnchorKind::DrawPile);
		if (IsValidPresentationAnchorPoint(DrawPileAnchor))
		{
			Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
			Profile.OffsetPixels = DrawPileAnchor.WidgetPosition - TargetSlotView.ScreenPosition;
			return Profile;
		}
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
		Profile.DurationSeconds = SlotMotionConfig.ExitDuration;
		Profile.EasePower = SlotMotionConfig.ExitMotionProfile.EasePower;
		break;
	case EWacomFirstPersonCardSlotTransitionKind::Discarded:
	case EWacomFirstPersonCardSlotTransitionKind::Exhausted:
		Profile.OriginMode = SlotMotionConfig.DiscardedExitOriginMode;
		Profile.OffsetPixels = SlotMotionConfig.DiscardedExitOffsetPixels;
		Profile.ViewportAnchor = SlotMotionConfig.DiscardedExitViewportAnchor;
		Profile.ScaleMultiplier = SlotMotionConfig.DiscardedExitScaleMultiplier;
		Profile.AngleOffsetDegrees = SlotMotionConfig.DiscardedExitAngleOffsetDegrees;
		Profile.StartDelaySeconds =
			FMath::Max(0, TransitionHint.SequenceIndex) * SlotMotionConfig.DiscardedExitStaggerSeconds;
		Profile.DurationSeconds = SlotMotionConfig.ExitDuration;
		Profile.EasePower = SlotMotionConfig.ExitMotionProfile.EasePower;
		break;
	default:
		return TOptional<FWacomFirstPersonCardTransitionMotionProfile>();
	}

	if (TransitionHint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played)
	{
		if (TransitionHint.bHasPlayedExitTargetWidgetPosition
			&& FMath::IsFinite(TransitionHint.PlayedExitTargetWidgetPosition.X)
			&& FMath::IsFinite(TransitionHint.PlayedExitTargetWidgetPosition.Y))
		{
			Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
			Profile.OffsetPixels =
				TransitionHint.PlayedExitTargetWidgetPosition - VisualSlotView.ScreenPosition;
			return Profile;
		}
		const FWacomFirstPersonCardPresentationAnchorPoint& PlayTargetAnchor =
			PresentationAnchors.Get(EWacomFirstPersonCardPresentationAnchorKind::PlayTarget);
		if (IsValidPresentationAnchorPoint(PlayTargetAnchor))
		{
			Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
			Profile.OffsetPixels = PlayTargetAnchor.WidgetPosition - VisualSlotView.ScreenPosition;
			return Profile;
		}
	}
	else if (TransitionHint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Discarded
		|| TransitionHint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Exhausted)
	{
		const FWacomFirstPersonCardPresentationAnchorPoint& DiscardPileAnchor =
			PresentationAnchors.Get(EWacomFirstPersonCardPresentationAnchorKind::DiscardPile);
		if (IsValidPresentationAnchorPoint(DiscardPileAnchor))
		{
			Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
			Profile.OffsetPixels = DiscardPileAnchor.WidgetPosition - VisualSlotView.ScreenPosition;
			return Profile;
		}
	}

	if (!SlotMotionConfig.bEnableReadableTransitionOrigins)
	{
		Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Profile.ScaleMultiplier = 1.0f;
		Profile.AngleOffsetDegrees = 0.0f;
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

	return Profile;
}

void UWacomFirstPersonCardLayerWidget::HandleSlotHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!CardInstanceId.IsValid())
	{
		return;
	}

	HoveredCardInstanceId = CardInstanceId;
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget && SlotWidget->GetSlotView().Entry.CardInstanceId == CardInstanceId)
		{
			HoveredSlotWidget = SlotWidget.Get();
			break;
		}
	}
	OnCardHoveredNative.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (HoveredCardInstanceId == CardInstanceId)
	{
		HoveredCardInstanceId.Invalidate();
	}
	if (UWacomFirstPersonCardLayerSlotWidget* HoveredSlot = HoveredSlotWidget.Get())
	{
		if (HoveredSlot->GetSlotView().Entry.CardInstanceId == CardInstanceId)
		{
			HoveredSlotWidget.Reset();
		}
	}
	OnCardUnhoveredNative.Broadcast(CardInstanceId, SlotView);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotVisualSlotUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	OnHoveredCardSlotUpdatedNative.Broadcast(CardInstanceId, SlotView);
	if (HoveredCardTargetHandle.IsValid()
		&& HoveredCardTargetHandle.CardInstanceId == CardInstanceId)
	{
		HoveredCardTargetSlotView = SlotView;
		OnHoveredCardTargetUpdatedNative.Broadcast(HoveredCardTargetHandle, SlotView);
	}
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

bool UWacomFirstPersonCardLayerWidget::ResolveAbsoluteScreenPositionToWidgetPosition(
	const FVector2D& AbsoluteScreenPosition,
	FVector2D& OutWidgetPosition) const
{
	FVector2D PixelPosition = FVector2D::ZeroVector;
	FVector2D ViewportPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		AbsoluteScreenPosition,
		PixelPosition,
		ViewportPosition);

	if (ViewportPosition.ContainsNaN())
	{
		return false;
	}

	OutWidgetPosition = ViewportPosition;
	return true;
}

bool UWacomFirstPersonCardLayerWidget::BroadcastCardPointerMovedFromWidgetPosition(
	const FVector2D& WidgetPosition)
{
	FWacomFirstPersonCardLayerSlotView ResolvedSlotView;
	UWacomFirstPersonCardLayerSlotWidget* ResolvedSlot = ResolveInteractiveSlotUnderPointer(
		WidgetPosition,
		FGuid(),
		false,
		true,
		&ResolvedSlotView);
	if (!ResolvedSlot)
	{
		ClearCardPointerView(true);
		return false;
	}

	FVector2D PointerViewportPosition = FVector2D::ZeroVector;
	FVector2D PointerNormalizedViewportPosition = FVector2D::ZeroVector;
	if (!ResolvePointerViewportPosition(
		WidgetPosition,
		PointerViewportPosition,
		PointerNormalizedViewportPosition))
	{
		ClearCardPointerView(true);
		return false;
	}

	FWacomFirstPersonCardPointerView PointerView;
	PointerView.CardInstanceId = ResolvedSlotView.Entry.CardInstanceId;
	PointerView.SlotView = ResolvedSlot->GetVisualSlotView();
	PointerView.SlotView.bIsHovered = HoveredSlotWidget.Get() == ResolvedSlot;
	PointerView.bHasPointerViewportPosition = true;
	PointerView.PointerViewportPosition = PointerViewportPosition;
	PointerView.PointerNormalizedViewportPosition = PointerNormalizedViewportPosition;
	CurrentPointerView = PointerView;
	bHasCurrentPointerView = true;
	OnCardPointerMovedNative.Broadcast(CurrentPointerView);
	return true;
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::ResolveInteractiveSlotUnderPointer(
	const FVector2D& WidgetPosition,
	const FGuid& ExcludedCardInstanceId,
	bool bRequirePlayable,
	bool bUseHoverHysteresis,
	FWacomFirstPersonCardLayerSlotView* OutResolvedSlotView) const
{
	if (!bCardLayerInteractionEnabled || WidgetPosition.ContainsNaN())
	{
		return nullptr;
	}

	struct FHitCandidate
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = nullptr;
		FWacomFirstPersonCardLayerSlotView SlotView;
		int32 Order = INDEX_NONE;
		float CenterX = 0.0f;
		float DistanceSquared = 0.0f;
	};

	TArray<FHitCandidate> Candidates;
	Candidates.Reserve(SlotWidgets.Num());
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget
			|| SlotWidget->IsExitingForFirstPersonLayer()
			|| !SlotWidget->IsCardLayerInteractionEnabled())
		{
			continue;
		}

		const FWacomFirstPersonCardLayerSlotView& SlotView = SlotWidget->GetSlotView();
		const FGuid SlotCardId = SlotView.Entry.CardInstanceId;
		if (!SlotCardId.IsValid()
			|| SlotCardId == ExcludedCardInstanceId
			|| !SlotView.bProjected
			|| (bRequirePlayable && !SlotView.Entry.bIsPlayable))
		{
			continue;
		}

		const FVector2D BodySize = SlotWidget->GetCardBodyHitSizeForFirstPersonLayer();
		if (!IsWidgetPositionInsideStableCardBody(SlotView, BodySize, WidgetPosition))
		{
			continue;
		}

		FHitCandidate Candidate;
		Candidate.SlotWidget = SlotWidget.Get();
		Candidate.SlotView = SlotView;
		Candidate.Order = ResolveInputHitOrder(SlotView);
		Candidate.CenterX = ResolveInputHitCenter(SlotView).X;
		Candidate.DistanceSquared = (WidgetPosition - ResolveInputHitCenter(SlotView)).SizeSquared();
		Candidates.Add(Candidate);
	}

	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	Candidates.Sort([](const FHitCandidate& A, const FHitCandidate& B)
	{
		if (A.Order != B.Order)
		{
			return A.Order < B.Order;
		}
		return A.CenterX < B.CenterX;
	});

	if (bUseHoverHysteresis)
	{
		if (UWacomFirstPersonCardLayerSlotWidget* CurrentHoveredSlot = HoveredSlotWidget.Get())
		{
			const int32 CurrentIndex = Candidates.IndexOfByPredicate(
				[CurrentHoveredSlot](const FHitCandidate& Candidate)
				{
					return Candidate.SlotWidget == CurrentHoveredSlot;
				});
			if (CurrentIndex != INDEX_NONE)
			{
				const float HysteresisPixels = FMath::Max(0.0f, CardDragConfig.HoverHitHysteresisPixels);
				const float CurrentCenterX = Candidates[CurrentIndex].CenterX;
				const bool bMaySwitchLeft =
					CurrentIndex > 0
					&& WidgetPosition.X < ((Candidates[CurrentIndex - 1].CenterX + CurrentCenterX) * 0.5f) - HysteresisPixels;
				const bool bMaySwitchRight =
					CurrentIndex + 1 < Candidates.Num()
					&& WidgetPosition.X > ((Candidates[CurrentIndex + 1].CenterX + CurrentCenterX) * 0.5f) + HysteresisPixels;
				if (!bMaySwitchLeft && !bMaySwitchRight)
				{
					if (OutResolvedSlotView)
					{
						*OutResolvedSlotView = Candidates[CurrentIndex].SlotView;
					}
					return CurrentHoveredSlot;
				}
			}
		}
	}

	int32 BestIndex = 0;
	for (int32 Index = 1; Index < Candidates.Num(); ++Index)
	{
		const float PreviousBoundary = (Candidates[Index - 1].CenterX + Candidates[Index].CenterX) * 0.5f;
		if (WidgetPosition.X >= PreviousBoundary)
		{
			BestIndex = Index;
		}
	}

	if (OutResolvedSlotView)
	{
		*OutResolvedSlotView = Candidates[BestIndex].SlotView;
	}
	return Candidates[BestIndex].SlotWidget;
}

bool UWacomFirstPersonCardLayerWidget::TryResolveInspectScrubHandBounds(
	FVector2D& OutMin,
	FVector2D& OutMax) const
{
	if (!bCardLayerInteractionEnabled)
	{
		return false;
	}

	bool bHasBounds = false;
	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget
			|| SlotWidget->IsExitingForFirstPersonLayer()
			|| !SlotWidget->IsCardLayerInteractionEnabled())
		{
			continue;
		}

		const FWacomFirstPersonCardLayerSlotView& SlotView = SlotWidget->GetSlotView();
		if (!SlotView.Entry.CardInstanceId.IsValid()
			|| !SlotView.bProjected)
		{
			continue;
		}

		AccumulateStableCardBodyBounds(
			SlotView,
			SlotWidget->GetCardBodyHitSizeForFirstPersonLayer(),
			OutMin,
			OutMax,
			bHasBounds);
	}
	return bHasBounds;
}

bool UWacomFirstPersonCardLayerWidget::IsWidgetPositionInsideInspectScrubArea(
	const FVector2D& WidgetPosition) const
{
	if (WidgetPosition.ContainsNaN())
	{
		return false;
	}

	FVector2D BoundsMin = FVector2D::ZeroVector;
	FVector2D BoundsMax = FVector2D::ZeroVector;
	if (!TryResolveInspectScrubHandBounds(BoundsMin, BoundsMax))
	{
		return false;
	}

	const FVector2D ScrubPadding(
		FMath::Max(0.0f, CardDragConfig.CardInspectScrubHandPaddingPixels.X),
		FMath::Max(0.0f, CardDragConfig.CardInspectScrubHandPaddingPixels.Y));
	BoundsMin -= ScrubPadding;
	BoundsMax += ScrubPadding;
	return WidgetPosition.X >= BoundsMin.X
		&& WidgetPosition.X <= BoundsMax.X
		&& WidgetPosition.Y >= BoundsMin.Y
		&& WidgetPosition.Y <= BoundsMax.Y;
}

bool UWacomFirstPersonCardLayerWidget::TryRouteInspectScrubPointer(
	UWacomFirstPersonCardLayerSlotWidget& GestureSlot,
	const FVector2D& WidgetPosition)
{
	if (!GestureSlot.IsInspectScrubActiveForFirstPersonLayer()
		|| !IsWidgetPositionInsideInspectScrubArea(WidgetPosition))
	{
		return false;
	}

	FWacomFirstPersonCardLayerSlotView ResolvedSlotView;
	UWacomFirstPersonCardLayerSlotWidget* TargetSlot = ResolveInteractiveSlotUnderPointer(
		WidgetPosition,
		FGuid(),
		true,
		false,
		&ResolvedSlotView);
	if (TargetSlot
		&& TargetSlot != &GestureSlot
		&& TargetSlot->CanBeginInspectScrubFromFirstPersonLayer())
	{
		GestureSlot.ClearInspectScrubGestureFromFirstPersonLayer();
		if (TargetSlot->BeginInspectScrubFromFirstPersonLayer(WidgetPosition))
		{
			ClearCardPointerView(true);
			ClearHoveredSlotState(true);
			return true;
		}
	}

	GestureSlot.UpdateGestureFromFirstPersonLayer(0.0f, WidgetPosition, true);
	return true;
}

bool UWacomFirstPersonCardLayerWidget::UpdateHoveredSlotFromWidgetPosition(const FVector2D& WidgetPosition)
{
	if (ShouldSuppressOrdinaryHoverForDrag())
	{
		ClearHoveredSlotState(true);
		return false;
	}

	FWacomFirstPersonCardLayerSlotView ResolvedSlotView;
	UWacomFirstPersonCardLayerSlotWidget* NewHoveredSlot = ResolveInteractiveSlotUnderPointer(
		WidgetPosition,
		FGuid(),
		false,
		true,
		&ResolvedSlotView);
	UWacomFirstPersonCardLayerSlotWidget* PreviousHoveredSlot = HoveredSlotWidget.Get();
	if (NewHoveredSlot)
	{
		NewHoveredSlot->UpdatePointerViewportDiagnostics(WidgetPosition);
	}
	if (PreviousHoveredSlot == NewHoveredSlot)
	{
		return NewHoveredSlot != nullptr;
	}

	if (PreviousHoveredSlot)
	{
		FWacomFirstPersonCardLayerSlotView PreviousSlotView = PreviousHoveredSlot->GetVisualSlotView();
		PreviousSlotView.bIsHovered = false;
		const FGuid PreviousCardId = PreviousHoveredSlot->GetSlotView().Entry.CardInstanceId;
		PreviousHoveredSlot->SetHoveredFromFirstPersonLayer(false);
		if (PreviousCardId.IsValid())
		{
			OnCardUnhoveredNative.Broadcast(PreviousCardId, PreviousSlotView);
		}
		ClearHoveredCardTargetState(true);
	}

	HoveredSlotWidget = NewHoveredSlot;
	if (!NewHoveredSlot)
	{
		HoveredCardInstanceId.Invalidate();
		return false;
	}

	NewHoveredSlot->SetHoveredFromFirstPersonLayer(true);
	FWacomFirstPersonCardLayerSlotView HoveredVisualSlotView = NewHoveredSlot->GetVisualSlotView();
	HoveredVisualSlotView.bIsHovered = true;
	const FGuid NewCardId = NewHoveredSlot->GetSlotView().Entry.CardInstanceId;
	if (NewCardId.IsValid())
	{
		HoveredCardInstanceId = NewCardId;
		OnCardHoveredNative.Broadcast(NewCardId, HoveredVisualSlotView);
		const FWacomInteractionTargetHandle CardTargetHandle = NewHoveredSlot->BuildCardTargetHandle();
		if (CardTargetHandle.IsValid())
		{
			HoveredCardTargetHandle = CardTargetHandle;
			HoveredCardTargetSlotView = HoveredVisualSlotView;
			OnCardTargetHoveredNative.Broadcast(CardTargetHandle, HoveredVisualSlotView);
		}
	}
	return true;
}

FWacomFirstPersonCardPointerRouteResult
UWacomFirstPersonCardLayerWidget::RouteSlotPointerMovedAtWidgetPosition(
	const FVector2D& WidgetPosition)
{
	if (RoutePointerToActiveGestureSlot(WidgetPosition))
	{
		if (ShouldSuppressOrdinaryHoverForDrag())
		{
			ClearCardPointerView(true);
			ClearHoveredSlotState(true);
		}
		return FWacomFirstPersonCardPointerRouteResult::Handled();
	}

	const bool bPointerStillOverCard = BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
	const bool bHoverHandled = UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
	return bPointerStillOverCard || bHoverHandled
		? FWacomFirstPersonCardPointerRouteResult::Handled()
		: FWacomFirstPersonCardPointerRouteResult::Unhandled();
}

FWacomFirstPersonCardPointerRouteResult
UWacomFirstPersonCardLayerWidget::RouteSlotPointerPressedAtWidgetPosition(
	const FVector2D& WidgetPosition)
{
	if (RoutePointerPressToActiveGesture(WidgetPosition))
	{
		return FWacomFirstPersonCardPointerRouteResult::Handled();
	}

	FWacomFirstPersonCardLayerSlotView ResolvedSlotView;
	UWacomFirstPersonCardLayerSlotWidget* TargetSlot = ResolveInteractiveSlotUnderPointer(
		WidgetPosition,
		FGuid(),
		false,
		false,
		&ResolvedSlotView);
	if (!TargetSlot)
	{
		return FWacomFirstPersonCardPointerRouteResult::Unhandled();
	}

	UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
	if (!TargetSlot->BeginGesturePressFromFirstPersonLayer(WidgetPosition))
	{
		return FWacomFirstPersonCardPointerRouteResult::Unhandled();
	}

	PressedSlotWidget = TargetSlot;
	return FWacomFirstPersonCardPointerRouteResult::CaptureMouse();
}

FWacomFirstPersonCardPointerRouteResult
UWacomFirstPersonCardLayerWidget::RouteSlotPointerReleasedAtWidgetPosition(
	const FVector2D& WidgetPosition)
{
	UWacomFirstPersonCardLayerSlotWidget* GestureSlot = FindActiveGestureSlot();
	if (!GestureSlot)
	{
		FWacomFirstPersonCardLayerSlotView ResolvedSlotView;
		GestureSlot = ResolveInteractiveSlotUnderPointer(
			WidgetPosition,
			FGuid(),
			false,
			false,
			&ResolvedSlotView);
	}
	if (!GestureSlot)
	{
		return FWacomFirstPersonCardPointerRouteResult::Unhandled();
	}

	const bool bSuppressHoverAfterRelease = ShouldSuppressOrdinaryHoverForDrag();
	const bool bSuppressInspectDragPromotion =
		GestureSlot->IsInspectScrubActiveForFirstPersonLayer()
		&& IsWidgetPositionInsideInspectScrubArea(WidgetPosition);
	if (!GestureSlot->ReleaseGestureFromFirstPersonLayer(
		WidgetPosition,
		bSuppressInspectDragPromotion))
	{
		return FWacomFirstPersonCardPointerRouteResult::Unhandled();
	}

	PressedSlotWidget.Reset();
	if (bSuppressHoverAfterRelease)
	{
		ClearHoveredSlotState(true);
		BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
	}
	else
	{
		UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
	}
	return FWacomFirstPersonCardPointerRouteResult::ReleaseMouseCapture();
}

bool UWacomFirstPersonCardLayerWidget::RoutePointerToActiveGestureSlot(const FVector2D& WidgetPosition)
{
	if (UWacomFirstPersonCardLayerSlotWidget* GestureSlot = FindActiveGestureSlot())
	{
		if (TryRouteInspectScrubPointer(*GestureSlot, WidgetPosition))
		{
			return true;
		}

		if (!GestureSlot->CanUpdateGestureFromSlotPointer())
		{
			// External drags are positioned by the player-controller pointer pump.
			return true;
		}

		GestureSlot->UpdateGestureFromFirstPersonLayer(0.0f, WidgetPosition);
		return true;
	}
	return false;
}

bool UWacomFirstPersonCardLayerWidget::RoutePointerPressToActiveGesture(
	const FVector2D& WidgetPosition)
{
	UWacomFirstPersonCardLayerSlotWidget* GestureSlot = FindActiveGestureSlot();
	if (!GestureSlot)
	{
		return false;
	}

	const bool bSuppressInspectDragPromotion =
		GestureSlot->IsInspectScrubActiveForFirstPersonLayer()
		&& IsWidgetPositionInsideInspectScrubArea(WidgetPosition);
	GestureSlot->UpdateGestureFromFirstPersonLayer(
		0.0f,
		WidgetPosition,
		bSuppressInspectDragPromotion);
	if (ShouldSuppressOrdinaryHoverForDrag())
	{
		ClearCardPointerView(true);
		ClearHoveredSlotState(true);
	}
	return true;
}

bool UWacomFirstPersonCardLayerWidget::RouteExternalPointerToActiveGestureSlot(
	const FVector2D& WidgetPosition)
{
	if (UWacomFirstPersonCardLayerSlotWidget* GestureSlot = FindActiveGestureSlot())
	{
		if (!GestureSlot->CanUpdateGestureFromExternalPointer())
		{
			return false;
		}

		GestureSlot->UpdateGestureFromFirstPersonLayer(0.0f, WidgetPosition);
		return true;
	}
	return false;
}

UWacomFirstPersonCardLayerSlotWidget* UWacomFirstPersonCardLayerWidget::FindActiveGestureSlot() const
{
	if (UWacomFirstPersonCardLayerSlotWidget* PressedSlot = PressedSlotWidget.Get())
	{
		if (PressedSlot->GetGestureStateForFirstPersonLayer() != EWacomFirstPersonCardGestureState::Idle
			&& PressedSlot->GetGestureStateForFirstPersonLayer() != EWacomFirstPersonCardGestureState::Cancelled)
		{
			return PressedSlot;
		}
	}

	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget
			&& SlotWidget->GetGestureStateForFirstPersonLayer() != EWacomFirstPersonCardGestureState::Idle
			&& SlotWidget->GetGestureStateForFirstPersonLayer() != EWacomFirstPersonCardGestureState::Cancelled)
		{
			return SlotWidget.Get();
		}
	}
	return nullptr;
}

bool UWacomFirstPersonCardLayerWidget::HandleSlotPointerEntered(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& ScreenPosition)
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (!ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		WidgetPosition = SourceSlot.GetSlotView().ScreenPosition;
	}
	if (RoutePointerToActiveGestureSlot(WidgetPosition))
	{
		if (ShouldSuppressOrdinaryHoverForDrag())
		{
			ClearCardPointerView(true);
			ClearHoveredSlotState(true);
		}
		return true;
	}
	BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
	return UpdateHoveredSlotFromWidgetPosition(WidgetPosition);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotPointerLeft(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& ScreenPosition)
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		if (RoutePointerToActiveGestureSlot(WidgetPosition))
		{
			if (ShouldSuppressOrdinaryHoverForDrag())
			{
				ClearCardPointerView(true);
				ClearHoveredSlotState(true);
			}
			return;
		}
		const bool bPointerStillOverCard = BroadcastCardPointerMovedFromWidgetPosition(WidgetPosition);
		if (UpdateHoveredSlotFromWidgetPosition(WidgetPosition))
		{
			return;
		}
		if (!bPointerStillOverCard)
		{
			ClearCardPointerView(true);
		}
	}

	if (HoveredSlotWidget.Get() == &SourceSlot || !HoveredSlotWidget.IsValid())
	{
		ClearCardPointerView(true);
		ClearHoveredSlotState(true);
	}
}

FWacomFirstPersonCardPointerRouteResult UWacomFirstPersonCardLayerWidget::HandleSlotPointerMoved(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& ScreenPosition)
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (!ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		WidgetPosition = SourceSlot.GetSlotView().ScreenPosition;
	}
	return RouteSlotPointerMovedAtWidgetPosition(WidgetPosition);
}

FWacomFirstPersonCardPointerRouteResult UWacomFirstPersonCardLayerWidget::HandleSlotPointerPressed(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& ScreenPosition)
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (!ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		WidgetPosition = SourceSlot.GetSlotView().ScreenPosition;
	}
	return RouteSlotPointerPressedAtWidgetPosition(WidgetPosition);
}

FWacomFirstPersonCardPointerRouteResult UWacomFirstPersonCardLayerWidget::HandleSlotPointerReleased(
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& ScreenPosition)
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (!ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		WidgetPosition = SourceSlot.GetSlotView().ScreenPosition;
	}
	return RouteSlotPointerReleasedAtWidgetPosition(WidgetPosition);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	CurrentDragView = DragView;
	ClearCardPointerView(true);
	ClearHoveredSlotState(true);
	if (CurrentDragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		&& CurrentDragView.bCommitArmed)
	{
		CurrentDragView.TargetFeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::CommitReady;
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
		ApplyPointerCardTargetToDragView(
			CurrentDragView,
			PreviousDragView,
			PointerCardTargetHandle,
			PointerCardTargetSlotView);
	}
	else if (CurrentDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		CurrentDragView.CurrentTarget = FWacomInteractionTargetHandle();
		CurrentDragView.bTargetValid = false;
		if (IsCardTargetFeedbackState(CurrentDragView.TargetFeedbackState))
		{
			CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		}
		CurrentDragView.bHasFeedbackTargetScreenPosition = false;
		CurrentDragView.FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	if (CurrentDragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		&& CurrentDragView.bCommitArmed)
	{
		CurrentDragView.TargetFeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::CommitReady;
	}
	RefreshCardTargetFocusFromCurrentDragView();
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
		ApplyPointerCardTargetToDragView(
			CurrentDragView,
			PreviousDragView,
			PointerCardTargetHandle,
			PointerCardTargetSlotView);
	}
	else if (CurrentDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card)
	{
		CurrentDragView.CurrentTarget = FWacomInteractionTargetHandle();
		CurrentDragView.bTargetValid = false;
		if (IsCardTargetFeedbackState(CurrentDragView.TargetFeedbackState))
		{
			CurrentDragView.TargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		}
		CurrentDragView.bHasFeedbackTargetScreenPosition = false;
		CurrentDragView.FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	if (CurrentDragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		&& CurrentDragView.bCommitArmed)
	{
		CurrentDragView.TargetFeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::CommitReady;
	}
	OnCardDragReleasedNative.Broadcast(CardInstanceId, CurrentDragView);
	CurrentDragView = FWacomFirstPersonCardDragView();
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ClearCardDragTargetFeedback();
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
			SlotWidget->ClearCardDragTargetFeedback();
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerWidget::HandleSlotEnterTransitionStarted(
	const FWacomFirstPersonCardEnterTransitionStartedView& View)
{
	OnEnterTransitionStartedNative.Broadcast(View);
}

bool UWacomFirstPersonCardLayerWidget::TryResolveCardTargetUnderDragPointer(
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	FWacomFirstPersonCardLayerSlotView& OutTargetSlotView) const
{
	OutTargetHandle = FWacomInteractionTargetHandle();
	OutTargetSlotView = FWacomFirstPersonCardLayerSlotView();

	if (!DragView.CardInstanceId.IsValid())
	{
		return false;
	}

	const bool bCanProbeCardTarget =
		DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard;
	if (!bCanProbeCardTarget)
	{
		return false;
	}

	const FVector2D PointerPosition = DragView.CurrentScreenPosition;
	if (PointerPosition.ContainsNaN())
	{
		return false;
	}

	FWacomFirstPersonCardLayerSlotView BestSlotView;
	UWacomFirstPersonCardLayerSlotWidget* BestSlotWidget = ResolveInteractiveSlotUnderPointer(
		PointerPosition,
		DragView.CardInstanceId,
		false,
		false,
		&BestSlotView);
	if (!BestSlotWidget || !BestSlotWidget->CanExposeCardTarget())
	{
		return false;
	}

	OutTargetSlotView = BestSlotWidget->GetVisualSlotView();
	OutTargetHandle = BestSlotWidget->BuildCardTargetHandle();
	return OutTargetHandle.IsValid();
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

void UWacomFirstPersonCardLayerWidget::RefreshCardTargetFocusFromCurrentDragView()
{
	const bool bHasCardFocus =
		CurrentDragView.CurrentTarget.TargetKind == EWacomInteractionTargetKind::Card
		&& CurrentDragView.CurrentTarget.CardInstanceId.IsValid()
		&& IsCardTargetFeedbackState(CurrentDragView.TargetFeedbackState);
	for (TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}
		const bool bIsFocusedTarget =
			bHasCardFocus
			&& SlotWidget->GetSlotView().Entry.CardInstanceId
				== CurrentDragView.CurrentTarget.CardInstanceId;
		SlotWidget->SetCardDragTargetFocusFeedback(
			bIsFocusedTarget
				? CurrentDragView.TargetFeedbackState
				: EWacomFirstPersonCardDragTargetFeedbackState::None,
			bIsFocusedTarget && CurrentDragView.bTargetValid);
	}
}

FLinearColor UWacomFirstPersonCardLayerWidget::ResolveAimArrowColor() const
{
	return FLinearColor::White;
}

FVector2D UWacomFirstPersonCardLayerWidget::ResolveAimArrowStart() const
{
	if (!CurrentDragView.CardInstanceId.IsValid())
	{
		return CurrentDragView.SourceSlotView.ScreenPosition;
	}

	if (const UWacomFirstPersonCardLayerSlotWidget* ActiveGestureSlot = FindActiveGestureSlot())
	{
		if (ActiveGestureSlot->GetSlotView().Entry.CardInstanceId == CurrentDragView.CardInstanceId)
		{
			return ActiveGestureSlot->GetVisualSlotView().ScreenPosition;
		}
	}

	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : SlotWidgets)
	{
		if (SlotWidget
			&& SlotWidget->GetSlotView().Entry.CardInstanceId == CurrentDragView.CardInstanceId)
		{
			return SlotWidget->GetVisualSlotView().ScreenPosition;
		}
	}

	for (const TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>& SlotWidget : OutgoingSlotWidgets)
	{
		if (SlotWidget
			&& SlotWidget->GetSlotView().Entry.CardInstanceId == CurrentDragView.CardInstanceId)
		{
			return SlotWidget->GetVisualSlotView().ScreenPosition;
		}
	}

	return CurrentDragView.SourceSlotView.ScreenPosition;
}

FVector2D UWacomFirstPersonCardLayerWidget::ResolveAimArrowEnd() const
{
	return CurrentDragView.CurrentScreenPosition;
}

float UWacomFirstPersonCardLayerWidget::ResolveAimArrowPresentationScale() const
{
	if (const UWacomFirstPersonCardLayerSlotWidget* ActiveGestureSlot = FindActiveGestureSlot())
	{
		if (ActiveGestureSlot->GetSlotView().Entry.CardInstanceId == CurrentDragView.CardInstanceId)
		{
			return FMath::Clamp(
				ActiveGestureSlot->GetVisualSlotView().PresentationScale,
				0.5f,
				1.0f);
		}
	}

	return FMath::Clamp(CurrentDragView.SourceSlotView.PresentationScale, 0.5f, 1.0f);
}

