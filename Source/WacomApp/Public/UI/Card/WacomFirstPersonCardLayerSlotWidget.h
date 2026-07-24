// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardLayerSlotWidget.generated.h"

class UOverlay;
class UWacomCardView;
class UWacomFirstPersonCardViewWidget;
class UWacomFirstPersonCardLayerWidget;
class FWacomFirstPersonCardDepthMotion;
class FWacomFirstPersonCardGestureController;
struct FWacomFirstPersonCardGestureControllerState;
class FWacomFirstPersonCardDragPickupPlayback;
class FWacomFirstPersonCardInteractionFeedbackPlayback;
class FWacomFirstPersonCardSlotPresentationController;
struct FWacomFirstPersonCardLayerResolvedFeedbackHint;
struct FWacomFirstPersonCardLayerTestAccess;

struct FWacomFirstPersonCardDepthMotionDeleter
{
	void operator()(FWacomFirstPersonCardDepthMotion* Motion) const;
};

struct FWacomFirstPersonCardDragPickupPlaybackDeleter
{
	void operator()(FWacomFirstPersonCardDragPickupPlayback* Playback) const;
};

struct FWacomFirstPersonCardInteractionFeedbackPlaybackDeleter
{
	void operator()(FWacomFirstPersonCardInteractionFeedbackPlayback* Playback) const;
};

struct FWacomFirstPersonCardGestureControllerDeleter
{
	void operator()(FWacomFirstPersonCardGestureController* Controller) const;
};

struct FWacomFirstPersonCardSlotPresentationControllerDeleter
{
	void operator()(FWacomFirstPersonCardSlotPresentationController* Controller) const;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotTargetNative, const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotDragNative, const FGuid&, const FWacomFirstPersonCardDragView&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FWacomFirstPersonCardLayerSlotFaceInspectionNative,
	const FGuid&,
	EWacomCardFaceContext,
	const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomFirstPersonCardLayerSlotEnterTransitionStartedNative,
	const FWacomFirstPersonCardEnterTransitionStartedView&);

enum class EWacomFirstPersonCardGestureInputSource : uint8
{
	None,
	MousePointer,
	ExternalPointer
};

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomFirstPersonCardSlotAutomationTestView
{
	float InteractionCueAmount = 0.0f;
	EWacomFirstPersonCardInteractionCueKind InteractionCueKind =
		EWacomFirstPersonCardInteractionCueKind::None;
	FLinearColor InteractionCueColor = FLinearColor::Transparent;
	float PressedFeedbackAmount = 0.0f;
	EWacomFirstPersonCardGestureSource GestureSource = EWacomFirstPersonCardGestureSource::None;
	bool bPressed = false;
	bool bDenyFeedbackActive = false;
	bool bInvalidTargetPreviewActive = false;
	float InvalidTargetPreviewAmount = 0.0f;
	float DenyProgress = 0.0f;
	FVector2D DenyDirection = FVector2D(0.0f, -1.0f);
	bool bCommitFeedbackActive = false;
	bool bRetainedFeedbackActive = false;
	bool bRetainedFeedbackHeld = false;
	bool bRetainedFeedbackBlocking = false;
	EWacomFirstPersonCardRetainSealPhase RetainSealPhase =
		EWacomFirstPersonCardRetainSealPhase::Inactive;
	float RetainSealProgress = 0.0f;
	float RetainSealLiftPixels = 0.0f;
	float RetainSealScaleMultiplier = 1.0f;
	float RetainedFeedbackElapsedSeconds = 0.0f;
	float RetainedFeedbackStartDelaySeconds = 0.0f;
	bool bCardDragProbeFeedback = false;
	bool bCardDragTargetAffordanceFeedback = false;
	bool bCardDragTargetFocusActive = false;
	EWacomFirstPersonCardDragTargetFeedbackState CardDragTargetAffordanceFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardDragTargetFeedbackState CardDragTargetFocusFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardDragTargetFeedbackState DirectDragTargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardDragTargetFeedbackState DragTargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardMotionIntent ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	FWacomFirstPersonCardDepthView CardDepthView;
	FWacomFirstPersonCardSelectionView SelectionView;
	FWacomFirstPersonCardUseEffectView CardUseEffectView;
	FWacomFirstPersonCardPlayedDissolveView PlayedDissolveView;
	FWacomFirstPersonCardHandTargetImpactView HandTargetImpactView;
	FWacomFirstPersonCardDataRewriteView DataRewriteView;
	FWacomFirstPersonCardEffectBadgeFeedbackView EffectBadgeFeedbackView;
	FWacomFirstPersonCardDrawRevealView DrawRevealView;
	FWacomFirstPersonCardGainRevealView GainRevealView;
	FWidgetTransform RenderTransform;
	int32 RenderZOrder = 0;
	bool bDragPickupFeedbackActive = false;
	float DragPickupAlpha = 0.0f;
	int32 DragPickupTriggerCount = 0;
	int32 DragPickupSoundRequestCount = 0;
	float LastDragPickupSoundPitchMultiplier = 1.0f;
	int32 DenySoundRequestCount = 0;
	float LastDenySoundPitchMultiplier = 1.0f;
	bool bPlayedDissolvePlaybackActive = false;
	bool bCardUseEffectPlaybackActive = false;
	bool bCardUseReformPlaybackActive = false;
	bool bCardUseReformUsingTargetSlot = false;
	bool bHandTargetImpactCommitActive = false;
	bool bDataRewritePlaybackActive = false;
	bool bDataRewritePendingHandoff = false;
	bool bEffectBadgeFeedbackPlaybackActive = false;
	bool bDrawRevealPlaybackActive = false;
	bool bDrawRevealWaiting = false;
	float DrawRevealProgress = 0.0f;
	float DrawRevealHorizontalScale = 1.0f;
	FVector2D DrawRevealLandingScale = FVector2D(1.0f, 1.0f);
	float DrawRevealLandingTranslationYPixels = 0.0f;
	bool bGainRevealPlaybackActive = false;
	bool bGainRevealWaiting = false;
	float GainRevealProgress = 0.0f;
	bool bHandTargetDeparturePending = false;
	bool bHandTargetDepartureGateOpen = false;
	int32 HandTargetImpactZOrderBoost = 0;
	int32 CardUseEffectSoundRequestCount = 0;
	int32 CardUseReformSoundRequestCount = 0;
	float LastCardUseEffectSoundPitchMultiplier = 1.0f;
	float CardUseFlipProgress = 0.0f;
	float CardUseImpactProgress = 0.0f;
	float CardUseHorizontalScaleMultiplier = 1.0f;
	float CardUseOpacityMultiplier = 1.0f;
	int32 PlayedDissolveSoundRequestCount = 0;
	float LastPlayedDissolveSoundPitchMultiplier = 1.0f;
	bool bEnterTransitionPlaybackActive = false;
	bool bEnterTransitionBlocksInteraction = false;
	float EnterTransitionElapsedSeconds = 0.0f;
	float EnterTransitionStartDelaySeconds = 0.0f;
	float EnterTransitionDurationSeconds = 0.0f;
	bool bExitTransitionPlaybackActive = false;
	float ExitTransitionElapsedSeconds = 0.0f;
	float ExitTransitionStartDelaySeconds = 0.0f;
	float ExitTransitionDurationSeconds = 0.0f;
	int32 EnterTransitionSoundRequestCount = 0;
	int32 OptionalEnterSoundSkipCount = 0;
	EWacomFirstPersonCardSlotTransitionKind LastEnterTransitionSoundKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	FWacomFirstPersonCardSlotRuntimeConfig SlotRuntimeConfig;
	int32 SlotRuntimeConfigApplyCount = 0;
	int32 SurfaceReadinessState = 0;
	int32 CostDigitReadinessState = 0;
	int32 EffectBadgeReadinessState = 0;
	uint32 SurfaceReadinessGeneration = 0;
	uint32 CostDigitReadinessGeneration = 0;
	uint32 EffectBadgeReadinessGeneration = 0;
	bool bPlaybackFrozenForReadiness = false;
	int32 PresentationReadinessTimeoutCount = 0;
	int32 PresentationReadinessFallbackCount = 0;
};
#endif

/**
 * Single visual card slot inside the first-person card layer.
 *
 * This widget owns pointer interaction only. It does not submit battle commands;
 * callers decide what to do with the forwarded card instance id.
 */
UCLASS()
class WACOMAPP_API UWacomFirstPersonCardLayerSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomFirstPersonCardLayerSlotWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UWacomFirstPersonCardLayerSlotWidget() override;

	void SetCardViewClass(TSubclassOf<UWacomFirstPersonCardViewWidget> InCardViewClass);
	void SetSlotView(const FWacomFirstPersonCardLayerSlotView& InSlotView);
	void SetSlotViewImmediate(const FWacomFirstPersonCardLayerSlotView& InSlotView);
	void BeginSlotMotion(const FWacomFirstPersonCardLayerSlotView& InTargetSlotView, bool bTreatAsNewSlot);
	void BeginSlotMotionWithEnterOffset(
		const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
		bool bTreatAsNewSlot,
		const TOptional<FVector2D>& EnterOffsetOverride);
	void BeginSlotMotionWithEnterProfile(
		const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
		bool bTreatAsNewSlot,
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& EnterProfileOverride);
	void BeginExitMotion(const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView);
	void BeginExitMotionWithOffset(
		const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
		const TOptional<FVector2D>& ExitOffsetOverride);
	void BeginExitMotionWithProfile(
		const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind =
			EWacomFirstPersonCardSlotTransitionKind::Default);
	void TriggerCommitFeedback();
	void TriggerRetainedFeedback(
		int32 SequenceIndex,
		int32 SequenceCount,
		bool bRetainUntilExplicitRelease = false);
	void TriggerRetainedReleaseFeedback();
	void TriggerCardUseReformFeedback();
	void TriggerCardUseReformOutFeedback();
	void TriggerCardUseReformInFeedback();
	void TriggerHandTargetImpactFeedback();
	/** Locks the authoritative one-digit before/after values before incoming ViewData is applied. */
	bool PrepareCardDataRewriteForSlotView(
		const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
		const FWacomFirstPersonCardLayerResolvedFeedbackHint& RewriteHint);
	void TriggerCardDataRewriteFeedback(
		int32 FieldMask,
		EWacomFirstPersonCardDataRewriteTone Tone,
		int32 Seed,
		int32 SequenceIndex,
		int32 SequenceCount,
		bool bBlocksPresentationPhase = false);
	void TriggerEffectBadgeFeedback(
		const TArray<FWacomFirstPersonCardEffectBadgeChange>& Changes,
		bool bBlocksPresentationPhase = false);
	void BeginDeferredExitWithHandTargetImpact(
		const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind);
	bool IsHandTargetImpactDeparturePending() const;
	bool IsHandTargetImpactDepartureGateOpen() const;
	bool HasHandTargetImpactReachedPeak() const { return IsHandTargetImpactDepartureGateOpen(); }
	void SetHandTargetImpactDepartureOwnedByPileTransfer(bool bOwned);
	void ReleaseDeferredHandTargetExitNow();
	bool HasActivePresentationPlayback() const;
	void ForceCompletePresentationPlayback();
	void SetSlotRuntimeConfig(const FWacomFirstPersonCardSlotRuntimeConfig& InConfig);
	void SetCardDragFeedbackTarget(
		const FWacomInteractionTargetHandle& TargetHandle,
		bool bValidTarget,
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None,
		const TOptional<FVector2D>& FeedbackTargetScreenPosition = TOptional<FVector2D>());
	void SetCardDragProbeFeedback(bool bEnabled, bool bValidTarget = false);
	void SetCardDragTargetAffordanceFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None,
		bool bValidTarget = false);
	void SetCardDragTargetFocusFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None,
		bool bValidTarget = false);
	void ClearCardDragTargetFeedback();
	void CancelCardDragGesture(bool bBroadcastCancel);
	void SetCardLayerInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomFirstPersonCardViewWidget* GetCardView() const { return CardView; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetInnerCardView() const;

	const FWacomFirstPersonCardLayerSlotView& GetSlotView() const { return CurrentSlotView; }
	const FWacomFirstPersonCardLayerSlotView& GetVisualSlotView() const { return VisualSlotView; }
	const FString& GetSlotMotionKey() const { return SlotMotionKey; }
	void SetSlotMotionKey(const FString& InKey) { SlotMotionKey = InKey; }
	void SetOwningFirstPersonCardLayer(UWacomFirstPersonCardLayerWidget* InLayer);

	bool IsExitingForFirstPersonLayer() const { return bIsExitingForFirstPersonLayer; }
	bool IsExitMotionFinished() const;
	bool WantsSlotMotionTick() const { return bWantsSlotMotionTick; }
	bool CanExposeCardTarget() const;
	FWacomInteractionTargetHandle BuildCardTargetHandle() const;
	FVector2D GetCardBodyHitSizeForFirstPersonLayer() const;
	bool IsWidgetPositionInsideCardBodyForFirstPersonLayer(const FVector2D& WidgetPosition) const;
	EWacomFirstPersonCardGestureState GetGestureStateForFirstPersonLayer() const;
	bool CanUpdateGestureFromSlotPointer() const;
	bool CanUpdateGestureFromExternalPointer() const;
	bool IsInspectScrubActiveForFirstPersonLayer() const;
	bool CanBeginInspectScrubFromFirstPersonLayer() const;
	EWacomFirstPersonCardDragTargetFeedbackState GetDragTargetFeedbackStateForFirstPersonLayer() const
	{
		return ResolveEffectiveDragTargetFeedbackState();
	}
	EWacomFirstPersonCardDragTargetFeedbackState GetCardDragTargetAffordanceFeedbackStateForFirstPersonLayer() const
	{
		return CardDragTargetAffordanceFeedbackState;
	}
	FWacomFirstPersonCardDragView BuildDragView() const;
	void SetHoveredFromFirstPersonLayer(bool bHovered);
	bool BeginGesturePressFromFirstPersonLayer(const FVector2D& WidgetPosition);
	bool BeginDragGestureFromFirstPersonLayer(const FVector2D& WidgetPosition);
	bool BeginDragGestureFromFirstPersonLayer(
		const FVector2D& GestureOriginPosition,
		const FVector2D& InitialPointerPosition);
	bool BeginInspectScrubFromFirstPersonLayer(const FVector2D& WidgetPosition);
	void UpdateGestureFromFirstPersonLayer(
		float DeltaTime,
		const FVector2D& WidgetPosition,
		bool bSuppressInspectDragPromotion = false);
	bool ReleaseGestureFromFirstPersonLayer(
		const FVector2D& WidgetPosition,
		bool bSuppressInspectDragPromotion = false);
	void ClearInspectScrubGestureFromFirstPersonLayer();
	bool IsLockedFaceInspectionActive() const;
	EWacomCardFaceContext GetInspectedFaceContext() const { return InspectedFaceContext; }
	bool TryToggleLockedFaceInspection();
	bool CloseLockedFaceInspection();

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsHoveredForFirstPersonLayer() const { return bIsHoveredForFirstPersonLayer; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardLayerInteractionEnabled() const { return bCardLayerInteractionEnabled; }

	FWacomFirstPersonCardLayerSlotInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardUnhoveredNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardVisualSlotUpdatedNative;
	FWacomFirstPersonCardLayerSlotTargetNative OnCardTargetHoveredNative;
	FWacomFirstPersonCardLayerSlotTargetNative OnCardTargetUnhoveredNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragStartedNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragUpdatedNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragReleasedNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragCancelledNative;
	FWacomFirstPersonCardLayerSlotFaceInspectionNative OnCardFaceInspectLockedNative;
	FWacomFirstPersonCardLayerSlotFaceInspectionNative OnCardFaceChangedNative;
	FWacomFirstPersonCardLayerSlotFaceInspectionNative OnCardFaceInspectClosedNative;
	FWacomFirstPersonCardLayerSlotEnterTransitionStartedNative OnEnterTransitionStartedNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardViewWidget> CardView;

	UPROPERTY(Transient)
	TSubclassOf<UWacomFirstPersonCardViewWidget> CardViewClass;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView CurrentSlotView;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView TargetSlotView;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView VisualSlotView;

	TWeakObjectPtr<UWacomFirstPersonCardLayerWidget> OwningFirstPersonCardLayer;
	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	FWacomFirstPersonCardSlotVisualConfig SlotVisualConfig;
	FWacomFirstPersonCardInteractionFeedbackConfig InteractionFeedbackConfig;
	FWacomFirstPersonCardDragPickupConfig DragPickupConfig;
	FWacomFirstPersonCardDragConfig CardDragConfig;
	FString SlotMotionKey;
	EWacomFirstPersonCardMotionIntent ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	float ExitMotionElapsedSeconds = 0.0f;
	TUniquePtr<
		FWacomFirstPersonCardGestureController,
		FWacomFirstPersonCardGestureControllerDeleter> GestureController;
	TUniquePtr<
		FWacomFirstPersonCardDepthMotion,
		FWacomFirstPersonCardDepthMotionDeleter> CardDepthMotion;
	TUniquePtr<
		FWacomFirstPersonCardDragPickupPlayback,
		FWacomFirstPersonCardDragPickupPlaybackDeleter> DragPickupPlayback;
	TUniquePtr<
		FWacomFirstPersonCardInteractionFeedbackPlayback,
		FWacomFirstPersonCardInteractionFeedbackPlaybackDeleter> InteractionFeedbackPlayback;
	TUniquePtr<
		FWacomFirstPersonCardSlotPresentationController,
		FWacomFirstPersonCardSlotPresentationControllerDeleter> PresentationController;
	float HandTargetImpactScaleMultiplier = 1.0f;
	float HandTargetImpactTranslationYPixels = 0.0f;
	int32 HandTargetImpactZOrderBoost = 0;
	FWacomFirstPersonCardLayerSlotView CardUseReformStartSlotView;
	float CardUseFlipProgress = 0.0f;
	float CardUseImpactProgress = 0.0f;
	float CardUseMotionAlpha = 0.0f;
	float CardUseOpacityMultiplier = 1.0f;
	EWacomCardFaceContext InspectedFaceContext = EWacomCardFaceContext::Battle;
	EWacomCardFaceContext PendingFaceContext = EWacomCardFaceContext::Battle;
	float FaceFlipElapsedSeconds = 0.0f;
	bool bFaceFlipActive = false;
	bool bFaceFlipDataSwapped = false;
	bool bCardLayerInteractionEnabled = false;
	bool bIsHoveredForFirstPersonLayer = false;
	bool bHasVisualSlotView = false;
	bool bIsExitingForFirstPersonLayer = false;
	bool bUsesFixedExitTransitionPlayback = false;
	bool bWantsSlotMotionTick = false;
	bool bHasPointerViewportPosition = false;
	bool bCardDepthPointerDirty = false;
	bool bHasFeedbackTargetScreenPosition = false;
	bool bCardDragProbeFeedback = false;
	bool bCardDragProbeFeedbackValid = false;
	bool bCardDragTargetAffordanceFeedback = false;
	bool bCardDragTargetAffordanceFeedbackValid = false;
	EWacomFirstPersonCardDragTargetFeedbackState CardDragTargetAffordanceFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardDragTargetFeedbackState CardDragTargetFocusFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardDragTargetFeedbackState DirectDragTargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	EWacomFirstPersonCardDragTargetFeedbackState DragTargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	FVector2D PointerViewportPosition = FVector2D::ZeroVector;
	FVector2D PointerNormalizedViewportPosition = FVector2D::ZeroVector;
	FVector2D FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	TSet<FSoftObjectPath> WarnedUnavailableEnterSoundPaths;
#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardSlotAutomationTestView GetAutomationTestViewForTest() const;
	bool RequestHoverForTest();
	void RequestUnhoverForTest();
	bool RequestPressForTest();
	bool RequestMouseUpForTest();
	void TickSlotMotionForTest(float DeltaTime);
	void SetLocalHitCanvasSizeOverrideForTest(const TOptional<FVector2D>& InSize);
	bool RequestHoverAtLocalPositionForTest(const FVector2D& LocalPosition);
	void RequestMoveAtLocalPositionForTest(const FVector2D& LocalPosition);
	void SetCardDepthPointerPositionForTest(const FVector2D& WidgetPosition);
	bool RequestPressAtLocalPositionForTest(const FVector2D& LocalPosition);
	bool RequestGesturePressForTest(const FVector2D& ScreenPosition);
	void RequestGestureMoveForTest(float DeltaTime, const FVector2D& ScreenPosition);
	bool RequestGestureReleaseForTest(const FVector2D& ScreenPosition);

	TOptional<FVector2D> LocalHitCanvasSizeOverrideForTest;
	int32 SlotRuntimeConfigApplyCountForTest = 0;
	int32 EnterTransitionSoundRequestCountForTest = 0;
	int32 OptionalEnterSoundSkipCountForTest = 0;
	int32 DragPickupTriggerCountForTest = 0;
	int32 DragPickupSoundRequestCountForTest = 0;
	float LastDragPickupSoundPitchMultiplierForTest = 1.0f;
	int32 DenySoundRequestCountForTest = 0;
	float LastDenySoundPitchMultiplierForTest = 1.0f;
	int32 PlayedDissolveSoundRequestCountForTest = 0;
	float LastPlayedDissolveSoundPitchMultiplierForTest = 1.0f;
	int32 CardUseEffectSoundRequestCountForTest = 0;
	int32 CardUseReformSoundRequestCountForTest = 0;
	float LastCardUseEffectSoundPitchMultiplierForTest = 1.0f;
	EWacomFirstPersonCardSlotTransitionKind LastEnterTransitionSoundKindForTest =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	int32 PresentationReadinessTimeoutCountForTest = 0;
	int32 PresentationReadinessFallbackCountForTest = 0;
#endif

	friend class UWacomFirstPersonCardLayerWidget;
	friend struct FWacomFirstPersonCardLayerTestAccess;

	void EnsureCardView();
	FWacomFirstPersonCardGestureControllerState& GestureRuntime();
	const FWacomFirstPersonCardGestureControllerState& GestureRuntime() const;
	void ApplyCurrentSlotView();
	const FWacomCardViewData& ResolveCurrentFaceCardViewData() const;
	void BeginLockedFaceInspection();
	void EndLockedFaceInspection(bool bBroadcastClosed);
	void TickFaceFlip(float DeltaTime);
	void CancelFaceFlipAndRestoreDefault();
	void ApplyFaceFlipLocalPresentation(float HorizontalScale, float Opacity);
	void ResetFaceFlipLocalPresentation();
	void ApplyVisualSlotView();
	void ApplySlotViewToWidget(const FWacomFirstPersonCardLayerSlotView& SlotView);
	void RefreshPresentationTarget(
		bool bSnapVisualWhenMotionDisabled,
		EWacomFirstPersonCardMotionIntent PreferredIntent = EWacomFirstPersonCardMotionIntent::Layout);
	FWacomFirstPersonCardSlotVisualState ResolveVisualState(
		const FWacomFirstPersonCardLayerSlotView& BaseSlotView) const;
	EWacomFirstPersonCardMotionIntent ResolveMotionIntentForPresentationChange(
		const FWacomFirstPersonCardLayerSlotView& PreviousBaseSlotView,
		const FWacomFirstPersonCardLayerSlotView& NewBaseSlotView,
		const FWacomFirstPersonCardLayerSlotView& PreviousPresentationSlotView,
		const FWacomFirstPersonCardLayerSlotView& NewPresentationSlotView,
		EWacomFirstPersonCardMotionIntent PreferredIntent) const;
	const FWacomFirstPersonCardMotionProfile& GetMotionProfileForIntent(
		EWacomFirstPersonCardMotionIntent Intent) const;
	FWacomFirstPersonCardLayerSlotView ComposePresentationSlotView(
		const FWacomFirstPersonCardLayerSlotView& BaseSlotView) const;
	EWacomFirstPersonCardDragTargetFeedbackState ResolveEffectiveDragTargetFeedbackState() const;
	void BroadcastVisualSlotUpdatedIfNeeded(
		const FWacomFirstPersonCardLayerSlotView& PreviousVisualSlotView,
		const FWacomFirstPersonCardLayerSlotView& CurrentVisualSlotView);
	bool CanInteractWithCurrentSlot() const;
	bool CanStartCardDragGesture() const;
	bool IsNoTargetDragCard() const;
	bool IsTargetedAimCard() const;
	bool CanPreserveLockedInspectionForSlotRefresh(
		const FWacomFirstPersonCardLayerSlotView& InSlotView,
		bool bTreatAsNewSlot) const;
	void RefreshPreservedLockedInspectionTarget(
		const FWacomFirstPersonCardLayerSlotView& InSlotView);
	FWacomFirstPersonCardLayerSlotView BuildInspectOverrideSlotView() const;
	FWacomFirstPersonCardLayerSlotView BuildNoTargetDragOverrideSlotView() const;
	FWacomFirstPersonCardLayerSlotView BuildAimOverrideSlotView() const;
	const FWacomFirstPersonCardLayerSlotView& GetGestureBaseSlotView() const;
	const FWacomFirstPersonCardLayerSlotView& GetEffectiveTargetSlotView() const;
	bool ResolveInspectScreenPosition(FVector2D& OutScreenPosition) const;
	bool ResolveAbsoluteScreenPositionToWidgetPosition(const FVector2D& AbsoluteScreenPosition, FVector2D& OutWidgetPosition) const;
	bool ResolvePointerWidgetPosition(const FPointerEvent& InMouseEvent, FVector2D& OutScreenPosition) const;
	bool IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const;
	bool IsLocalPositionInsideCardBody(const FVector2D& LocalPosition) const;
	void UpdateBodyHoverFromScreenPosition(const FVector2D& ScreenPosition);
	void UpdateBodyHoverFromLocalPosition(const FVector2D& LocalPosition);
	void BeginGesturePress(
		const FVector2D& ScreenPosition,
		EWacomFirstPersonCardGestureSource Source,
		EWacomFirstPersonCardGestureInputSource InputSource);
	void UpdateGesture(
		float DeltaTime,
		const FVector2D& ScreenPosition,
		bool bSuppressInspectDragPromotion = false,
		bool bBroadcastDragUpdate = true);
	bool ReleaseGesture(
		const FVector2D& ScreenPosition,
		bool bSuppressInspectDragPromotion = false);
	bool PromoteGestureToCardDrag(bool bBroadcastStartOrCancel);
	void SetGestureState(EWacomFirstPersonCardGestureState NewState, bool bBroadcastStartOrCancel);
	void UpdateGestureOverrideTarget();
	void ClearGestureState(bool bBroadcastCancel);
	float ComputeNoTargetDragOutDistance() const;
	void UpdatePointerViewportDiagnostics(const FVector2D& WidgetPosition);
	void ClearPointerViewportDiagnostics();
	void UpdateCardDepthMotion(float DeltaTime);
	void ApplyCardDepthView();
	void BeginDragPickupFeedback();
	void TickDragPickupFeedback(float DeltaTime);
	void TryStartDeferredDragPickupFeedback();
	void ResetDragPickupFeedback();
	void PlayPendingDragPickupSound();
	float GetDragPickupAlpha() const;
	void ResetCardSurfaceEffectView();
	void ApplyActiveSurfaceEffectView();
	void BeginSurfacePresentationReadiness(
		FName EffectName,
		bool bReuseReadyGeneration = false,
		bool bBlocksPresentationPhase = true);
	void BeginCostDigitPresentationReadiness();
	void BeginEffectBadgePresentationReadiness();
	bool ResolveSurfacePresentationReadiness(float DeltaTime, float& OutPlaybackDeltaTime);
	bool ResolveCostDigitPresentationReadiness(float DeltaTime, float& OutPlaybackDeltaTime);
	bool ResolveEffectBadgePresentationReadiness(float DeltaTime, float& OutPlaybackDeltaTime);
	void CancelSurfacePresentationReadiness();
	void CancelSurfacePresentationReadinessIfOwnedBy(FName EffectName);
	void CancelCostDigitPresentationReadiness();
	void CancelEffectBadgePresentationReadiness();
	void CancelAllPresentationReadiness();
	void RefreshPresentationReadinessFrozenFlag();
	void HandleSurfacePresentationReadinessFailure();
	void RecordPresentationReadinessFailure(
		FName ChannelName,
		FName EffectName,
		bool bTimedOut);
	bool CanPlayHandTargetImpact() const;
	void BeginHandTargetImpactPreview();
	void EndHandTargetImpactPreview();
	void TickHandTargetImpactPlayback(float DeltaTime);
	void ClearHandTargetImpactPlayback();
	void ApplyHandTargetImpactSurfaceView();
	void PlayPendingHandTargetImpactSound();
	bool IsHandTargetImpactPlaybackActive() const;
	bool IsHandTargetImpactCommitPlaybackActive() const;
	bool CanPlayCardDataRewrite() const;
	void BeginCardDataRewritePlayback(
		int32 FieldMask,
		EWacomFirstPersonCardDataRewriteTone Tone,
		int32 Seed,
		int32 SequenceIndex,
		bool bAllowSequenceDelay);
	void TickCardDataRewritePlayback(float DeltaTime);
	void ClearCardDataRewritePlayback();
	void ApplyCardDataRewriteView();
	void PlayPendingCardDataRewriteSound();
	bool IsCardDataRewritePlaybackActive() const;
	void TickEffectBadgeFeedbackPlayback(float DeltaTime);
	void ClearEffectBadgeFeedbackPlayback();
	void ApplyEffectBadgeFeedbackView();
	void PlayPendingEffectBadgeFeedbackSound();
	bool IsEffectBadgeFeedbackPlaybackActive() const;
	bool CanPlayDrawReveal() const;
	void PrepareDrawRevealPlayback(EWacomFirstPersonCardSlotTransitionKind TransitionKind);
	void StartDrawRevealPlayback();
	void UpdateDrawRevealPlayback(float NormalizedEnterProgress);
	void ApplyDrawRevealSurfaceView();
	void ClearDrawRevealPlayback();
	bool IsDrawRevealPlaybackActive() const;
	bool CanPlayGainReveal() const;
	void PrepareGainRevealPlayback(EWacomFirstPersonCardSlotTransitionKind TransitionKind);
	void StartGainRevealPlayback();
	void UpdateGainRevealPlayback(float NormalizedEnterProgress);
	void ApplyGainRevealSurfaceView();
	void ClearGainRevealPlayback();
	bool IsGainRevealPlaybackActive() const;
	void TickRetainSealPlayback(float DeltaTime);
	void ApplyRetainSealSurfaceView();
	void ClearRetainSealPlayback();
	bool IsRetainSealPlaybackActive() const;
	bool IsRetainSealPlaybackBlockingPresentation() const;
	bool CanPlayCardUseEffect() const;
	bool CanPlayCardUseReformEffect() const;
	bool CanPlayExhaustDissolve() const;
	void StartSurfaceDeparturePlayback(EWacomFirstPersonCardSlotTransitionKind TransitionKind);
	void TickSurfaceDeparturePlayback(float DeltaTime);
	void ClearSurfaceDeparturePlayback();
	void PlayPendingSurfaceDepartureSound();
	bool IsSurfaceDeparturePlaybackActive() const;
	void TickCardUseReformPlayback(float DeltaTime);
	void ClearCardUseReformPlayback(bool bSnapToTarget = false);
	void PlayPendingCardUseReformSound();
	bool IsCardUseReformPlaybackActive() const;
	bool IsCardUseReformPlaybackBlockingStage() const;
	void TriggerCardUseReformFeedbackInternal(bool bOutboundOnly, bool bInboundOnly);
	void BroadcastDragStarted();
	void BroadcastDragUpdated();
	void BroadcastDragReleased();
	void BroadcastDragCancelled();
	void SetHoveredForFirstPersonLayer(bool bHovered, bool bBroadcast = true);
	void SetPressedForFirstPersonLayer(bool bPressed);
	void TriggerDenyFeedback(
		const FVector2D& ReleaseDirection = FVector2D(0.0f, -1.0f),
		int32 Seed = 0);
	void PlayPendingDenySound();
	void ClearInteractionFeedback();
	bool IsDenyFeedbackActive() const;
	bool IsRetainedFeedbackActive() const;
	void UpdatePressedFeedback(float DeltaTime);
	void ApplyInteractionCue();
	void UpdateVisibilityForInteractionMode();
	void SetTickEnabledForMotion(bool bEnabled);
	void UpdateWantsTick();
	void StartEnterTransitionPlayback(
		const FWacomFirstPersonCardLayerSlotView& StartSlotView,
		const FWacomFirstPersonCardTransitionMotionProfile& EnterProfile);
	void ClearEnterTransitionPlayback();
	bool TickEnterTransitionPlayback(float DeltaTime);
	void BroadcastPendingEnterTransitionStarted();
	void PlayPendingTransitionStartSound();
	bool IsEnterTransitionPlaybackActive() const;
	bool IsEnterTransitionBlockingInteraction() const;
	void StartExitTransitionPlayback(
		const FWacomFirstPersonCardLayerSlotView& StartSlotView,
		const FWacomFirstPersonCardLayerSlotView& TargetSlotView,
		const FWacomFirstPersonCardTransitionMotionProfile& ExitProfile);
	void ClearExitTransitionPlayback();
	bool TickExitTransitionPlayback(float DeltaTime);
	bool IsExitTransitionPlaybackActive() const;

};
