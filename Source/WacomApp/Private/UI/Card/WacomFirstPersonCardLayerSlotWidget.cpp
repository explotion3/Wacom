// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/GameViewportClient.h"
#include "Styling/SlateBrush.h"
#include "InputCoreTypes.h"
#include "UI/Card/WacomCardView.h"

namespace
{
	float ComputeInterpAlpha(float Speed, float DeltaTime)
	{
		return Speed <= 0.0f ? 1.0f : FMath::Clamp(DeltaTime * Speed, 0.0f, 1.0f);
	}

	float ComputePulseAlpha(float ElapsedSeconds, float DurationSeconds)
	{
		if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds)
		{
			return 0.0f;
		}

		return 1.0f - FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass)
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
	if (CardView)
	{
		CardView->RemoveFromParent();
		CardView = nullptr;
	}
	EnsureCardView();
	ApplyCurrentSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotView(const FWacomFirstPersonCardLayerSlotView& InSlotView)
{
	BeginSlotMotion(InSlotView, !bHasVisualSlotView);
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotViewImmediate(
	const FWacomFirstPersonCardLayerSlotView& InSlotView)
{
	if (bIsHoveredForFirstPersonLayer
		&& (CurrentSlotView.Entry.CardInstanceId != InSlotView.Entry.CardInstanceId
			|| !InSlotView.bProjected
			|| !InSlotView.Entry.CardInstanceId.IsValid()))
	{
		SetHoveredForFirstPersonLayer(false);
	}
	if (CurrentSlotView.Entry.CardInstanceId != InSlotView.Entry.CardInstanceId
		|| !InSlotView.bProjected
		|| !InSlotView.Entry.CardInstanceId.IsValid())
	{
		ClearGestureState(true);
	}
	ClearInteractionFeedback();

	CurrentSlotView = InSlotView;
	TargetSlotView = InSlotView;
	VisualSlotView = InSlotView;
	bHasVisualSlotView = true;
	bIsExitingForFirstPersonLayer = false;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();
	ApplyVisualSlotView();
	SetTickEnabledForMotion(false);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSlotMotion(
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	bool bTreatAsNewSlot)
{
	BeginSlotMotionWithEnterOffset(InTargetSlotView, bTreatAsNewSlot, TOptional<FVector2D>());
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSlotMotionWithEnterOffset(
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	bool bTreatAsNewSlot,
	const TOptional<FVector2D>& EnterOffsetOverride)
{
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> EnterProfileOverride;
	if (EnterOffsetOverride.IsSet())
	{
		FWacomFirstPersonCardTransitionMotionProfile Profile;
		Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Profile.OffsetPixels = EnterOffsetOverride.GetValue();
		EnterProfileOverride = Profile;
	}
	BeginSlotMotionWithEnterProfile(InTargetSlotView, bTreatAsNewSlot, EnterProfileOverride);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSlotMotionWithEnterProfile(
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	bool bTreatAsNewSlot,
	const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& EnterProfileOverride)
{
	if (!SlotMotionConfig.bEnabled)
	{
		SetSlotViewImmediate(InTargetSlotView);
		return;
	}

	if (bIsHoveredForFirstPersonLayer
		&& (CurrentSlotView.Entry.CardInstanceId != InTargetSlotView.Entry.CardInstanceId
			|| !InTargetSlotView.bProjected
			|| !InTargetSlotView.Entry.CardInstanceId.IsValid()))
	{
		SetHoveredForFirstPersonLayer(false);
	}
	if (CurrentSlotView.Entry.CardInstanceId != InTargetSlotView.Entry.CardInstanceId
		|| !InTargetSlotView.bProjected
		|| !InTargetSlotView.Entry.CardInstanceId.IsValid())
	{
		ClearGestureState(true);
		ClearInteractionFeedback();
	}

	const bool bCanReuseVisual =
		bHasVisualSlotView
		&& !bTreatAsNewSlot
		&& CurrentSlotView.Entry.CardInstanceId == InTargetSlotView.Entry.CardInstanceId;
	const float JumpDistance = bCanReuseVisual
		? FVector2D::Distance(VisualSlotView.ScreenPosition, InTargetSlotView.ScreenPosition)
		: 0.0f;
	const bool bGestureActive =
		GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	const bool bLargeJump =
		bCanReuseVisual
		&& !bGestureActive
		&& SlotMotionConfig.ResetDistancePixels > 0.0f
		&& JumpDistance > SlotMotionConfig.ResetDistancePixels;

	CurrentSlotView = InTargetSlotView;
	TargetSlotView = InTargetSlotView;
	bIsExitingForFirstPersonLayer = false;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();

	if (!bCanReuseVisual || bLargeJump)
	{
		VisualSlotView = InTargetSlotView;
		if (bTreatAsNewSlot && InTargetSlotView.bProjected)
		{
			FWacomFirstPersonCardTransitionMotionProfile DefaultEnterProfile;
			DefaultEnterProfile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
			DefaultEnterProfile.OffsetPixels = SlotMotionConfig.EnterOffsetPixels;
			DefaultEnterProfile.ViewportAnchor = FVector2D(0.5f, 0.5f);
			DefaultEnterProfile.ScaleMultiplier = 1.0f;
			DefaultEnterProfile.AngleOffsetDegrees = 0.0f;
			const FWacomFirstPersonCardTransitionMotionProfile EnterProfile =
				EnterProfileOverride.Get(DefaultEnterProfile);
			VisualSlotView.ScreenPosition = InTargetSlotView.ScreenPosition + EnterProfile.OffsetPixels;
			VisualSlotView.WidgetPosition = VisualSlotView.ScreenPosition;
			VisualSlotView.SnappedWidgetPosition = VisualSlotView.ScreenPosition;
			VisualSlotView.RenderScale =
				FMath::Max(0.01f, InTargetSlotView.RenderScale * FMath::Max(0.01f, EnterProfile.ScaleMultiplier));
			VisualSlotView.RenderAngleDegrees =
				InTargetSlotView.RenderAngleDegrees + EnterProfile.AngleOffsetDegrees;
			VisualSlotView.RenderOpacity = FMath::Clamp(SlotMotionConfig.EnterOpacity, 0.0f, 1.0f);
		}
		bHasVisualSlotView = true;
	}

	ApplyVisualSlotView();
	SetTickEnabledForMotion(true);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginExitMotion(
	const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView)
{
	BeginExitMotionWithOffset(InExitTargetSlotView, TOptional<FVector2D>());
}

void UWacomFirstPersonCardLayerSlotWidget::BeginExitMotionWithOffset(
	const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
	const TOptional<FVector2D>& ExitOffsetOverride)
{
	TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfileOverride;
	if (ExitOffsetOverride.IsSet())
	{
		FWacomFirstPersonCardTransitionMotionProfile Profile;
		Profile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Profile.OffsetPixels = ExitOffsetOverride.GetValue();
		ExitProfileOverride = Profile;
	}
	BeginExitMotionWithProfile(InExitTargetSlotView, ExitProfileOverride);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginExitMotionWithProfile(
	const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
	const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride)
{
	if (!SlotMotionConfig.bEnabled || SlotMotionConfig.ExitDuration <= 0.0f || !bHasVisualSlotView)
	{
		SetHoveredForFirstPersonLayer(false);
		ClearInteractionFeedback();
		bIsExitingForFirstPersonLayer = true;
		ExitMotionElapsedSeconds = SlotMotionConfig.ExitDuration;
		SetVisibility(ESlateVisibility::Collapsed);
		SetTickEnabledForMotion(false);
		return;
	}

	SetHoveredForFirstPersonLayer(false);
	ClearGestureState(true);
	ClearInteractionFeedback();
	CurrentSlotView = InExitTargetSlotView;
	CurrentSlotView.bProjected = false;
	TargetSlotView = InExitTargetSlotView;
	FWacomFirstPersonCardTransitionMotionProfile DefaultExitProfile;
	DefaultExitProfile.OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
	DefaultExitProfile.OffsetPixels = SlotMotionConfig.ExitOffsetPixels;
	DefaultExitProfile.ViewportAnchor = FVector2D(0.5f, 0.5f);
	DefaultExitProfile.ScaleMultiplier = 1.0f;
	DefaultExitProfile.AngleOffsetDegrees = 0.0f;
	const FWacomFirstPersonCardTransitionMotionProfile ExitProfile =
		ExitProfileOverride.Get(DefaultExitProfile);
	TargetSlotView.ScreenPosition = VisualSlotView.ScreenPosition + ExitProfile.OffsetPixels;
	TargetSlotView.WidgetPosition = TargetSlotView.ScreenPosition;
	TargetSlotView.SnappedWidgetPosition = TargetSlotView.ScreenPosition;
	TargetSlotView.RenderScale =
		FMath::Max(0.01f, VisualSlotView.RenderScale * FMath::Max(0.01f, ExitProfile.ScaleMultiplier));
	TargetSlotView.RenderAngleDegrees =
		VisualSlotView.RenderAngleDegrees + ExitProfile.AngleOffsetDegrees;
	TargetSlotView.RenderOpacity = 0.0f;
	TargetSlotView.bProjected = VisualSlotView.bProjected;
	bIsExitingForFirstPersonLayer = true;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();
	ApplyVisualSlotView();
	SetTickEnabledForMotion(true);
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotMotionConfig(
	const FWacomFirstPersonCardSlotMotionConfig& InConfig)
{
	SlotMotionConfig = InConfig;
	SlotMotionConfig.MotionSpeed = FMath::Max(0.0f, SlotMotionConfig.MotionSpeed);
	SlotMotionConfig.OpacitySpeed = FMath::Max(0.0f, SlotMotionConfig.OpacitySpeed);
	SlotMotionConfig.EnterOpacity = FMath::Clamp(SlotMotionConfig.EnterOpacity, 0.0f, 1.0f);
	SlotMotionConfig.ExitDuration = FMath::Max(0.0f, SlotMotionConfig.ExitDuration);
	SlotMotionConfig.ResetDistancePixels = FMath::Max(0.0f, SlotMotionConfig.ResetDistancePixels);
	SlotMotionConfig.DrawnEnterViewportAnchor.X = FMath::Clamp(SlotMotionConfig.DrawnEnterViewportAnchor.X, 0.0f, 1.0f);
	SlotMotionConfig.DrawnEnterViewportAnchor.Y = FMath::Clamp(SlotMotionConfig.DrawnEnterViewportAnchor.Y, 0.0f, 1.0f);
	SlotMotionConfig.DrawnEnterScaleMultiplier = FMath::Max(0.01f, SlotMotionConfig.DrawnEnterScaleMultiplier);
	SlotMotionConfig.GainedEnterViewportAnchor.X = FMath::Clamp(SlotMotionConfig.GainedEnterViewportAnchor.X, 0.0f, 1.0f);
	SlotMotionConfig.GainedEnterViewportAnchor.Y = FMath::Clamp(SlotMotionConfig.GainedEnterViewportAnchor.Y, 0.0f, 1.0f);
	SlotMotionConfig.GainedEnterScaleMultiplier = FMath::Max(0.01f, SlotMotionConfig.GainedEnterScaleMultiplier);
	SlotMotionConfig.PlayedExitViewportAnchor.X = FMath::Clamp(SlotMotionConfig.PlayedExitViewportAnchor.X, 0.0f, 1.0f);
	SlotMotionConfig.PlayedExitViewportAnchor.Y = FMath::Clamp(SlotMotionConfig.PlayedExitViewportAnchor.Y, 0.0f, 1.0f);
	SlotMotionConfig.PlayedExitScaleMultiplier = FMath::Max(0.01f, SlotMotionConfig.PlayedExitScaleMultiplier);
	SlotMotionConfig.DiscardedExitViewportAnchor.X = FMath::Clamp(SlotMotionConfig.DiscardedExitViewportAnchor.X, 0.0f, 1.0f);
	SlotMotionConfig.DiscardedExitViewportAnchor.Y = FMath::Clamp(SlotMotionConfig.DiscardedExitViewportAnchor.Y, 0.0f, 1.0f);
	SlotMotionConfig.DiscardedExitScaleMultiplier = FMath::Max(0.01f, SlotMotionConfig.DiscardedExitScaleMultiplier);
	if (!SlotMotionConfig.bEnabled && bHasVisualSlotView)
	{
		SetSlotViewImmediate(TargetSlotView);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	SlotFeedbackConfig = InConfig;
	SlotFeedbackConfig.PlayableHoverOpacity = FMath::Clamp(SlotFeedbackConfig.PlayableHoverOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.PressedScale = FMath::Max(0.01f, SlotFeedbackConfig.PressedScale);
	SlotFeedbackConfig.PressedOpacity = FMath::Clamp(SlotFeedbackConfig.PressedOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.ConfirmDuration = FMath::Max(0.0f, SlotFeedbackConfig.ConfirmDuration);
	SlotFeedbackConfig.ConfirmOpacity = FMath::Clamp(SlotFeedbackConfig.ConfirmOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.DenyDuration = FMath::Max(0.0f, SlotFeedbackConfig.DenyDuration);
	SlotFeedbackConfig.DenyShakePixels = FMath::Max(0.0f, SlotFeedbackConfig.DenyShakePixels);
	SlotFeedbackConfig.DenyOpacity = FMath::Clamp(SlotFeedbackConfig.DenyOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.PlayCommitDuration = FMath::Max(0.0f, SlotFeedbackConfig.PlayCommitDuration);
	SlotFeedbackConfig.PlayCommitOpacity = FMath::Clamp(SlotFeedbackConfig.PlayCommitOpacity, 0.0f, 1.0f);
	SlotFeedbackConfig.PlayCommitScale = FMath::Max(0.01f, SlotFeedbackConfig.PlayCommitScale);
	if (!SlotFeedbackConfig.bEnabled)
	{
		ClearInteractionFeedback();
	}
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragConfig(
	const FWacomFirstPersonCardDragConfig& InConfig)
{
	CardDragConfig = InConfig;
	CardDragConfig.CardInspectHoldDelaySeconds = FMath::Max(0.0f, CardDragConfig.CardInspectHoldDelaySeconds);
	CardDragConfig.CardDragStartThresholdPixels = FMath::Max(0.0f, CardDragConfig.CardDragStartThresholdPixels);
	CardDragConfig.NoTargetCardDragOutCommitDistancePixels =
		FMath::Max(0.0f, CardDragConfig.NoTargetCardDragOutCommitDistancePixels);
	CardDragConfig.CardInspectScreenPosition.X = FMath::Clamp(CardDragConfig.CardInspectScreenPosition.X, 0.0f, 1.0f);
	CardDragConfig.CardInspectScreenPosition.Y = FMath::Clamp(CardDragConfig.CardInspectScreenPosition.Y, 0.0f, 1.0f);
	CardDragConfig.CardInspectScale = FMath::Max(0.01f, CardDragConfig.CardInspectScale);
	CardDragConfig.CardDragCameraLookScale = FMath::Max(0.0f, CardDragConfig.CardDragCameraLookScale);
	CardDragConfig.DragTargetFeedbackOpacity =
		FMath::Clamp(CardDragConfig.DragTargetFeedbackOpacity, 0.0f, 1.0f);
	CardDragConfig.DragAimArrowSnapBlend =
		FMath::Clamp(CardDragConfig.DragAimArrowSnapBlend, 0.0f, 1.0f);
	CardDragConfig.DragCommitReadyScale = FMath::Max(0.01f, CardDragConfig.DragCommitReadyScale);
	CardDragConfig.DragCardTargetProbeScale = FMath::Max(0.01f, CardDragConfig.DragCardTargetProbeScale);
	CardDragConfig.SelectedSourceLiftPixels = FMath::Max(0.0f, CardDragConfig.SelectedSourceLiftPixels);
	CardDragConfig.SelectedSourceScale = FMath::Max(0.01f, CardDragConfig.SelectedSourceScale);
	CardDragConfig.SelectedSourceZOrderBoost = FMath::Max(0, CardDragConfig.SelectedSourceZOrderBoost);
	CardDragConfig.SelectedSourceAngleBlend =
		FMath::Clamp(CardDragConfig.SelectedSourceAngleBlend, 0.0f, 1.0f);
	if (!CardDragConfig.bEnableFirstPersonCardDragCommit)
	{
		ClearGestureState(true);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragFeedbackTarget(
	const FWacomInteractionTargetHandle& TargetHandle,
	bool bValidTarget,
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	const TOptional<FVector2D>& InFeedbackTargetScreenPosition)
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return;
	}

	GestureFeedbackTargetHandle = TargetHandle;
	bGestureTargetValid = TargetHandle.IsValid() && bValidTarget;
	DragTargetFeedbackState = CardDragConfig.bEnableDragTargetFeedback
		? FeedbackState
		: EWacomFirstPersonCardDragTargetFeedbackState::None;
	if (InFeedbackTargetScreenPosition.IsSet())
	{
		bHasFeedbackTargetScreenPosition = true;
		FeedbackTargetScreenPosition = InFeedbackTargetScreenPosition.GetValue();
	}
	else
	{
		bHasFeedbackTargetScreenPosition = false;
		FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	}
	ApplyVisualSlotView();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragProbeFeedback(bool bEnabled, bool bValidTarget)
{
	SetCardDragTargetAffordanceFeedback(
		bEnabled
			? (bValidTarget
				? EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
				: EWacomFirstPersonCardDragTargetFeedbackState::CardProbe)
			: EWacomFirstPersonCardDragTargetFeedbackState::None,
		bValidTarget);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragTargetAffordanceFeedback(
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState,
	bool bValidTarget)
{
	const bool bEnableFeedback =
		FeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		&& CardDragConfig.bEnableDragTargetFeedback;
	const bool bValidFeedback =
		bEnableFeedback
		&& (bValidTarget
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	const EWacomFirstPersonCardDragTargetFeedbackState NextState =
		bEnableFeedback ? FeedbackState : EWacomFirstPersonCardDragTargetFeedbackState::None;

	if (DragTargetFeedbackState == NextState
		&& bCardDragProbeFeedback == bEnableFeedback
		&& bCardDragProbeFeedbackValid == bValidFeedback)
	{
		return;
	}

	DragTargetFeedbackState = NextState;
	bCardDragProbeFeedback = bEnableFeedback;
	bCardDragProbeFeedbackValid = bValidFeedback;
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::CancelCardDragGesture(bool bBroadcastCancel)
{
	ClearGestureState(bBroadcastCancel);
}

bool UWacomFirstPersonCardLayerSlotWidget::IsExitMotionFinished() const
{
	return bIsExitingForFirstPersonLayer
		&& ExitMotionElapsedSeconds >= FMath::Max(0.0f, SlotMotionConfig.ExitDuration);
}

bool UWacomFirstPersonCardLayerSlotWidget::CanExposeCardTarget() const
{
	return bCardLayerInteractionEnabled
		&& !bIsExitingForFirstPersonLayer
		&& bHasVisualSlotView
		&& VisualSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

FWacomInteractionTargetHandle UWacomFirstPersonCardLayerSlotWidget::BuildCardTargetHandle() const
{
	if (!CanExposeCardTarget())
	{
		return FWacomInteractionTargetHandle();
	}

	return FWacomInteractionTargetHandle::ForCardTarget(
		CurrentSlotView.Entry.CardInstanceId,
		const_cast<UWacomFirstPersonCardLayerSlotWidget*>(this),
		VisualSlotView.ScreenPosition);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardLayerInteractionEnabled(bool bEnabled)
{
	if (bCardLayerInteractionEnabled == bEnabled)
	{
		return;
	}

	if (!bEnabled)
	{
		SetHoveredForFirstPersonLayer(false);
		ClearGestureState(true);
		ClearInteractionFeedback();
	}

	bCardLayerInteractionEnabled = bEnabled;
	UpdateVisibilityForInteractionMode();
}

TSharedRef<SWidget> UWacomFirstPersonCardLayerSlotWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			TEXT("FirstPersonCardLayerSlotRoot"));
		WidgetTree->RootWidget = RootOverlay;
	}
	else
	{
		RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	}

	EnsureCardView();
	UpdateVisibilityForInteractionMode();
	return Super::RebuildWidget();
}

void UWacomFirstPersonCardLayerSlotWidget::NativeDestruct()
{
	SetHoveredForFirstPersonLayer(false);
	ClearGestureState(false);
	ClearInteractionFeedback();
	SetTickEnabledForMotion(false);
	OnCardClickedNative.Clear();
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	OnCardTargetHoveredNative.Clear();
	OnCardTargetUnhoveredNative.Clear();
	OnCardDragStartedNative.Clear();
	OnCardDragUpdatedNative.Clear();
	OnCardDragReleasedNative.Clear();
	OnCardDragCancelledNative.Clear();
	CardView = nullptr;
	RootOverlay = nullptr;
	Super::NativeDestruct();
}

void UWacomFirstPersonCardLayerSlotWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bWantsSlotMotionTick)
	{
		return;
	}
	if (!bHasVisualSlotView)
	{
		SetTickEnabledForMotion(false);
		return;
	}

	if (bIsExitingForFirstPersonLayer)
	{
		ExitMotionElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}
	if (GestureState == EWacomFirstPersonCardGestureState::Pressed)
	{
		UpdateGesture(InDeltaTime, CurrentGestureScreenPosition);
	}
	if (ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration)
	{
		ConfirmFeedbackElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}
	if (DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration)
	{
		DenyFeedbackElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}
	if (CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration)
	{
		CommitFeedbackElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}

	bool bNearTarget = true;
	const FWacomFirstPersonCardLayerSlotView PreviousVisualSlotView = VisualSlotView;
	if (!bIsExitingForFirstPersonLayer || SlotMotionConfig.bEnabled)
	{
		const float MotionAlpha = ComputeInterpAlpha(SlotMotionConfig.MotionSpeed, InDeltaTime);
		const float OpacityAlpha = ComputeInterpAlpha(SlotMotionConfig.OpacitySpeed, InDeltaTime);
		const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
		VisualSlotView = LerpSlotView(VisualSlotView, EffectiveTargetSlotView, MotionAlpha, OpacityAlpha);
		ApplyVisualSlotView();

		bNearTarget =
			FVector2D::Distance(VisualSlotView.ScreenPosition, EffectiveTargetSlotView.ScreenPosition) <= 0.1f
			&& FMath::Abs(VisualSlotView.RenderAngleDegrees - EffectiveTargetSlotView.RenderAngleDegrees) <= 0.05f
			&& FMath::Abs(VisualSlotView.RenderScale - EffectiveTargetSlotView.RenderScale) <= 0.001f
			&& FMath::Abs(VisualSlotView.RenderOpacity - EffectiveTargetSlotView.RenderOpacity) <= 0.01f;
		if (bNearTarget)
		{
			VisualSlotView = EffectiveTargetSlotView;
			ApplyVisualSlotView();
		}
	}
	else
	{
		ApplyVisualSlotView();
	}

	const bool bInspectVisualChanged =
		GestureState == EWacomFirstPersonCardGestureState::Inspecting
		&& (FVector2D::Distance(PreviousVisualSlotView.ScreenPosition, VisualSlotView.ScreenPosition) > 0.1f
			|| FMath::Abs(PreviousVisualSlotView.RenderAngleDegrees - VisualSlotView.RenderAngleDegrees) > 0.05f
			|| FMath::Abs(PreviousVisualSlotView.RenderScale - VisualSlotView.RenderScale) > 0.001f
			|| FMath::Abs(PreviousVisualSlotView.RenderOpacity - VisualSlotView.RenderOpacity) > 0.01f
			|| PreviousVisualSlotView.ZOrder != VisualSlotView.ZOrder);
	if (bInspectVisualChanged)
	{
		BroadcastDragUpdated();
	}

	if (IsExitMotionFinished())
	{
		VisualSlotView.bProjected = false;
		ApplyVisualSlotView();
		UpdateWantsTick();
		return;
	}

	if (bNearTarget && !bIsExitingForFirstPersonLayer)
	{
		UpdateWantsTick();
	}
	else
	{
		UpdateWantsTick();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (CanInteractWithCurrentSlot())
	{
		SetHoveredForFirstPersonLayer(true);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		SetPressedForFirstPersonLayer(false);
	}
	SetHoveredForFirstPersonLayer(false);
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanInteractWithCurrentSlot())
	{
		FVector2D PointerWidgetPosition = FVector2D::ZeroVector;
		if (!ResolvePointerWidgetPosition(InMouseEvent, PointerWidgetPosition))
		{
			PointerWidgetPosition = bHasVisualSlotView
				? VisualSlotView.ScreenPosition
				: CurrentSlotView.ScreenPosition;
		}
		BeginGesturePress(PointerWidgetPosition);
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanInteractWithCurrentSlot())
	{
		FVector2D PointerWidgetPosition = CurrentGestureScreenPosition;
		ResolvePointerWidgetPosition(InMouseEvent, PointerWidgetPosition);
		ReleaseGesture(PointerWidgetPosition);
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled)
	{
		FVector2D PointerWidgetPosition = CurrentGestureScreenPosition;
		ResolvePointerWidgetPosition(InMouseEvent, PointerWidgetPosition);
		UpdateGesture(0.0f, PointerWidgetPosition);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UWacomFirstPersonCardLayerSlotWidget::EnsureCardView()
{
	if (CardView)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree)
	{
		return;
	}

	if (!RootOverlay)
	{
		if (WidgetTree->RootWidget)
		{
			RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
		}
		else
		{
			RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("FirstPersonCardLayerSlotRoot"));
			WidgetTree->RootWidget = RootOverlay;
		}
	}
	if (!RootOverlay)
	{
		return;
	}

	UClass* ClassToUse = CardViewClass ? CardViewClass.Get() : UWacomCardView::StaticClass();
	CardView = WidgetTree->ConstructWidget<UWacomCardView>(ClassToUse, TEXT("CardView"));
	if (!CardView)
	{
		return;
	}

	CardView->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* CardSlot = RootOverlay->AddChildToOverlay(CardView))
	{
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
	}
	if (FeedbackOverlay)
	{
		FeedbackOverlay->RemoveFromParent();
		if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	else
	{
		EnsureFeedbackOverlay();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::EnsureFeedbackOverlay()
{
	if (FeedbackOverlay)
	{
		if (FeedbackOverlay->GetParent() != RootOverlay && RootOverlay)
		{
			FeedbackOverlay->RemoveFromParent();
			if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!RootOverlay && WidgetTree && WidgetTree->RootWidget)
	{
		RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	}
	if (!WidgetTree || !RootOverlay)
	{
		return;
	}

	FeedbackOverlay = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("InteractionFeedbackOverlay"));
	if (!FeedbackOverlay)
	{
		return;
	}

	FeedbackOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	FeedbackOverlay->SetRenderOpacity(0.0f);
	FSlateBrush FeedbackBrush;
	FeedbackBrush.DrawAs = ESlateBrushDrawType::Box;
	FeedbackOverlay->SetBrush(FeedbackBrush);
	if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(FeedbackOverlay))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCurrentSlotView()
{
	EnsureCardView();
	if (CardView)
	{
		CardView->SetCardViewData(CurrentSlotView.Entry.CardViewData);
	}
	EnsureFeedbackOverlay();
	UpdateVisibilityForInteractionMode();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyVisualSlotView()
{
	if (!bHasVisualSlotView)
	{
		return;
	}
	ApplySlotViewToWidget(VisualSlotView);
}

void UWacomFirstPersonCardLayerSlotWidget::ApplySlotViewToWidget(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(SlotView.ScreenPosition);
		CanvasSlot->SetZOrder(SlotView.ZOrder);
	}
	const bool bDenyActive = SlotFeedbackConfig.bEnabled
		&& DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
	float DenyShakeOffset = 0.0f;
	if (bDenyActive && SlotFeedbackConfig.DenyDuration > 0.0f)
	{
		const float Progress = FMath::Clamp(DenyFeedbackElapsedSeconds / SlotFeedbackConfig.DenyDuration, 0.0f, 1.0f);
		const float ShakeAlpha = 1.0f - Progress;
		DenyShakeOffset = FMath::Sin(Progress * PI * 6.0f) * SlotFeedbackConfig.DenyShakePixels * ShakeAlpha;
	}

	SetRenderOpacity(FMath::Clamp(SlotView.RenderOpacity, 0.0f, 1.0f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform CardRenderTransform;
	const float PressedScale = (SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer)
		? SlotFeedbackConfig.PressedScale
		: 1.0f;
	const bool bDragTargetFeedbackActive =
		CardDragConfig.bEnableDragTargetFeedback
		&& DragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None;
	const float CommitScale =
		SlotFeedbackConfig.bEnabled
		&& SlotFeedbackConfig.bEnablePlayCommitFeedback
		&& CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration
			? SlotFeedbackConfig.PlayCommitScale
			: 1.0f;
	const float DragReadyScale =
		CardDragConfig.bEnableDragTargetFeedback
		&& DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CommitReady
			? CardDragConfig.DragCommitReadyScale
			: 1.0f;
	const float CardProbeScale =
		CardDragConfig.bEnableDragTargetFeedback
		&& bCardDragProbeFeedback
		&& DragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget
			? CardDragConfig.DragCardTargetProbeScale
			: 1.0f;
	CardRenderTransform.Translation = FVector2D(DenyShakeOffset, 0.0f);
	CardRenderTransform.Scale = FVector2D(FMath::Max(
		0.01f,
		SlotView.RenderScale
			* (bDragTargetFeedbackActive ? 1.0f : PressedScale)
			* CommitScale
			* DragReadyScale
			* CardProbeScale));
	CardRenderTransform.Angle = SlotView.RenderAngleDegrees;
	SetRenderTransform(CardRenderTransform);
	ApplyFeedbackOverlay();
	UpdateVisibilityForInteractionMode();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& CurrentSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanApplyPlayableHoverFeedback() const
{
	return CanClickCurrentSlot()
		&& !CurrentSlotView.Entry.bIsPendingTargeting
		&& bIsHoveredForFirstPersonLayer;
}

bool UWacomFirstPersonCardLayerSlotWidget::CanClickCurrentSlot() const
{
	return CanInteractWithCurrentSlot() && CurrentSlotView.Entry.bIsPlayable;
}

bool UWacomFirstPersonCardLayerSlotWidget::CanStartCardDragGesture() const
{
	return CardDragConfig.bEnableFirstPersonCardDragCommit
		&& CanInteractWithCurrentSlot()
		&& CurrentSlotView.Entry.bIsPlayable;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsNoTargetDragCard() const
{
	switch (CurrentSlotView.Entry.TargetMode)
	{
	case ECardTargetMode::None:
	case ECardTargetMode::Self:
	case ECardTargetMode::AllEnemyParts:
		return true;
	default:
		return false;
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsTargetedAimCard() const
{
	return CurrentSlotView.Entry.TargetMode == ECardTargetMode::SingleEnemyPart
		|| CurrentSlotView.Entry.TargetMode == ECardTargetMode::HandCard;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::BuildInspectOverrideSlotView() const
{
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView = GetGestureBaseSlotView();
	FWacomFirstPersonCardLayerSlotView InspectSlot = BaseSlotView;
	FVector2D InspectPosition = InspectSlot.ScreenPosition;
	if (ResolveInspectScreenPosition(InspectPosition))
	{
		InspectSlot.ScreenPosition = InspectPosition;
		InspectSlot.WidgetPosition = InspectPosition;
		InspectSlot.SnappedWidgetPosition = InspectPosition;
	}
	InspectSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.CardInspectScale);
	InspectSlot.RenderAngleDegrees = 0.0f;
	InspectSlot.ZOrder = BaseSlotView.ZOrder + 1400;
	InspectSlot.GestureState = GestureState;
	return InspectSlot;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::BuildNoTargetDragOverrideSlotView() const
{
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView = GetGestureBaseSlotView();
	FWacomFirstPersonCardLayerSlotView DragSlot = BaseSlotView;
	const FVector2D DragDelta = CurrentGestureScreenPosition - PressScreenPosition;
	DragSlot.ScreenPosition = BaseSlotView.ScreenPosition + DragDelta;
	DragSlot.WidgetPosition = DragSlot.ScreenPosition;
	DragSlot.SnappedWidgetPosition = DragSlot.ScreenPosition;
	DragSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.CardInspectScale);
	DragSlot.RenderAngleDegrees = FMath::Lerp(BaseSlotView.RenderAngleDegrees, 0.0f, 0.65f);
	DragSlot.ZOrder = BaseSlotView.ZOrder + 1400;
	DragSlot.GestureState = GestureState;
	return DragSlot;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::BuildAimOverrideSlotView() const
{
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView = GetGestureBaseSlotView();
	FWacomFirstPersonCardLayerSlotView AimSlot = BaseSlotView;
	AimSlot.ScreenPosition = BaseSlotView.ScreenPosition + FVector2D(0.0f, -CardDragConfig.SelectedSourceLiftPixels);
	AimSlot.WidgetPosition = AimSlot.ScreenPosition;
	AimSlot.SnappedWidgetPosition = AimSlot.ScreenPosition;
	AimSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.SelectedSourceScale);
	if (CardDragConfig.bSelectedSourceStraightenAngle)
	{
		AimSlot.RenderAngleDegrees = FMath::Lerp(
			BaseSlotView.RenderAngleDegrees,
			0.0f,
			CardDragConfig.SelectedSourceAngleBlend);
	}
	AimSlot.ZOrder = BaseSlotView.ZOrder + CardDragConfig.SelectedSourceZOrderBoost;
	AimSlot.GestureState = GestureState;
	return AimSlot;
}

const FWacomFirstPersonCardLayerSlotView& UWacomFirstPersonCardLayerSlotWidget::GetGestureBaseSlotView() const
{
	return GestureStartSlotView.IsSet()
		? GestureStartSlotView.GetValue()
		: TargetSlotView;
}

const FWacomFirstPersonCardLayerSlotView& UWacomFirstPersonCardLayerSlotWidget::GetEffectiveTargetSlotView() const
{
	return GestureOverrideTargetSlotView.IsSet()
		? GestureOverrideTargetSlotView.GetValue()
		: TargetSlotView;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveInspectScreenPosition(FVector2D& OutScreenPosition) const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D ViewportSize = FVector2D::ZeroVector;
			ViewportClient->GetViewportSize(ViewportSize);
			if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
			{
				const APlayerController* PC = GetOwningPlayer();
				const float ViewportScale = PC
					? FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(PC))
					: 1.0f;
				ViewportSize /= ViewportScale;
				OutScreenPosition = FVector2D(
					ViewportSize.X * CardDragConfig.CardInspectScreenPosition.X,
					ViewportSize.Y * CardDragConfig.CardInspectScreenPosition.Y);
				return true;
			}
		}
	}

	if (!TargetSlotView.AnchorWidgetPosition.IsNearlyZero())
	{
		OutScreenPosition = TargetSlotView.AnchorWidgetPosition + FVector2D(0.0f, -96.0f);
		return true;
	}
	return false;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolvePointerWidgetPosition(
	const FPointerEvent& InMouseEvent,
	FVector2D& OutScreenPosition) const
{
	FVector2D PixelPosition = FVector2D::ZeroVector;
	FVector2D ViewportPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		InMouseEvent.GetScreenSpacePosition(),
		PixelPosition,
		ViewportPosition);

	if (ViewportPosition.ContainsNaN())
	{
		return false;
	}

	OutScreenPosition = ViewportPosition;
	const_cast<UWacomFirstPersonCardLayerSlotWidget*>(this)->UpdatePointerViewportDiagnostics(ViewportPosition);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::BeginGesturePress(const FVector2D& ScreenPosition)
{
	if (!CanInteractWithCurrentSlot())
	{
		return;
	}

	ClearGestureState(false);
	GestureStartSlotView = bHasVisualSlotView ? VisualSlotView : TargetSlotView;
	PressScreenPosition = ScreenPosition;
	CurrentGestureScreenPosition = ScreenPosition;
	UpdatePointerViewportDiagnostics(ScreenPosition);
	GestureElapsedSeconds = 0.0f;
	bGestureTargetValid = false;
	bGestureCommitArmed = false;
	bSuppressClickOnRelease = false;
	GestureFeedbackTargetHandle = FWacomInteractionTargetHandle();
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	GestureState = EWacomFirstPersonCardGestureState::Pressed;
	SetPressedForFirstPersonLayer(true);
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGesture(float DeltaTime, const FVector2D& ScreenPosition)
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return;
	}

	CurrentGestureScreenPosition = ScreenPosition;
	UpdatePointerViewportDiagnostics(ScreenPosition);
	GestureElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float DragDistance = FVector2D::Distance(CurrentGestureScreenPosition, PressScreenPosition);

	if (GestureState == EWacomFirstPersonCardGestureState::Pressed)
	{
		if (CanStartCardDragGesture()
			&& DragDistance >= CardDragConfig.CardDragStartThresholdPixels)
		{
			if (IsNoTargetDragCard())
			{
				SetGestureState(EWacomFirstPersonCardGestureState::DraggingNoTargetCard, true);
			}
			else if (IsTargetedAimCard())
			{
				SetGestureState(EWacomFirstPersonCardGestureState::AimingTargetedCard, true);
			}
			else
			{
				SetGestureState(EWacomFirstPersonCardGestureState::Cancelled, true);
			}
		}
		else if (CanStartCardDragGesture()
			&& GestureElapsedSeconds >= CardDragConfig.CardInspectHoldDelaySeconds)
		{
			SetGestureState(EWacomFirstPersonCardGestureState::Inspecting, true);
		}
	}

	if (GestureState == EWacomFirstPersonCardGestureState::Inspecting
		&& DragDistance >= CardDragConfig.CardDragStartThresholdPixels)
	{
		if (IsNoTargetDragCard())
		{
			SetGestureState(EWacomFirstPersonCardGestureState::DraggingNoTargetCard, true);
		}
		else if (IsTargetedAimCard())
		{
			SetGestureState(EWacomFirstPersonCardGestureState::AimingTargetedCard, true);
		}
	}

	if (GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit)
	{
		const bool bNowArmed =
			ComputeNoTargetDragOutDistance() >= CardDragConfig.NoTargetCardDragOutCommitDistancePixels;
		SetGestureState(
			bNowArmed
				? EWacomFirstPersonCardGestureState::ArmedForCommit
				: EWacomFirstPersonCardGestureState::DraggingNoTargetCard,
			false);
		DragTargetFeedbackState =
			CardDragConfig.bEnableDragTargetFeedback && bNowArmed
				? EWacomFirstPersonCardDragTargetFeedbackState::CommitReady
				: EWacomFirstPersonCardDragTargetFeedbackState::None;
		ApplyVisualSlotView();
	}

	UpdateGestureOverrideTarget();
	BroadcastDragUpdated();
	UpdateWantsTick();
}

bool UWacomFirstPersonCardLayerSlotWidget::ReleaseGesture(const FVector2D& ScreenPosition)
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return false;
	}

	CurrentGestureScreenPosition = ScreenPosition;
	UpdateGesture(0.0f, ScreenPosition);

	const EWacomFirstPersonCardGestureState ReleaseState = GestureState;
	SetPressedForFirstPersonLayer(false);
	BroadcastDragReleased();

	const bool bAllowQuickClick =
		!bSuppressClickOnRelease
		&& CardDragConfig.bEnableClickToPlayCard
		&& ReleaseState == EWacomFirstPersonCardGestureState::Pressed
		&& GestureElapsedSeconds < CardDragConfig.CardInspectHoldDelaySeconds;

	if (bAllowQuickClick)
	{
		if (CanClickCurrentSlot())
		{
			TriggerConfirmFeedback();
			OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
		}
		else
		{
			TriggerDenyFeedback();
		}
	}
	else if (ReleaseState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| (ReleaseState == EWacomFirstPersonCardGestureState::AimingTargetedCard && bGestureTargetValid))
	{
		TriggerConfirmFeedback();
	}
	else if (ReleaseState == EWacomFirstPersonCardGestureState::Inspecting)
	{
		// Releasing read/inspect mode is a neutral return to the hand, not a rejected play attempt.
	}
	else if (ReleaseState != EWacomFirstPersonCardGestureState::Pressed)
	{
		TriggerDenyFeedback();
	}

	ClearGestureState(false);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::SetGestureState(
	EWacomFirstPersonCardGestureState NewState,
	bool bBroadcastStartOrCancel)
{
	if (GestureState == NewState)
	{
		return;
	}

	const EWacomFirstPersonCardGestureState PreviousState = GestureState;
	GestureState = NewState;
	bSuppressClickOnRelease =
		bSuppressClickOnRelease
		|| NewState == EWacomFirstPersonCardGestureState::Inspecting
		|| NewState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| NewState == EWacomFirstPersonCardGestureState::AimingTargetedCard
		|| NewState == EWacomFirstPersonCardGestureState::ArmedForCommit;
	bGestureCommitArmed = NewState == EWacomFirstPersonCardGestureState::ArmedForCommit;
	UpdateGestureOverrideTarget();

	if (bBroadcastStartOrCancel
		&& PreviousState == EWacomFirstPersonCardGestureState::Pressed
		&& (NewState == EWacomFirstPersonCardGestureState::Inspecting
			|| NewState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| NewState == EWacomFirstPersonCardGestureState::AimingTargetedCard))
	{
		BroadcastDragStarted();
	}
	else if (bBroadcastStartOrCancel && NewState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		BroadcastDragCancelled();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGestureOverrideTarget()
{
	switch (GestureState)
	{
	case EWacomFirstPersonCardGestureState::Inspecting:
		GestureOverrideTargetSlotView = BuildInspectOverrideSlotView();
		break;
	case EWacomFirstPersonCardGestureState::DraggingNoTargetCard:
	case EWacomFirstPersonCardGestureState::ArmedForCommit:
		GestureOverrideTargetSlotView = BuildNoTargetDragOverrideSlotView();
		break;
	case EWacomFirstPersonCardGestureState::AimingTargetedCard:
		GestureOverrideTargetSlotView = BuildAimOverrideSlotView();
		break;
	default:
		GestureOverrideTargetSlotView.Reset();
		break;
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ClearGestureState(bool bBroadcastCancel)
{
	const bool bHadGesture = GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	if (bHadGesture && bBroadcastCancel)
	{
		BroadcastDragCancelled();
	}

	GestureState = EWacomFirstPersonCardGestureState::Idle;
	GestureStartSlotView.Reset();
	GestureOverrideTargetSlotView.Reset();
	GestureFeedbackTargetHandle = FWacomInteractionTargetHandle();
	DragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	ClearPointerViewportDiagnostics();
	bGestureTargetValid = false;
	bGestureCommitArmed = false;
	bSuppressClickOnRelease = false;
	GestureElapsedSeconds = 0.0f;
	SetPressedForFirstPersonLayer(false);
	UpdateWantsTick();
}

float UWacomFirstPersonCardLayerSlotWidget::ComputeNoTargetDragOutDistance() const
{
	switch (CardDragConfig.NoTargetCardDragOutDirection)
	{
	case EWacomFirstPersonCardDragOutDirection::Up:
	default:
		return FMath::Max(0.0f, PressScreenPosition.Y - CurrentGestureScreenPosition.Y);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::UpdatePointerViewportDiagnostics(const FVector2D& WidgetPosition)
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			ViewportClient->GetViewportSize(ViewportSize);
		}
	}

	float ViewportScale = 1.0f;
	if (APlayerController* PC = GetOwningPlayer())
	{
		ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(PC));
	}
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		ViewportSize = FVector2D(1920.0f, 1080.0f);
		ViewportScale = 1.0f;
	}

	const FVector2D WidgetViewportSize = ViewportSize / FMath::Max(0.01f, ViewportScale);
	if (WidgetViewportSize.X <= 0.0f || WidgetViewportSize.Y <= 0.0f)
	{
		ClearPointerViewportDiagnostics();
		return;
	}

	PointerViewportPosition = WidgetPosition;
	PointerNormalizedViewportPosition = FVector2D(
		FMath::Clamp((WidgetPosition.X / WidgetViewportSize.X) * 2.0f - 1.0f, -1.0f, 1.0f),
		FMath::Clamp((WidgetPosition.Y / WidgetViewportSize.Y) * 2.0f - 1.0f, -1.0f, 1.0f));
	bHasPointerViewportPosition = true;
}

void UWacomFirstPersonCardLayerSlotWidget::ClearPointerViewportDiagnostics()
{
	bHasPointerViewportPosition = false;
	PointerViewportPosition = FVector2D::ZeroVector;
	PointerNormalizedViewportPosition = FVector2D::ZeroVector;
}

FWacomFirstPersonCardDragView UWacomFirstPersonCardLayerSlotWidget::BuildDragView() const
{
	FWacomFirstPersonCardDragView View;
	View.GestureState = GestureState;
	View.CardInstanceId = CurrentSlotView.Entry.CardInstanceId;
	View.SourceSlotView = bHasVisualSlotView ? VisualSlotView : CurrentSlotView;
	View.SourceSlotView.GestureState = GestureState;
	View.PressScreenPosition = PressScreenPosition;
	View.CurrentScreenPosition = CurrentGestureScreenPosition;
	View.CurrentTarget = GestureFeedbackTargetHandle;
	View.bCommitArmed = bGestureCommitArmed;
	View.bTargetValid = bGestureTargetValid;
	View.bHasPointerViewportPosition = bHasPointerViewportPosition;
	View.PointerViewportPosition = PointerViewportPosition;
	View.PointerNormalizedViewportPosition = PointerNormalizedViewportPosition;
	View.TargetFeedbackState = DragTargetFeedbackState;
	View.bHasFeedbackTargetScreenPosition = bHasFeedbackTargetScreenPosition;
	View.FeedbackTargetScreenPosition = FeedbackTargetScreenPosition;
	return View;
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragStarted()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		OnCardDragStartedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragUpdated()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid()
		&& GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled)
	{
		OnCardDragUpdatedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragReleased()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		OnCardDragReleasedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastDragCancelled()
{
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		OnCardDragCancelledNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, BuildDragView());
	}
}

#if WITH_AUTOMATION_TESTS
bool UWacomFirstPersonCardLayerSlotWidget::RequestHoverForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	SetHoveredForFirstPersonLayer(true);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::RequestUnhoverForTest()
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		SetPressedForFirstPersonLayer(false);
	}
	SetHoveredForFirstPersonLayer(false);
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestPressForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	SetPressedForFirstPersonLayer(true);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestClickForTest()
{
	if (!CanClickCurrentSlot())
	{
		return false;
	}

	TriggerConfirmFeedback();
	OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestMouseUpForTest()
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	if (GestureState != EWacomFirstPersonCardGestureState::Idle)
	{
		return ReleaseGesture(CurrentGestureScreenPosition);
	}

	SetPressedForFirstPersonLayer(false);
	if (CanClickCurrentSlot())
	{
		TriggerConfirmFeedback();
		OnCardClickedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
	}
	else
	{
		TriggerDenyFeedback();
	}
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestGesturePressForTest(const FVector2D& ScreenPosition)
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	BeginGesturePress(ScreenPosition);
	return true;
}

void UWacomFirstPersonCardLayerSlotWidget::RequestGestureMoveForTest(
	float DeltaTime,
	const FVector2D& ScreenPosition)
{
	UpdateGesture(DeltaTime, ScreenPosition);
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestGestureReleaseForTest(const FVector2D& ScreenPosition)
{
	return ReleaseGesture(ScreenPosition);
}

void UWacomFirstPersonCardLayerSlotWidget::TickSlotMotionForTest(float DeltaTime)
{
	NativeTick(FGeometry(), DeltaTime);
}

float UWacomFirstPersonCardLayerSlotWidget::GetFeedbackOverlayRenderOpacityForTest() const
{
	return FeedbackOverlay ? FeedbackOverlay->GetRenderOpacity() : 0.0f;
}

FLinearColor UWacomFirstPersonCardLayerSlotWidget::GetFeedbackOverlayColorForTest() const
{
	return FeedbackOverlay ? FeedbackOverlay->GetColorAndOpacity() : FLinearColor::Transparent;
}
#endif

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredForFirstPersonLayer(bool bHovered)
{
	if (bIsHoveredForFirstPersonLayer == bHovered)
	{
		return;
	}

	bIsHoveredForFirstPersonLayer = bHovered;
	if (!bIsHoveredForFirstPersonLayer)
	{
		const bool bGestureActive =
			GestureState != EWacomFirstPersonCardGestureState::Idle
			&& GestureState != EWacomFirstPersonCardGestureState::Cancelled;
		if (!bGestureActive)
		{
			SetPressedForFirstPersonLayer(false);
		}
	}
	ApplyVisualSlotView();
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		if (bIsHoveredForFirstPersonLayer)
		{
			OnCardHoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
			if (const FWacomInteractionTargetHandle CardTargetHandle = BuildCardTargetHandle(); CardTargetHandle.IsValid())
			{
				FWacomFirstPersonCardLayerSlotView VisualTargetSlotView = VisualSlotView;
				VisualTargetSlotView.bIsHovered = true;
				OnCardTargetHoveredNative.Broadcast(CardTargetHandle, VisualTargetSlotView);
			}
		}
		else
		{
			const FWacomInteractionTargetHandle CardTargetHandle = FWacomInteractionTargetHandle::ForCardTarget(
				CurrentSlotView.Entry.CardInstanceId,
				this,
				VisualSlotView.ScreenPosition);
			OnCardUnhoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, CurrentSlotView);
			FWacomFirstPersonCardLayerSlotView VisualTargetSlotView = VisualSlotView;
			VisualTargetSlotView.bIsHovered = false;
			OnCardTargetUnhoveredNative.Broadcast(CardTargetHandle, VisualTargetSlotView);
		}
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetPressedForFirstPersonLayer(bool bPressed)
{
	if (bIsPressedForFirstPersonLayer == bPressed)
	{
		return;
	}

	bIsPressedForFirstPersonLayer = bPressed && SlotFeedbackConfig.bEnabled && CanInteractWithCurrentSlot();
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerConfirmFeedback()
{
	if (!SlotFeedbackConfig.bEnabled || SlotFeedbackConfig.ConfirmDuration <= 0.0f)
	{
		return;
	}

	ConfirmFeedbackElapsedSeconds = 0.0f;
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerDenyFeedback()
{
	if (!SlotFeedbackConfig.bEnabled || SlotFeedbackConfig.DenyDuration <= 0.0f)
	{
		return;
	}

	DenyFeedbackElapsedSeconds = 0.0f;
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCommitFeedback()
{
	if (!SlotFeedbackConfig.bEnabled
		|| !SlotFeedbackConfig.bEnablePlayCommitFeedback
		|| SlotFeedbackConfig.PlayCommitDuration <= 0.0f)
	{
		return;
	}

	CommitFeedbackElapsedSeconds = 0.0f;
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInteractionFeedback()
{
	bIsPressedForFirstPersonLayer = false;
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	DragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	ConfirmFeedbackElapsedSeconds = SlotFeedbackConfig.ConfirmDuration;
	DenyFeedbackElapsedSeconds = SlotFeedbackConfig.DenyDuration;
	CommitFeedbackElapsedSeconds = SlotFeedbackConfig.PlayCommitDuration;
	ApplyFeedbackOverlay();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyFeedbackOverlay()
{
	EnsureFeedbackOverlay();
	if (!FeedbackOverlay)
	{
		return;
	}

	FLinearColor OverlayColor = FLinearColor::Transparent;
	float OverlayOpacity = 0.0f;
	if (SlotFeedbackConfig.bEnabled)
	{
		const float DenyAlpha = ComputePulseAlpha(DenyFeedbackElapsedSeconds, SlotFeedbackConfig.DenyDuration);
		const float ConfirmAlpha = ComputePulseAlpha(ConfirmFeedbackElapsedSeconds, SlotFeedbackConfig.ConfirmDuration);
		const float CommitAlpha =
			SlotFeedbackConfig.bEnablePlayCommitFeedback
				? ComputePulseAlpha(CommitFeedbackElapsedSeconds, SlotFeedbackConfig.PlayCommitDuration)
				: 0.0f;
		if (DenyAlpha > 0.0f)
		{
			OverlayColor = SlotFeedbackConfig.DenyColor;
			OverlayOpacity = SlotFeedbackConfig.DenyOpacity * DenyAlpha;
		}
		else if (bIsPressedForFirstPersonLayer)
		{
			OverlayColor = SlotFeedbackConfig.PressedColor;
			OverlayOpacity = SlotFeedbackConfig.PressedOpacity;
		}
		else if (CommitAlpha > 0.0f)
		{
			OverlayColor = SlotFeedbackConfig.PlayCommitColor;
			OverlayOpacity = SlotFeedbackConfig.PlayCommitOpacity * CommitAlpha;
		}
		else if (ConfirmAlpha > 0.0f)
		{
			OverlayColor = SlotFeedbackConfig.PressedColor;
			OverlayOpacity = SlotFeedbackConfig.ConfirmOpacity * ConfirmAlpha;
		}
		else if (CanApplyPlayableHoverFeedback())
		{
			OverlayColor = SlotFeedbackConfig.PlayableHoverColor;
			OverlayOpacity = SlotFeedbackConfig.PlayableHoverOpacity;
		}
	}
	if (CardDragConfig.bEnableDragTargetFeedback)
	{
		if (DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CommitReady
			|| DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
			|| DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget)
		{
			OverlayColor = CardDragConfig.DragValidTargetColor;
			OverlayOpacity = CardDragConfig.DragTargetFeedbackOpacity;
		}
		else if (DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::Invalid
			|| DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			OverlayColor = CardDragConfig.DragInvalidTargetColor;
			OverlayOpacity = DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget
				? CardDragConfig.DragTargetFeedbackOpacity * 0.6f
				: CardDragConfig.DragTargetFeedbackOpacity;
		}
		else if (bCardDragProbeFeedback && bCardDragProbeFeedbackValid)
		{
			OverlayColor = CardDragConfig.DragValidTargetColor;
			OverlayOpacity = CardDragConfig.DragTargetFeedbackOpacity;
		}
		else if (DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| DragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ZoneProbe
			|| bCardDragProbeFeedback)
		{
			OverlayColor = CardDragConfig.DragCardProbeTargetColor;
			OverlayOpacity = CardDragConfig.DragTargetFeedbackOpacity;
		}
	}

	OverlayColor.A = 1.0f;
	FeedbackOverlay->SetColorAndOpacity(OverlayColor);
	FeedbackOverlay->SetRenderOpacity(FMath::Clamp(OverlayOpacity, 0.0f, 1.0f));
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateVisibilityForInteractionMode()
{
	const bool bVisible = bHasVisualSlotView
		? VisualSlotView.bProjected
		: CurrentSlotView.bProjected;
	SetVisibility(bVisible
		? (bCardLayerInteractionEnabled ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible)
		: ESlateVisibility::Collapsed);
	if (CardView)
	{
		CardView->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (FeedbackOverlay)
	{
		FeedbackOverlay->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetTickEnabledForMotion(bool bEnabled)
{
	bWantsSlotMotionTick = bEnabled;
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateWantsTick()
{
	const bool bFeedbackActive =
		(SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer)
		|| ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration
		|| DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration
		|| (SlotFeedbackConfig.bEnablePlayCommitFeedback
			&& CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration)
		|| (CardDragConfig.bEnableDragTargetFeedback
			&& (DragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
				|| bCardDragProbeFeedback));
	const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
	const bool bGestureActive =
		GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	bWantsSlotMotionTick = bIsExitingForFirstPersonLayer
		|| (bHasVisualSlotView
			&& (FVector2D::Distance(VisualSlotView.ScreenPosition, EffectiveTargetSlotView.ScreenPosition) > 0.1f
				|| FMath::Abs(VisualSlotView.RenderAngleDegrees - EffectiveTargetSlotView.RenderAngleDegrees) > 0.05f
				|| FMath::Abs(VisualSlotView.RenderScale - EffectiveTargetSlotView.RenderScale) > 0.001f
				|| FMath::Abs(VisualSlotView.RenderOpacity - EffectiveTargetSlotView.RenderOpacity) > 0.01f))
		|| bFeedbackActive
		|| bGestureActive;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::LerpSlotView(
	const FWacomFirstPersonCardLayerSlotView& From,
	const FWacomFirstPersonCardLayerSlotView& To,
	float MotionAlpha,
	float OpacityAlpha)
{
	FWacomFirstPersonCardLayerSlotView Result = To;
	Result.ScreenPosition = FMath::Lerp(From.ScreenPosition, To.ScreenPosition, MotionAlpha);
	Result.WidgetPosition = Result.ScreenPosition;
	Result.SnappedWidgetPosition = Result.ScreenPosition;
	Result.RenderAngleDegrees = FMath::Lerp(From.RenderAngleDegrees, To.RenderAngleDegrees, MotionAlpha);
	Result.RenderScale = FMath::Lerp(From.RenderScale, To.RenderScale, MotionAlpha);
	Result.RenderOpacity = FMath::Lerp(From.RenderOpacity, To.RenderOpacity, OpacityAlpha);
	Result.bProjected = From.bProjected || To.bProjected;
	return Result;
}
