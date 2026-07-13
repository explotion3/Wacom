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
class FWacomFirstPersonCardDragPickupPlayback;
class FWacomFirstPersonCardHandTargetImpactPlayback;
class FWacomFirstPersonCardSurfaceDeparturePlayback;
class FWacomFirstPersonCardUseReformPlayback;
class FWacomFirstPersonCardTransitionPlayback;
struct FWacomFirstPersonCardLayerTestAccess;

struct FWacomFirstPersonCardTransitionPlaybackDeleter
{
	void operator()(FWacomFirstPersonCardTransitionPlayback* Playback) const;
};

struct FWacomFirstPersonCardDepthMotionDeleter
{
	void operator()(FWacomFirstPersonCardDepthMotion* Motion) const;
};

struct FWacomFirstPersonCardDragPickupPlaybackDeleter
{
	void operator()(FWacomFirstPersonCardDragPickupPlayback* Playback) const;
};

struct FWacomFirstPersonCardSurfaceDeparturePlaybackDeleter
{
	void operator()(FWacomFirstPersonCardSurfaceDeparturePlayback* Playback) const;
};

struct FWacomFirstPersonCardHandTargetImpactPlaybackDeleter
{
	void operator()(FWacomFirstPersonCardHandTargetImpactPlayback* Playback) const;
};

struct FWacomFirstPersonCardUseReformPlaybackDeleter
{
	void operator()(FWacomFirstPersonCardUseReformPlayback* Playback) const;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotTargetNative, const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotDragNative, const FGuid&, const FWacomFirstPersonCardDragView&);
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
	float FeedbackOverlayOpacity = 0.0f;
	FLinearColor FeedbackOverlayColor = FLinearColor::Transparent;
	float InteractionFeedbackOpacity = 0.0f;
	EWacomFirstPersonCardInteractionFeedbackKind InteractionFeedbackKind =
		EWacomFirstPersonCardInteractionFeedbackKind::None;
	bool bHasInteractionFeedbackImage = false;
	bool bInteractionFeedbackMaterialConfigured = false;
	bool bInteractionFeedbackMaterialLoaded = false;
	bool bInteractionFeedbackUsesOverrideMaterial = false;
	bool bInteractionFeedbackUsesBrushMaterial = false;
	bool bInteractionFeedbackLayerAboveFeedbackOverlay = false;
	EWacomFirstPersonCardGestureSource GestureSource = EWacomFirstPersonCardGestureSource::None;
	bool bPressed = false;
	bool bDenyFeedbackActive = false;
	bool bConfirmFeedbackActive = false;
	bool bCommitFeedbackActive = false;
	bool bRetainedFeedbackActive = false;
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
	FWidgetTransform RenderTransform;
	int32 RenderZOrder = 0;
	bool bDragPickupFeedbackActive = false;
	float DragPickupAlpha = 0.0f;
	int32 DragPickupTriggerCount = 0;
	int32 DragPickupSoundRequestCount = 0;
	float LastDragPickupSoundPitchMultiplier = 1.0f;
	bool bPlayedDissolvePlaybackActive = false;
	bool bCardUseEffectPlaybackActive = false;
	bool bCardUseReformPlaybackActive = false;
	bool bCardUseReformUsingTargetSlot = false;
	bool bHandTargetImpactCommitActive = false;
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
	EWacomFirstPersonCardSlotTransitionKind LastEnterTransitionSoundKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	FWacomFirstPersonCardDragConfig CardDragConfig;
	FWacomFirstPersonCardSlotVisualConfig SlotVisualConfig;
	int32 SlotMotionConfigApplyCount = 0;
	int32 SlotFeedbackConfigApplyCount = 0;
	int32 CardDragConfigApplyCount = 0;
	int32 SlotVisualConfigApplyCount = 0;
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
	void TriggerRetainedFeedback(int32 SequenceIndex, int32 SequenceCount);
	void TriggerCardUseReformFeedback();
	void TriggerHandTargetImpactFeedback();
	void BeginDeferredExitWithHandTargetImpact(
		const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind);
	bool IsHandTargetImpactDeparturePending() const { return bHandTargetImpactDeparturePending; }
	bool IsHandTargetImpactDepartureGateOpen() const;
	void SetHandTargetImpactDepartureOwnedByPileTransfer(bool bOwned);
	void ReleaseDeferredHandTargetExitNow();
	bool HasActivePresentationPlayback() const;
	void ForceCompletePresentationPlayback();
	void SetSlotMotionConfig(const FWacomFirstPersonCardSlotMotionConfig& InConfig);
	void SetSlotVisualConfig(const FWacomFirstPersonCardSlotVisualConfig& InConfig);
	void SetSlotFeedbackConfig(const FWacomFirstPersonCardSlotFeedbackConfig& InConfig);
	void SetCardDragConfig(const FWacomFirstPersonCardDragConfig& InConfig);
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
	EWacomFirstPersonCardGestureState GetGestureStateForFirstPersonLayer() const { return GestureState; }
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
	FWacomFirstPersonCardSlotFeedbackConfig SlotFeedbackConfig;
	FWacomFirstPersonCardDragConfig CardDragConfig;
	FString SlotMotionKey;
	EWacomFirstPersonCardMotionIntent ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	EWacomFirstPersonCardGestureState GestureState = EWacomFirstPersonCardGestureState::Idle;
	EWacomFirstPersonCardGestureSource GestureSource = EWacomFirstPersonCardGestureSource::None;
	EWacomFirstPersonCardGestureInputSource GestureInputSource = EWacomFirstPersonCardGestureInputSource::None;
	TOptional<FWacomFirstPersonCardLayerSlotView> GestureStartSlotView;
	TOptional<FWacomFirstPersonCardLayerSlotView> GestureOverrideTargetSlotView;
	FWacomInteractionTargetHandle GestureFeedbackTargetHandle;
	FVector2D PressScreenPosition = FVector2D::ZeroVector;
	FVector2D CurrentGestureScreenPosition = FVector2D::ZeroVector;
	float GestureElapsedSeconds = 0.0f;
	float ExitMotionElapsedSeconds = 0.0f;
	TUniquePtr<
		FWacomFirstPersonCardTransitionPlayback,
		FWacomFirstPersonCardTransitionPlaybackDeleter> TransitionPlayback;
	TUniquePtr<
		FWacomFirstPersonCardDepthMotion,
		FWacomFirstPersonCardDepthMotionDeleter> CardDepthMotion;
	TUniquePtr<
		FWacomFirstPersonCardDragPickupPlayback,
		FWacomFirstPersonCardDragPickupPlaybackDeleter> DragPickupPlayback;
	TUniquePtr<
		FWacomFirstPersonCardSurfaceDeparturePlayback,
		FWacomFirstPersonCardSurfaceDeparturePlaybackDeleter> SurfaceDeparturePlayback;
	TUniquePtr<
		FWacomFirstPersonCardUseReformPlayback,
		FWacomFirstPersonCardUseReformPlaybackDeleter> CardUseReformPlayback;
	TUniquePtr<
		FWacomFirstPersonCardHandTargetImpactPlayback,
		FWacomFirstPersonCardHandTargetImpactPlaybackDeleter> HandTargetImpactPlayback;
	FWacomFirstPersonCardLayerSlotView DeferredHandTargetExitSlotView;
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> DeferredHandTargetExitProfile;
	EWacomFirstPersonCardSlotTransitionKind DeferredHandTargetExitTransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	float HandTargetImpactScaleMultiplier = 1.0f;
	float HandTargetImpactTranslationYPixels = 0.0f;
	int32 HandTargetImpactZOrderBoost = 0;
	bool bHandTargetImpactDeparturePending = false;
	bool bHandTargetImpactDepartureOwnedByPileTransfer = false;
	bool bHandTargetImpactDepartureGateReleased = false;
	FWacomFirstPersonCardLayerSlotView CardUseReformStartSlotView;
	float CardUseFlipProgress = 0.0f;
	float CardUseImpactProgress = 0.0f;
	float CardUseMotionAlpha = 0.0f;
	float CardUseOpacityMultiplier = 1.0f;
	float ConfirmFeedbackElapsedSeconds = 999999.0f;
	float DenyFeedbackElapsedSeconds = 999999.0f;
	float CommitFeedbackElapsedSeconds = 999999.0f;
	float RetainedFeedbackElapsedSeconds = 999999.0f;
	float RetainedFeedbackStartDelaySeconds = 0.0f;
	bool bCardLayerInteractionEnabled = false;
	bool bIsHoveredForFirstPersonLayer = false;
	bool bIsPressedForFirstPersonLayer = false;
	bool bHasVisualSlotView = false;
	bool bIsExitingForFirstPersonLayer = false;
	bool bUsesFixedExitTransitionPlayback = false;
	bool bUsesSurfaceDepartureExit = false;
	EWacomFirstPersonCardSlotTransitionKind SurfaceDepartureTransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	bool bWantsSlotMotionTick = false;
	bool bPreserveGestureReturnMotion = false;
	bool bGestureTargetValid = false;
	bool bGestureCommitArmed = false;
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
	int32 SlotMotionConfigApplyCountForTest = 0;
	int32 SlotFeedbackConfigApplyCountForTest = 0;
	int32 CardDragConfigApplyCountForTest = 0;
	int32 SlotVisualConfigApplyCountForTest = 0;
	int32 EnterTransitionSoundRequestCountForTest = 0;
	int32 DragPickupTriggerCountForTest = 0;
	int32 DragPickupSoundRequestCountForTest = 0;
	float LastDragPickupSoundPitchMultiplierForTest = 1.0f;
	int32 PlayedDissolveSoundRequestCountForTest = 0;
	float LastPlayedDissolveSoundPitchMultiplierForTest = 1.0f;
	int32 CardUseEffectSoundRequestCountForTest = 0;
	int32 CardUseReformSoundRequestCountForTest = 0;
	float LastCardUseEffectSoundPitchMultiplierForTest = 1.0f;
	EWacomFirstPersonCardSlotTransitionKind LastEnterTransitionSoundKindForTest =
		EWacomFirstPersonCardSlotTransitionKind::Default;
#endif

	friend class UWacomFirstPersonCardLayerWidget;
	friend struct FWacomFirstPersonCardLayerTestAccess;

	void EnsureCardView();
	void ApplyCurrentSlotView();
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
	bool CanApplyPlayableHoverFeedback() const;
	bool CanStartCardDragGesture() const;
	bool IsNoTargetDragCard() const;
	bool IsTargetedAimCard() const;
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
	bool CanPlayHandTargetImpact() const;
	void BeginHandTargetImpactPreview();
	void EndHandTargetImpactPreview();
	void TickHandTargetImpactPlayback(float DeltaTime);
	void ClearHandTargetImpactPlayback();
	void ApplyHandTargetImpactSurfaceView();
	void PlayPendingHandTargetImpactSound();
	bool IsHandTargetImpactPlaybackActive() const;
	bool IsHandTargetImpactCommitPlaybackActive() const;
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
	void BroadcastDragStarted();
	void BroadcastDragUpdated();
	void BroadcastDragReleased();
	void BroadcastDragCancelled();
	void SetHoveredForFirstPersonLayer(bool bHovered, bool bBroadcast = true);
	void SetPressedForFirstPersonLayer(bool bPressed);
	void TriggerConfirmFeedback();
	void TriggerDenyFeedback();
	void ClearInteractionFeedback();
	bool IsDenyFeedbackActive() const;
	bool IsRetainedFeedbackActive() const;
	float ComputeRetainedFeedbackAlpha() const;
	void ApplyFeedbackOverlay();
	void ApplyInteractionFeedbackOverlay();
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

	FVector2D EnterTransitionStartWidgetPosition = FVector2D::ZeroVector;
};
