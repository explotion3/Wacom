// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "WacomFirstPersonCardLayerSlotWidget.generated.h"

class UOverlay;
class UImage;
class UWacomCardView;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotTargetNative, const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerSlotDragNative, const FGuid&, const FWacomFirstPersonCardDragView&);

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardTransitionMotionProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D OffsetPixels = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ViewportAnchor = FVector2D(0.5f, 0.5f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ScaleMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float AngleOffsetDegrees = 0.0f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotMotionConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float MotionSpeed = 26.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float OpacitySpeed = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D EnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EnterOpacity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ExitDuration = 0.16f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ResetDistancePixels = 420.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableEventAwareTransitions = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableReadableTransitionOrigins = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DrawnEnterOffsetPixels = FVector2D(0.0f, 96.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DrawnEnterViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D GainedEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode GainedEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D GainedEnterViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode PlayedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayedExitScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayedExitAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DiscardedExitOffsetPixels = FVector2D(0.0f, 120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode DiscardedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DiscardedExitViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DiscardedExitScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DiscardedExitAngleOffsetDegrees = 0.0f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotFeedbackConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor PlayableHoverColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayableHoverOpacity = 0.06f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedScale = 0.985f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor PressedColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedOpacity = 0.10f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ConfirmDuration = 0.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ConfirmOpacity = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyDuration = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyShakePixels = 8.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor DenyColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyOpacity = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnablePlayCommitFeedback = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitDuration = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitOpacity = 0.16f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor PlayCommitColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitScale = 1.015f;
};

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
	void SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass);
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
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride);
	void TriggerCommitFeedback();
	void SetSlotMotionConfig(const FWacomFirstPersonCardSlotMotionConfig& InConfig);
	void SetSlotFeedbackConfig(const FWacomFirstPersonCardSlotFeedbackConfig& InConfig);
	void SetCardDragConfig(const FWacomFirstPersonCardDragConfig& InConfig);
	void SetCardDragFeedbackTarget(
		const FWacomInteractionTargetHandle& TargetHandle,
		bool bValidTarget,
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None,
		const TOptional<FVector2D>& FeedbackTargetScreenPosition = TOptional<FVector2D>());
	void SetCardDragProbeFeedback(bool bEnabled);
	void CancelCardDragGesture(bool bBroadcastCancel);
	void SetCardLayerInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetCardView() const { return CardView; }

	const FWacomFirstPersonCardLayerSlotView& GetSlotView() const { return CurrentSlotView; }
	const FWacomFirstPersonCardLayerSlotView& GetVisualSlotView() const { return VisualSlotView; }
	const FString& GetSlotMotionKey() const { return SlotMotionKey; }
	void SetSlotMotionKey(const FString& InKey) { SlotMotionKey = InKey; }

	bool IsExitingForFirstPersonLayer() const { return bIsExitingForFirstPersonLayer; }
	bool IsExitMotionFinished() const;
	bool WantsSlotMotionTick() const { return bWantsSlotMotionTick; }
	bool CanExposeCardTarget() const;
	FWacomInteractionTargetHandle BuildCardTargetHandle() const;
	EWacomFirstPersonCardGestureState GetGestureStateForFirstPersonLayer() const { return GestureState; }
	FWacomFirstPersonCardDragView BuildDragView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsHoveredForFirstPersonLayer() const { return bIsHoveredForFirstPersonLayer; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardLayerInteractionEnabled() const { return bCardLayerInteractionEnabled; }

#if WITH_AUTOMATION_TESTS
	bool RequestHoverForTest();
	void RequestUnhoverForTest();
	bool RequestPressForTest();
	bool RequestClickForTest();
	bool RequestMouseUpForTest();
	void TickSlotMotionForTest(float DeltaTime);
	float GetFeedbackOverlayRenderOpacityForTest() const;
	FLinearColor GetFeedbackOverlayColorForTest() const;
	bool IsPressedForFirstPersonLayerForTest() const { return bIsPressedForFirstPersonLayer; }
	bool IsDenyFeedbackActiveForTest() const { return DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration; }
	bool IsConfirmFeedbackActiveForTest() const { return ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration; }
	bool IsCommitFeedbackActiveForTest() const { return CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration; }
	const FWacomFirstPersonCardDragConfig& GetCardDragConfigForTest() const { return CardDragConfig; }
	EWacomFirstPersonCardDragTargetFeedbackState GetDragTargetFeedbackStateForTest() const
	{
		return DragTargetFeedbackState;
	}
	bool HasCardDragProbeFeedbackForTest() const { return bCardDragProbeFeedback; }
	bool RequestGesturePressForTest(const FVector2D& ScreenPosition);
	void RequestGestureMoveForTest(float DeltaTime, const FVector2D& ScreenPosition);
	bool RequestGestureReleaseForTest(const FVector2D& ScreenPosition);
#endif

	FWacomFirstPersonCardLayerSlotInteractionNative OnCardClickedNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardHoveredNative;
	FWacomFirstPersonCardLayerSlotInteractionNative OnCardUnhoveredNative;
	FWacomFirstPersonCardLayerSlotTargetNative OnCardTargetHoveredNative;
	FWacomFirstPersonCardLayerSlotTargetNative OnCardTargetUnhoveredNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragStartedNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragUpdatedNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragReleasedNative;
	FWacomFirstPersonCardLayerSlotDragNative OnCardDragCancelledNative;

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
	TObjectPtr<UWacomCardView> CardView;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FeedbackOverlay;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> CardViewClass;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView CurrentSlotView;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView TargetSlotView;

	UPROPERTY(Transient)
	FWacomFirstPersonCardLayerSlotView VisualSlotView;

	FWacomFirstPersonCardSlotMotionConfig SlotMotionConfig;
	FWacomFirstPersonCardSlotFeedbackConfig SlotFeedbackConfig;
	FWacomFirstPersonCardDragConfig CardDragConfig;
	FString SlotMotionKey;
	EWacomFirstPersonCardGestureState GestureState = EWacomFirstPersonCardGestureState::Idle;
	TOptional<FWacomFirstPersonCardLayerSlotView> GestureStartSlotView;
	TOptional<FWacomFirstPersonCardLayerSlotView> GestureOverrideTargetSlotView;
	FWacomInteractionTargetHandle GestureFeedbackTargetHandle;
	FVector2D PressScreenPosition = FVector2D::ZeroVector;
	FVector2D CurrentGestureScreenPosition = FVector2D::ZeroVector;
	float GestureElapsedSeconds = 0.0f;
	float ExitMotionElapsedSeconds = 0.0f;
	float ConfirmFeedbackElapsedSeconds = 999999.0f;
	float DenyFeedbackElapsedSeconds = 999999.0f;
	float CommitFeedbackElapsedSeconds = 999999.0f;
	bool bCardLayerInteractionEnabled = false;
	bool bIsHoveredForFirstPersonLayer = false;
	bool bIsPressedForFirstPersonLayer = false;
	bool bHasVisualSlotView = false;
	bool bIsExitingForFirstPersonLayer = false;
	bool bWantsSlotMotionTick = false;
	bool bGestureTargetValid = false;
	bool bGestureCommitArmed = false;
	bool bSuppressClickOnRelease = false;
	bool bHasPointerViewportPosition = false;
	bool bHasFeedbackTargetScreenPosition = false;
	bool bCardDragProbeFeedback = false;
	EWacomFirstPersonCardDragTargetFeedbackState DragTargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	FVector2D PointerViewportPosition = FVector2D::ZeroVector;
	FVector2D PointerNormalizedViewportPosition = FVector2D::ZeroVector;
	FVector2D FeedbackTargetScreenPosition = FVector2D::ZeroVector;

	void EnsureCardView();
	void EnsureFeedbackOverlay();
	void ApplyCurrentSlotView();
	void ApplyVisualSlotView();
	void ApplySlotViewToWidget(const FWacomFirstPersonCardLayerSlotView& SlotView);
	bool CanInteractWithCurrentSlot() const;
	bool CanApplyPlayableHoverFeedback() const;
	bool CanClickCurrentSlot() const;
	bool CanStartCardDragGesture() const;
	bool IsNoTargetDragCard() const;
	bool IsTargetedAimCard() const;
	FWacomFirstPersonCardLayerSlotView BuildInspectOverrideSlotView() const;
	FWacomFirstPersonCardLayerSlotView BuildNoTargetDragOverrideSlotView() const;
	FWacomFirstPersonCardLayerSlotView BuildAimOverrideSlotView() const;
	const FWacomFirstPersonCardLayerSlotView& GetGestureBaseSlotView() const;
	const FWacomFirstPersonCardLayerSlotView& GetEffectiveTargetSlotView() const;
	bool ResolveInspectScreenPosition(FVector2D& OutScreenPosition) const;
	bool ResolvePointerWidgetPosition(const FPointerEvent& InMouseEvent, FVector2D& OutScreenPosition) const;
	void BeginGesturePress(const FVector2D& ScreenPosition);
	void UpdateGesture(float DeltaTime, const FVector2D& ScreenPosition);
	bool ReleaseGesture(const FVector2D& ScreenPosition);
	void SetGestureState(EWacomFirstPersonCardGestureState NewState, bool bBroadcastStartOrCancel);
	void UpdateGestureOverrideTarget();
	void ClearGestureState(bool bBroadcastCancel);
	float ComputeNoTargetDragOutDistance() const;
	void UpdatePointerViewportDiagnostics(const FVector2D& WidgetPosition);
	void ClearPointerViewportDiagnostics();
	void BroadcastDragStarted();
	void BroadcastDragUpdated();
	void BroadcastDragReleased();
	void BroadcastDragCancelled();
	void SetHoveredForFirstPersonLayer(bool bHovered);
	void SetPressedForFirstPersonLayer(bool bPressed);
	void TriggerConfirmFeedback();
	void TriggerDenyFeedback();
	void ClearInteractionFeedback();
	void ApplyFeedbackOverlay();
	void UpdateVisibilityForInteractionMode();
	void SetTickEnabledForMotion(bool bEnabled);
	void UpdateWantsTick();
	static FWacomFirstPersonCardLayerSlotView LerpSlotView(
		const FWacomFirstPersonCardLayerSlotView& From,
		const FWacomFirstPersonCardLayerSlotView& To,
		float MotionAlpha,
		float OpacityAlpha);
};
