// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "WacomFirstPersonCardLayerWidget.generated.h"

class UCanvasPanel;
class UWacomCardView;
class UWacomFirstPersonCardViewWidget;
class UWacomFirstPersonCardLayerSlotWidget;
class UWacomFirstPersonCardPileTransferWidget;
struct FWacomFirstPersonCardLayerTestAccess;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerTargetNative, const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerDragNative, const FGuid&, const FWacomFirstPersonCardDragView&);
DECLARE_MULTICAST_DELEGATE_OneParam(FWacomFirstPersonCardLayerPointerNative, const FWacomFirstPersonCardPointerView&);
DECLARE_MULTICAST_DELEGATE(FWacomFirstPersonCardLayerPointerExitNative);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomFirstPersonCardLayerEnterTransitionStartedNative,
	const FWacomFirstPersonCardEnterTransitionStartedView&);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomFirstPersonCardLayerPileTransferProgressNative,
	const FWacomFirstPersonCardPileTransferProgressView&);

enum class EWacomFirstPersonCardPointerRouteAction : uint8
{
	Unhandled,
	Handled,
	CaptureMouse,
	ReleaseMouseCapture
};

struct FWacomFirstPersonCardPointerRouteResult
{
	EWacomFirstPersonCardPointerRouteAction Action =
		EWacomFirstPersonCardPointerRouteAction::Unhandled;

	constexpr FWacomFirstPersonCardPointerRouteResult() = default;

	explicit constexpr FWacomFirstPersonCardPointerRouteResult(
		EWacomFirstPersonCardPointerRouteAction InAction)
		: Action(InAction)
	{
	}

	bool IsHandled() const
	{
		return Action != EWacomFirstPersonCardPointerRouteAction::Unhandled;
	}

	static constexpr FWacomFirstPersonCardPointerRouteResult Unhandled()
	{
		return FWacomFirstPersonCardPointerRouteResult(
			EWacomFirstPersonCardPointerRouteAction::Unhandled);
	}

	static constexpr FWacomFirstPersonCardPointerRouteResult Handled()
	{
		return FWacomFirstPersonCardPointerRouteResult(
			EWacomFirstPersonCardPointerRouteAction::Handled);
	}

	static constexpr FWacomFirstPersonCardPointerRouteResult CaptureMouse()
	{
		return FWacomFirstPersonCardPointerRouteResult(
			EWacomFirstPersonCardPointerRouteAction::CaptureMouse);
	}

	static constexpr FWacomFirstPersonCardPointerRouteResult ReleaseMouseCapture()
	{
		return FWacomFirstPersonCardPointerRouteResult(
			EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture);
	}
};

struct FWacomFirstPersonCardLayerResolvedTransitionHint
{
	EWacomFirstPersonCardSlotTransitionKind TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	int32 SequenceIndex = 0;
	int32 SequenceCount = 1;
	bool bPlayCommitFeedback = false;
	bool bHasPlayedExitTargetWidgetPosition = false;
	FVector2D PlayedExitTargetWidgetPosition = FVector2D::ZeroVector;
};

struct FWacomFirstPersonCardLayerResolvedFeedbackHint
{
	EWacomFirstPersonCardLayerFeedbackKind FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::None;
	int32 SequenceIndex = 0;
	int32 SequenceCount = 1;
	int32 DataRewriteFieldMask = 0;
	EWacomFirstPersonCardDataRewriteTone DataRewriteTone =
		EWacomFirstPersonCardDataRewriteTone::Neutral;
	int32 DataRewriteSeed = 0;
	bool bHasDataRewriteCostValues = false;
	int32 DataRewriteCostBefore = 0;
	int32 DataRewriteCostAfter = 0;
	TArray<FWacomFirstPersonCardEffectBadgeChange> EffectBadgeChanges;
	bool bBlocksPresentationPhase = false;
	bool bRetainUntilExplicitRelease = false;
};

struct FWacomFirstPersonCardLayerResolvedFeedbackBundle
{
	TArray<FWacomFirstPersonCardLayerResolvedFeedbackHint> Hints;
};

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomFirstPersonCardLayerAutomationTestView
{
	int32 SkippedEquivalentSlotRefreshCount = 0;
	int32 SlotRuntimeConfigPropagationCount = 0;
	FWacomFirstPersonCardSlotRuntimeConfig SlotRuntimeConfig;
	FWacomFirstPersonCardDragView CurrentDragView;
	FWacomFirstPersonCardPointerView CurrentPointerView;
	bool bHasCurrentPointerView = false;
	FGuid HoveredCardInstanceId;
	FLinearColor AimArrowColor = FLinearColor::White;
	FVector2D AimArrowStart = FVector2D::ZeroVector;
	FVector2D AimArrowEnd = FVector2D::ZeroVector;
	float AimArrowLineThickness = 3.0f;
	float AimArrowHeadLength = 18.0f;
	float AimArrowHeadWidth = 9.0f;
	TSubclassOf<UWacomFirstPersonCardViewWidget> CardViewClass;
};
#endif

/**
 * HUD card layer driven by first-person card anchor projection.
 * Renders UWacomFirstPersonCardViewWidget-compatible card face widgets, may opt into slot-level
 * hover / press / drag intent, and never owns battle or Run commands.
 */
UCLASS()
class WACOMAPP_API UWacomFirstPersonCardLayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCardViewClass(TSubclassOf<UWacomFirstPersonCardViewWidget> InCardViewClass);
	void SetSlotRuntimeConfig(const FWacomFirstPersonCardSlotRuntimeConfig& InConfig);
	void SetCardDragFeedbackTarget(
		const FWacomInteractionTargetHandle& TargetHandle,
		bool bValidTarget,
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None,
		const TOptional<FVector2D>& FeedbackTargetScreenPosition = TOptional<FVector2D>(),
		const FString& ResolvedIntentDebugSummary = FString(),
		const TArray<FWacomFirstPersonCardTargetAffordance>& CardTargetAffordances =
			TArray<FWacomFirstPersonCardTargetAffordance>());
	void CancelCardDragGesture(bool bBroadcastCancel);
	void ClearSlotMotionState();
	bool TryStartCardDragGesture(const FGuid& CardInstanceId);
	bool TryStartCardDragGesture(
		const FGuid& CardInstanceId,
		const TOptional<FVector2D>& InitialPointerWidgetPosition);
	bool UpdateActiveDragPointerFromWidgetPosition(const FVector2D& WidgetPosition);
	bool ReleaseActiveDragGestureFromWidgetPosition(const FVector2D& WidgetPosition);
	bool ReleaseActiveDragGestureAtCurrentPointer();
	bool IsCardDragGestureActive() const;
	bool IsKeyboardShortcutCardDragGestureActive() const;
	void SetCardTransitionHints(const TArray<FWacomFirstPersonCardLayerTransitionHint>& InHints);
	void SetCardFeedbackHints(const TArray<FWacomFirstPersonCardLayerFeedbackHint>& InHints);
	void SetPresentationAnchors(const FWacomFirstPersonCardPresentationAnchorSet& InAnchors);
	void SetPileTransferConfig(const FWacomFirstPersonCardPileTransferConfig& InConfig);
	void SetPileTransferHints(const TArray<FWacomFirstPersonCardPileTransferHint>& InHints);
	void SetCardSlots(const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots);
	void SetCardLayerInteractionEnabled(bool bEnabled);
	void SetCardLayerPresentationVisible(bool bVisible);
	bool IsCardLayerPresentationVisible() const { return bCardLayerPresentationVisible; }
	bool HasActivePresentationPlayback() const;
	bool HasHandTargetImpactReachedPeak(const FGuid& CardInstanceId) const;
	void ForceSettlePresentationPlayback();

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetCardViewCount() const { return SlotWidgets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetOutgoingCardViewCount() const { return OutgoingSlotWidgets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetCardViewAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomFirstPersonCardViewWidget* GetFirstPersonCardViewAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomFirstPersonCardLayerSlotWidget* GetSlotWidgetAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardSlotVisible(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FWidgetTransform GetCardRenderTransformAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	float GetCardRenderOpacityAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetCardZOrderAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardLayerInteractionEnabled() const { return bCardLayerInteractionEnabled; }

	FWacomInteractionTargetHandle BuildHoveredCardTargetHandle() const { return HoveredCardTargetHandle; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "获取第一人称卡牌层 motion/reconcile 的只读调试快照；用于排查槽位复用、离场和 dirty gate。"))
	FWacomFirstPersonCardLayerMotionDebugView GetSlotMotionDebugView() const { return LastMotionDebugView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "获取第一人称卡牌层 motion/reconcile 的单行调试摘要；不改变卡牌层状态。"))
	FString GetSlotMotionDebugSummary() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "获取当前拖卡目标解析的单行调试摘要；用于排查 World/Card/Zone target，不提交命令。"))
	FString GetDragTargetDebugSummary() const;

	void SetLogSlotMotionDiagnostics(bool bEnabled) { bLogSlotMotionDiagnostics = bEnabled; }

	FWacomFirstPersonCardLayerInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerInteractionNative OnCardUnhoveredNative;
	FWacomFirstPersonCardLayerInteractionNative OnHoveredCardSlotUpdatedNative;
	FWacomFirstPersonCardLayerTargetNative OnCardTargetHoveredNative;
	FWacomFirstPersonCardLayerTargetNative OnCardTargetUnhoveredNative;
	FWacomFirstPersonCardLayerTargetNative OnHoveredCardTargetUpdatedNative;
	FWacomFirstPersonCardLayerDragNative OnCardDragStartedNative;
	FWacomFirstPersonCardLayerDragNative OnCardDragUpdatedNative;
	FWacomFirstPersonCardLayerDragNative OnCardDragReleasedNative;
	FWacomFirstPersonCardLayerDragNative OnCardDragCancelledNative;
	FWacomFirstPersonCardLayerPointerNative OnCardPointerMovedNative;
	FWacomFirstPersonCardLayerPointerExitNative OnCardPointerLeftNative;
	FWacomFirstPersonCardLayerEnterTransitionStartedNative OnEnterTransitionStartedNative;
	FWacomFirstPersonCardLayerPileTransferProgressNative OnPileTransferProgressNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardPileTransferWidget> PileTransferWidget;
	FWacomFirstPersonCardPresentationAnchorSet PresentationAnchors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>> SlotWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomFirstPersonCardLayerSlotWidget>> OutgoingSlotWidgets;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerSlotView> LastSlots;

	UPROPERTY(Transient)
	TSubclassOf<UWacomFirstPersonCardViewWidget> CardViewClass;

	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	FWacomFirstPersonCardSlotVisualConfig SlotVisualConfig;
	FWacomFirstPersonCardInteractionFeedbackConfig InteractionFeedbackConfig;
	FWacomFirstPersonCardDragPickupConfig DragPickupConfig;
	FWacomFirstPersonCardDragConfig CardDragConfig;
	FWacomFirstPersonCardPileTransferConfig PileTransferConfig;
	FWacomFirstPersonCardLayerMotionDebugView LastMotionDebugView;
	FGuid HoveredCardInstanceId;
	FWacomInteractionTargetHandle HoveredCardTargetHandle;
	FWacomFirstPersonCardLayerSlotView HoveredCardTargetSlotView;
	TWeakObjectPtr<UWacomFirstPersonCardLayerSlotWidget> HoveredSlotWidget;
	TWeakObjectPtr<UWacomFirstPersonCardLayerSlotWidget> PressedSlotWidget;
	FWacomFirstPersonCardDragView CurrentDragView;
	FWacomFirstPersonCardPointerView CurrentPointerView;
	bool bHasCurrentPointerView = false;
	FString CurrentDragResolvedIntentDebugSummary;
	TMap<FString, FWacomFirstPersonCardLayerResolvedTransitionHint> PendingTransitionHintsByKey;
	TMap<FString, FWacomFirstPersonCardLayerResolvedFeedbackBundle> PendingFeedbackHintsByKey;
	TSet<uint64> PlayedPileTransferKeys;
	TArray<FWacomFirstPersonCardPileTransferHint> DeferredPileTransferHints;
	bool bCardLayerInteractionEnabled = false;
	bool bCardLayerPresentationVisible = true;
	bool bLogSlotMotionDiagnostics = false;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardLayerAutomationTestView GetAutomationTestViewForTest() const;
	void TickSlotMotionForTest(float DeltaTime);
	UWacomFirstPersonCardLayerSlotWidget* FindSlotWidgetByKeyForTest(const FString& SlotKey) const;
	UWacomFirstPersonCardLayerSlotWidget* GetOutgoingSlotWidgetAtForTest(int32 Index) const;
	void AddUntrackedSlotChildForTest();
	void SetViewportSizeOverrideForTest(const FVector2D& WidgetViewportSize);
	FGuid ResolveHoveredCardAtWidgetPositionForTest(const FVector2D& WidgetPosition);
	bool HandleSlotPointerEnteredAtWidgetPositionForTest(
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& WidgetPosition);
	bool HandleSlotPointerMovedAtWidgetPositionForTest(
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& WidgetPosition);
	EWacomFirstPersonCardPointerRouteAction HandleSlotPointerMovedRouteActionAtWidgetPositionForTest(
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& WidgetPosition);
	bool RequestPressAtWidgetPositionForTest(const FVector2D& WidgetPosition);
	EWacomFirstPersonCardPointerRouteAction RequestPressRouteActionAtWidgetPositionForTest(
		const FVector2D& WidgetPosition);
	bool RequestReleaseAtWidgetPositionForTest(const FVector2D& WidgetPosition);
	EWacomFirstPersonCardPointerRouteAction RequestReleaseRouteActionAtWidgetPositionForTest(
		const FVector2D& WidgetPosition);

	TOptional<FVector2D> WidgetViewportSizeOverrideForTest;
	int32 SkippedEquivalentSlotRefreshCountForTest = 0;
	int32 SlotRuntimeConfigPropagationCountForTest = 0;
#endif

	friend class UWacomFirstPersonCardLayerSlotWidget;
	friend struct FWacomFirstPersonCardLayerTestAccess;

	UWacomFirstPersonCardLayerSlotWidget* CreateSlotWidget();
	FWacomFirstPersonCardSlotRuntimeConfig BuildCurrentSlotRuntimeConfig() const;
	void ApplyLayerVisibility();
	void EnsurePileTransferWidget();
	void ProcessDeferredPileTransferHints();
	void HandlePileTransferProgress(const FWacomFirstPersonCardPileTransferProgressView& Progress);
	void ReleaseOwnedSlateMouseCapture();
	void BindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	void UnbindSlotWidget(UWacomFirstPersonCardLayerSlotWidget* SlotWidget);
	void ClearHoveredCardTargetState(bool bBroadcastUnhover);
	void ClearCurrentDragState(bool bBroadcastCancel);
	bool ShouldSuppressOrdinaryHoverForDrag() const;
	int32 RemoveOutgoingFinishedSlots();
	int32 RemoveUntrackedSlotChildren();
	void EnforceOutgoingSlotLimit();
	void RepairSlotMotionInvariants();
	void RefreshSlotMotionDebugCounts();
	int32 CountRootCanvasSlotChildren() const;
	int32 CountMotionTickSlots() const;
	void ReportSlotMotionDiagnosticsIfNeeded();
	FString MakeSlotMotionKey(const FWacomFirstPersonCardLayerSlotView& SlotView) const;
	bool CanSkipEquivalentSlotRefresh(const TArray<FWacomFirstPersonCardLayerSlotView>& InSlots) const;
	bool AreSlotViewsEquivalentForRefresh(
		const FWacomFirstPersonCardLayerSlotView& A,
		const FWacomFirstPersonCardLayerSlotView& B) const;
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> GetEnterProfileForTransition(
		const FWacomFirstPersonCardLayerResolvedTransitionHint& TransitionHint,
		const FWacomFirstPersonCardLayerSlotView& TargetSlotView) const;
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> GetExitProfileForTransition(
		const FWacomFirstPersonCardLayerResolvedTransitionHint& TransitionHint,
		const FWacomFirstPersonCardLayerSlotView& VisualSlotView) const;
	bool ResolveViewportAnchorPosition(
		const FVector2D& NormalizedViewportAnchor,
		FVector2D& OutWidgetPosition) const;
	bool ResolvePointerViewportPosition(
		const FVector2D& WidgetPosition,
		FVector2D& OutPointerViewportPosition,
		FVector2D& OutPointerNormalizedViewportPosition) const;
	void HandleSlotHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotVisualSlotUpdated(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotCardTargetHovered(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotCardTargetUnhovered(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleSlotDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleSlotDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleSlotDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleSlotDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleSlotEnterTransitionStarted(
		const FWacomFirstPersonCardEnterTransitionStartedView& View);
	bool RoutePointerPressToActiveGesture(const FVector2D& WidgetPosition);
	bool HandleSlotPointerEntered(UWacomFirstPersonCardLayerSlotWidget& SourceSlot, const FVector2D& ScreenPosition);
	void HandleSlotPointerLeft(UWacomFirstPersonCardLayerSlotWidget& SourceSlot, const FVector2D& ScreenPosition);
	FWacomFirstPersonCardPointerRouteResult HandleSlotPointerMoved(
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& ScreenPosition);
	FWacomFirstPersonCardPointerRouteResult HandleSlotPointerPressed(
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& ScreenPosition);
	FWacomFirstPersonCardPointerRouteResult HandleSlotPointerReleased(
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& ScreenPosition);
	bool ResolveAbsoluteScreenPositionToWidgetPosition(const FVector2D& AbsoluteScreenPosition, FVector2D& OutWidgetPosition) const;
	bool BroadcastCardPointerMovedFromWidgetPosition(const FVector2D& WidgetPosition);
	void ClearCardPointerView(bool bBroadcastPointerLeft);
	UWacomFirstPersonCardLayerSlotWidget* ResolveInteractiveSlotUnderPointer(
		const FVector2D& WidgetPosition,
		const FGuid& ExcludedCardInstanceId,
		bool bRequirePlayable,
		bool bUseHoverHysteresis,
		FWacomFirstPersonCardLayerSlotView* OutResolvedSlotView = nullptr) const;
	bool TryResolveInspectScrubHandBounds(FVector2D& OutMin, FVector2D& OutMax) const;
	bool IsWidgetPositionInsideInspectScrubArea(const FVector2D& WidgetPosition) const;
	bool TryRouteInspectScrubPointer(
		UWacomFirstPersonCardLayerSlotWidget& GestureSlot,
		const FVector2D& WidgetPosition);
	bool UpdateHoveredSlotFromWidgetPosition(const FVector2D& WidgetPosition);
	FWacomFirstPersonCardPointerRouteResult RouteSlotPointerMovedAtWidgetPosition(
		const FVector2D& WidgetPosition);
	FWacomFirstPersonCardPointerRouteResult RouteSlotPointerPressedAtWidgetPosition(
		const FVector2D& WidgetPosition);
	FWacomFirstPersonCardPointerRouteResult RouteSlotPointerReleasedAtWidgetPosition(
		const FVector2D& WidgetPosition);
	bool RoutePointerToActiveGestureSlot(const FVector2D& WidgetPosition);
	bool RouteExternalPointerToActiveGestureSlot(const FVector2D& WidgetPosition);
	void ClearHoveredSlotState(bool bBroadcastUnhover);
	UWacomFirstPersonCardLayerSlotWidget* FindActiveGestureSlot() const;
	bool TryResolveCardTargetUnderDragPointer(
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		FWacomFirstPersonCardLayerSlotView& OutTargetSlotView) const;
	void ApplyDragFeedbackToCurrentDragView(
		const FWacomInteractionTargetHandle& TargetHandle,
		bool bValidTarget,
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
		const TOptional<FVector2D>& FeedbackTargetScreenPosition);
	void RefreshCardTargetFocusFromCurrentDragView();
	FLinearColor ResolveAimArrowColor() const;
	FVector2D ResolveAimArrowStart() const;
	FVector2D ResolveAimArrowEnd() const;
	float ResolveAimArrowPresentationScale() const;
};
