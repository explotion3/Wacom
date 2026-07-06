// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerConfigUtils.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

namespace
{
	float ComputeMotionAlpha(float Speed, float DeltaTime, float EasePower)
	{
		const float LinearAlpha = Speed <= 0.0f ? 1.0f : FMath::Clamp(DeltaTime * Speed, 0.0f, 1.0f);
		const float SafeEasePower = FMath::Max(0.1f, EasePower);
		return FMath::Pow(LinearAlpha, SafeEasePower);
	}

	float ComputeFeedbackPulseAlpha(float ElapsedSeconds, float DurationSeconds)
	{
		if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds)
		{
			return 0.0f;
		}

		return 1.0f - FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	}

	float ComputeTransitionEaseAlpha(float LinearAlpha, float EasePower)
	{
		const float ClampedAlpha = FMath::Clamp(LinearAlpha, 0.0f, 1.0f);
		const float SafeEasePower = FMath::Max(0.1f, EasePower);
		return FMath::InterpEaseInOut(0.0f, 1.0f, ClampedAlpha, SafeEasePower);
	}

	float ComputeDenyShakeOffset(float ElapsedSeconds, float DurationSeconds, float ShakePixels)
	{
		if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds || ShakePixels <= 0.0f)
		{
			return 0.0f;
		}

		const float Progress = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
		const float ShakeAlpha = 1.0f - Progress;
		return FMath::Sin(Progress * PI * 6.0f) * ShakePixels * ShakeAlpha;
	}

	bool IsCardTargetFocusFeedbackState(EWacomFirstPersonCardDragTargetFeedbackState FeedbackState)
	{
		return FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	}

	bool IsFormalDragGestureState(EWacomFirstPersonCardGestureState GestureState)
	{
		return GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
			|| GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard;
	}

	FReply BuildPointerRouteReply(
		const FWacomFirstPersonCardPointerRouteResult& RouteResult,
		const TSharedRef<SWidget>& CaptureWidget)
	{
		switch (RouteResult.Action)
		{
		case EWacomFirstPersonCardPointerRouteAction::Handled:
			return FReply::Handled();
		case EWacomFirstPersonCardPointerRouteAction::CaptureMouse:
			return FReply::Handled().CaptureMouse(CaptureWidget);
		case EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture:
			return FReply::Handled().ReleaseMouseCapture();
		case EWacomFirstPersonCardPointerRouteAction::Unhandled:
		default:
			return FReply::Unhandled();
		}
	}
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardViewClass(
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
	if (CardView)
	{
		CardView->RemoveFromParent();
		CardView = nullptr;
	}
	EnsureCardView();
	ApplyCurrentSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetOwningFirstPersonCardLayer(
	UWacomFirstPersonCardLayerWidget* InLayer)
{
	OwningFirstPersonCardLayer = InLayer;
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
	ClearEnterTransitionPlayback();

	CurrentSlotView = InSlotView;
	bHasVisualSlotView = true;
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	VisualSlotView = TargetSlotView;
	ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
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
		ClearEnterTransitionPlayback();
		ClearGestureState(true);
		ClearInteractionFeedback();
	}

	const bool bCanReuseVisual =
		bHasVisualSlotView
		&& !bTreatAsNewSlot
		&& CurrentSlotView.Entry.CardInstanceId == InTargetSlotView.Entry.CardInstanceId;
	const FWacomFirstPersonCardLayerSlotView PreviousBaseSlotView = CurrentSlotView;
	const FWacomFirstPersonCardLayerSlotView PreviousPresentationSlotView = TargetSlotView;
	const FWacomFirstPersonCardLayerSlotView IncomingPresentationSlotView =
		ComposePresentationSlotView(InTargetSlotView);

	CurrentSlotView = InTargetSlotView;
	TargetSlotView = IncomingPresentationSlotView;
	ActiveMotionIntent = bTreatAsNewSlot
		? EWacomFirstPersonCardMotionIntent::Enter
		: ResolveMotionIntentForPresentationChange(
			PreviousBaseSlotView,
			InTargetSlotView,
			PreviousPresentationSlotView,
			IncomingPresentationSlotView,
			EWacomFirstPersonCardMotionIntent::Layout);
	bIsExitingForFirstPersonLayer = false;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();

	if (!bCanReuseVisual)
	{
		bPreserveGestureReturnMotion = false;
		VisualSlotView = TargetSlotView;
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
			VisualSlotView.ScreenPosition = TargetSlotView.ScreenPosition + EnterProfile.OffsetPixels;
			VisualSlotView.WidgetPosition = VisualSlotView.ScreenPosition;
			VisualSlotView.SnappedWidgetPosition = VisualSlotView.ScreenPosition;
			VisualSlotView.RenderScale =
				FMath::Max(0.01f, TargetSlotView.RenderScale * FMath::Max(0.01f, EnterProfile.ScaleMultiplier));
			VisualSlotView.RenderAngleDegrees =
				TargetSlotView.RenderAngleDegrees + EnterProfile.AngleOffsetDegrees;
			VisualSlotView.RenderOpacity = FMath::Clamp(SlotMotionConfig.EnterOpacity, 0.0f, 1.0f);
			if (EnterProfile.DurationSeconds > 0.0f
				|| EnterProfile.StartDelaySeconds > 0.0f
				|| EnterProfile.ArcLiftPixels > 0.0f)
			{
				StartEnterTransitionPlayback(VisualSlotView, EnterProfile);
			}
			else
			{
				ClearEnterTransitionPlayback();
			}
		}
		else
		{
			ClearEnterTransitionPlayback();
		}
		bHasVisualSlotView = true;
	}
	else if (!VisualSlotView.bProjected && TargetSlotView.bProjected)
	{
		VisualSlotView = TargetSlotView;
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
		ClearEnterTransitionPlayback();
		bIsExitingForFirstPersonLayer = true;
		ExitMotionElapsedSeconds = SlotMotionConfig.ExitDuration;
		SetVisibility(ESlateVisibility::Collapsed);
		SetTickEnabledForMotion(false);
		return;
	}

	SetHoveredForFirstPersonLayer(false);
	ClearGestureState(true);
	ClearInteractionFeedback();
	ClearEnterTransitionPlayback();
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
	ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Exit;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();
	ApplyVisualSlotView();
	SetTickEnabledForMotion(true);
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotMotionConfig(
	const FWacomFirstPersonCardSlotMotionConfig& InConfig)
{
	const FWacomFirstPersonCardSlotMotionConfig NewConfig = NormalizeSlotMotionConfig(InConfig);
	if (AreSlotMotionConfigsEquivalent(SlotMotionConfig, NewConfig))
	{
		return;
	}

	SlotMotionConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++SlotMotionConfigApplyCountForTest;
#endif
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	if (!SlotMotionConfig.bEnabled && bHasVisualSlotView)
	{
		ClearEnterTransitionPlayback();
		VisualSlotView = TargetSlotView;
		ApplyVisualSlotView();
	}
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotVisualConfig(
	const FWacomFirstPersonCardSlotVisualConfig& InConfig)
{
	const FWacomFirstPersonCardSlotVisualConfig NewConfig = NormalizeSlotVisualConfig(InConfig);
	if (AreSlotVisualConfigsEquivalent(SlotVisualConfig, NewConfig))
	{
		return;
	}

	SlotVisualConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++SlotVisualConfigApplyCountForTest;
#endif
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotFeedbackConfig(
	const FWacomFirstPersonCardSlotFeedbackConfig& InConfig)
{
	const FWacomFirstPersonCardSlotFeedbackConfig NewConfig = NormalizeSlotFeedbackConfig(InConfig);
	if (AreSlotFeedbackConfigsEquivalent(SlotFeedbackConfig, NewConfig))
	{
		return;
	}

	SlotFeedbackConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++SlotFeedbackConfigApplyCountForTest;
#endif
	if (!SlotFeedbackConfig.bEnabled)
	{
		ClearInteractionFeedback();
	}
	else if (!SlotFeedbackConfig.bEnableRetainedFeedback
		|| SlotFeedbackConfig.RetainedFeedbackDuration <= 0.0f)
	{
		RetainedFeedbackElapsedSeconds =
			RetainedFeedbackStartDelaySeconds + SlotFeedbackConfig.RetainedFeedbackDuration;
	}
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragConfig(
	const FWacomFirstPersonCardDragConfig& InConfig)
{
	const FWacomFirstPersonCardDragConfig NewConfig = NormalizeCardDragConfig(InConfig);
	if (AreCardDragConfigsEquivalent(CardDragConfig, NewConfig))
	{
		return;
	}

	CardDragConfig = NewConfig;
#if WITH_AUTOMATION_TESTS
	++CardDragConfigApplyCountForTest;
#endif
	if (!CardDragConfig.bEnableFirstPersonCardDragCommit)
	{
		ClearGestureState(true);
	}
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
	UpdateWantsTick();
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
	DirectDragTargetFeedbackState = CardDragConfig.bEnableDragTargetFeedback
		? FeedbackState
		: EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
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
	SetCardDragTargetFocusFeedback(
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

	if (CardDragTargetAffordanceFeedbackState == NextState
		&& bCardDragTargetAffordanceFeedback == bEnableFeedback
		&& bCardDragTargetAffordanceFeedbackValid == bValidFeedback)
	{
		return;
	}

	CardDragTargetAffordanceFeedbackState = NextState;
	bCardDragTargetAffordanceFeedback = bEnableFeedback;
	bCardDragTargetAffordanceFeedbackValid = bValidFeedback;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDragTargetFocusFeedback(
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

	if (CardDragTargetFocusFeedbackState == NextState
		&& bCardDragProbeFeedback == bEnableFeedback
		&& bCardDragProbeFeedbackValid == bValidFeedback)
	{
		return;
	}

	CardDragTargetFocusFeedbackState = NextState;
	bCardDragProbeFeedback = bEnableFeedback;
	bCardDragProbeFeedbackValid = bValidFeedback;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::DragTargetFocus);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearCardDragTargetFeedback()
{
	const bool bHadFocusFeedback =
		CardDragTargetFocusFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| bCardDragProbeFeedback;
	const bool bHadFeedback =
		DragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| CardDragTargetAffordanceFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| CardDragTargetFocusFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| DirectDragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| bCardDragTargetAffordanceFeedback
		|| bCardDragProbeFeedback;
	CardDragTargetAffordanceFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	CardDragTargetFocusFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DirectDragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	bCardDragTargetAffordanceFeedback = false;
	bCardDragTargetAffordanceFeedbackValid = false;
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	if (bHadFeedback)
	{
		RefreshPresentationTarget(
			true,
			bHadFocusFeedback
				? EWacomFirstPersonCardMotionIntent::DragTargetFocus
				: EWacomFirstPersonCardMotionIntent::Layout);
		ApplyVisualSlotView();
		UpdateWantsTick();
	}
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

bool UWacomFirstPersonCardLayerSlotWidget::CanUpdateGestureFromSlotPointer() const
{
	return GestureInputSource == EWacomFirstPersonCardGestureInputSource::MousePointer
		&& (GestureState == EWacomFirstPersonCardGestureState::Pressed
			|| GestureState == EWacomFirstPersonCardGestureState::Inspecting
			|| IsFormalDragGestureState(GestureState));
}

bool UWacomFirstPersonCardLayerSlotWidget::CanUpdateGestureFromExternalPointer() const
{
	return IsFormalDragGestureState(GestureState)
		&& GestureInputSource == EWacomFirstPersonCardGestureInputSource::ExternalPointer;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsInspectScrubActiveForFirstPersonLayer() const
{
	return GestureState == EWacomFirstPersonCardGestureState::Inspecting
		&& GestureSource == EWacomFirstPersonCardGestureSource::MousePress
		&& GestureInputSource == EWacomFirstPersonCardGestureInputSource::MousePointer;
}

bool UWacomFirstPersonCardLayerSlotWidget::CanBeginInspectScrubFromFirstPersonLayer() const
{
	return CanStartCardDragGesture();
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

FVector2D UWacomFirstPersonCardLayerSlotWidget::GetCardBodyHitSizeForFirstPersonLayer() const
{
	return CardView
		? CardView->GetCardBodyHitSize()
		: UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsWidgetPositionInsideCardBodyForFirstPersonLayer(
	const FVector2D& WidgetPosition) const
{
	if (WidgetPosition.ContainsNaN())
	{
		return false;
	}

	FVector2D BodySize = GetCardBodyHitSizeForFirstPersonLayer();
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		BodySize = UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
	}
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView& HitSlotView = bHasVisualSlotView
		? VisualSlotView
		: CurrentSlotView;
	const float RenderScale = FMath::Max(0.01f, HitSlotView.RenderScale);
	FVector2D LocalDelta = (WidgetPosition - HitSlotView.ScreenPosition) / RenderScale;
	const float InverseAngleRadians = FMath::DegreesToRadians(-HitSlotView.RenderAngleDegrees);
	const float CosAngle = FMath::Cos(InverseAngleRadians);
	const float SinAngle = FMath::Sin(InverseAngleRadians);
	LocalDelta = FVector2D(
		LocalDelta.X * CosAngle - LocalDelta.Y * SinAngle,
		LocalDelta.X * SinAngle + LocalDelta.Y * CosAngle);

	const FVector2D HalfBodySize = BodySize * 0.5f;
	return FMath::Abs(LocalDelta.X) <= HalfBodySize.X
		&& FMath::Abs(LocalDelta.Y) <= HalfBodySize.Y;
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
	OnCardHoveredNative.Clear();
	OnCardUnhoveredNative.Clear();
	OnCardVisualSlotUpdatedNative.Clear();
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
	if (IsRetainedFeedbackActive())
	{
		RetainedFeedbackElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}

	bool bNearTarget = true;
	const FWacomFirstPersonCardLayerSlotView PreviousVisualSlotView = VisualSlotView;
	if (IsEnterTransitionPlaybackActive())
	{
		bNearTarget = TickEnterTransitionPlayback(InDeltaTime);
	}
	else if (!bIsExitingForFirstPersonLayer || SlotMotionConfig.bEnabled)
	{
		const FWacomFirstPersonCardMotionProfile& ActiveMotionProfile =
			GetMotionProfileForIntent(ActiveMotionIntent);
		const float MotionAlpha = ComputeMotionAlpha(
			ActiveMotionProfile.MotionSpeed,
			InDeltaTime,
			ActiveMotionProfile.EasePower);
		const float OpacityAlpha = ComputeMotionAlpha(
			ActiveMotionProfile.OpacitySpeed,
			InDeltaTime,
			ActiveMotionProfile.EasePower);
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
			if (GestureState == EWacomFirstPersonCardGestureState::Idle
				|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
			{
				bPreserveGestureReturnMotion = false;
			}
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
	BroadcastVisualSlotUpdatedIfNeeded(PreviousVisualSlotView, VisualSlotView);
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
	{
		Layer->HandleSlotPointerEntered(*this, InMouseEvent.GetScreenSpacePosition());
	}
	else
	{
		UpdateBodyHoverFromScreenPosition(InMouseEvent.GetScreenSpacePosition());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		SetPressedForFirstPersonLayer(false);
	}
	if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
	{
		Layer->HandleSlotPointerLeft(*this, InMouseEvent.GetScreenSpacePosition());
	}
	else
	{
		SetHoveredForFirstPersonLayer(false);
	}
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
		{
			const FWacomFirstPersonCardPointerRouteResult RouteResult =
				Layer->HandleSlotPointerPressed(*this, InMouseEvent.GetScreenSpacePosition());
			return RouteResult.IsHandled()
				? BuildPointerRouteReply(RouteResult, TakeWidget())
				: Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
		{
			const FWacomFirstPersonCardPointerRouteResult RouteResult =
				Layer->HandleSlotPointerReleased(*this, InMouseEvent.GetScreenSpacePosition());
			return RouteResult.IsHandled()
				? BuildPointerRouteReply(RouteResult, TakeWidget())
				: Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWacomFirstPersonCardLayerSlotWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (UWacomFirstPersonCardLayerWidget* Layer = OwningFirstPersonCardLayer.Get())
	{
		const FWacomFirstPersonCardPointerRouteResult RouteResult =
			Layer->HandleSlotPointerMoved(*this, InMouseEvent.GetScreenSpacePosition());
		return RouteResult.IsHandled()
			? BuildPointerRouteReply(RouteResult, TakeWidget())
			: Super::NativeOnMouseMove(InGeometry, InMouseEvent);
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

	UClass* ClassToUse = CardViewClass
		? CardViewClass.Get()
		: UWacomFirstPersonCardViewWidget::StaticClass();
	CardView = WidgetTree->ConstructWidget<UWacomFirstPersonCardViewWidget>(ClassToUse, TEXT("CardView"));
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
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCurrentSlotView()
{
	EnsureCardView();
	if (CardView)
	{
		CardView->SetCardViewData(CurrentSlotView.Entry.CardViewData);
	}
	UpdateVisibilityForInteractionMode();
}

UWacomCardView* UWacomFirstPersonCardLayerSlotWidget::GetInnerCardView() const
{
	return CardView ? CardView->GetInnerCardView() : nullptr;
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyVisualSlotView()
{
	if (!bHasVisualSlotView)
	{
		return;
	}
	ApplySlotViewToWidget(VisualSlotView);
}

EWacomFirstPersonCardDragTargetFeedbackState
UWacomFirstPersonCardLayerSlotWidget::ResolveEffectiveDragTargetFeedbackState() const
{
	if (CardDragTargetFocusFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None)
	{
		return CardDragTargetFocusFeedbackState;
	}
	if (CardDragTargetAffordanceFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None)
	{
		return CardDragTargetAffordanceFeedbackState;
	}
	return DirectDragTargetFeedbackState;
}

void UWacomFirstPersonCardLayerSlotWidget::RefreshPresentationTarget(
	bool bSnapVisualWhenMotionDisabled,
	EWacomFirstPersonCardMotionIntent PreferredIntent)
{
	const FWacomFirstPersonCardLayerSlotView PreviousBaseSlotView = CurrentSlotView;
	const FWacomFirstPersonCardLayerSlotView PreviousPresentationSlotView = TargetSlotView;
	TargetSlotView = ComposePresentationSlotView(CurrentSlotView);
	ActiveMotionIntent = ResolveMotionIntentForPresentationChange(
		PreviousBaseSlotView,
		CurrentSlotView,
		PreviousPresentationSlotView,
		TargetSlotView,
		PreferredIntent);
	if (bSnapVisualWhenMotionDisabled && !SlotMotionConfig.bEnabled && bHasVisualSlotView)
	{
		VisualSlotView = TargetSlotView;
	}
}

FWacomFirstPersonCardSlotVisualState UWacomFirstPersonCardLayerSlotWidget::ResolveVisualState(
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView) const
{
	FWacomFirstPersonCardSlotVisualState State;
	State.bPendingSource = BaseSlotView.Entry.bIsPendingTargeting;
	State.bCardDragTargetFocusActive =
		CardDragConfig.bEnableDragTargetFeedback
		&& bCardDragProbeFeedback
		&& IsCardTargetFocusFeedbackState(CardDragTargetFocusFeedbackState);
	State.bTargetSelectDeemphasized =
		BaseSlotView.bHasPendingTargetingCardInHand
		&& !State.bPendingSource
		&& SlotVisualConfig.bEnableTargetSelectHandDeemphasis;
	State.bHovered =
		BaseSlotView.bIsHovered
		&& BaseSlotView.Entry.bIsPlayable
		&& !State.bPendingSource
		&& !State.bCardDragTargetFocusActive;
	return State;
}

int32 UWacomFirstPersonCardLayerSlotWidget::GetMotionIntentPriority(EWacomFirstPersonCardMotionIntent Intent)
{
	switch (Intent)
	{
	case EWacomFirstPersonCardMotionIntent::Exit:
		return 60;
	case EWacomFirstPersonCardMotionIntent::Enter:
		return 50;
	case EWacomFirstPersonCardMotionIntent::DragTargetFocus:
		return 40;
	case EWacomFirstPersonCardMotionIntent::Pending:
		return 30;
	case EWacomFirstPersonCardMotionIntent::Hover:
		return 20;
	case EWacomFirstPersonCardMotionIntent::Layout:
	default:
		return 10;
	}
}

const FWacomFirstPersonCardMotionProfile& UWacomFirstPersonCardLayerSlotWidget::GetMotionProfileForIntent(
	EWacomFirstPersonCardMotionIntent Intent) const
{
	switch (Intent)
	{
	case EWacomFirstPersonCardMotionIntent::Hover:
		return SlotMotionConfig.HoverMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Pending:
		return SlotMotionConfig.PendingMotionProfile;
	case EWacomFirstPersonCardMotionIntent::DragTargetFocus:
		return SlotMotionConfig.DragTargetFocusMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Enter:
		return SlotMotionConfig.EnterMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Exit:
		return SlotMotionConfig.ExitMotionProfile;
	case EWacomFirstPersonCardMotionIntent::Layout:
	default:
		return SlotMotionConfig.LayoutMotionProfile;
	}
}

EWacomFirstPersonCardMotionIntent UWacomFirstPersonCardLayerSlotWidget::ResolveMotionIntentForPresentationChange(
	const FWacomFirstPersonCardLayerSlotView& PreviousBaseSlotView,
	const FWacomFirstPersonCardLayerSlotView& NewBaseSlotView,
	const FWacomFirstPersonCardLayerSlotView& PreviousPresentationSlotView,
	const FWacomFirstPersonCardLayerSlotView& NewPresentationSlotView,
	EWacomFirstPersonCardMotionIntent PreferredIntent) const
{
	EWacomFirstPersonCardMotionIntent Intent = PreferredIntent;
	const auto ChooseHigherPriorityIntent = [&Intent](EWacomFirstPersonCardMotionIntent Candidate)
	{
		if (GetMotionIntentPriority(Candidate) > GetMotionIntentPriority(Intent))
		{
			Intent = Candidate;
		}
	};

	const FWacomFirstPersonCardSlotVisualState PreviousState = ResolveVisualState(PreviousBaseSlotView);
	const FWacomFirstPersonCardSlotVisualState NewState = ResolveVisualState(NewBaseSlotView);
	if (PreviousState.bCardDragTargetFocusActive != NewState.bCardDragTargetFocusActive)
	{
		ChooseHigherPriorityIntent(EWacomFirstPersonCardMotionIntent::DragTargetFocus);
	}
	if (PreviousState.bPendingSource != NewState.bPendingSource
		|| PreviousState.bTargetSelectDeemphasized != NewState.bTargetSelectDeemphasized)
	{
		ChooseHigherPriorityIntent(EWacomFirstPersonCardMotionIntent::Pending);
	}
	if (PreviousState.bHovered != NewState.bHovered)
	{
		ChooseHigherPriorityIntent(EWacomFirstPersonCardMotionIntent::Hover);
	}

	const bool bPresentationChanged =
		FVector2D::Distance(PreviousPresentationSlotView.ScreenPosition, NewPresentationSlotView.ScreenPosition) > 0.1f
		|| FMath::Abs(PreviousPresentationSlotView.RenderAngleDegrees - NewPresentationSlotView.RenderAngleDegrees) > 0.05f
		|| FMath::Abs(PreviousPresentationSlotView.RenderScale - NewPresentationSlotView.RenderScale) > 0.001f
		|| FMath::Abs(PreviousPresentationSlotView.RenderOpacity - NewPresentationSlotView.RenderOpacity) > 0.01f
		|| PreviousPresentationSlotView.ZOrder != NewPresentationSlotView.ZOrder;
	if (!bPresentationChanged)
	{
		return PreferredIntent;
	}

	return Intent;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::ComposePresentationSlotView(
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView) const
{
	FWacomFirstPersonCardLayerSlotView Presentation = BaseSlotView;
	if (!BaseSlotView.bProjected)
	{
		return Presentation;
	}

	const FWacomFirstPersonCardSlotVisualState State = ResolveVisualState(BaseSlotView);
	if (State.bPendingSource)
	{
		Presentation.ScreenPosition.Y -= SlotVisualConfig.PendingTargetingLiftPixels;
		Presentation.WidgetPosition = Presentation.ScreenPosition;
		Presentation.SnappedWidgetPosition = Presentation.ScreenPosition;
		Presentation.RenderScale =
			FMath::Max(0.01f, Presentation.RenderScale * SlotVisualConfig.PendingTargetingScale);
		if (SlotVisualConfig.bPendingTargetingStraightenAngle)
		{
			Presentation.RenderAngleDegrees = FMath::Lerp(
				Presentation.RenderAngleDegrees,
				0.0f,
				SlotVisualConfig.PendingTargetingAngleBlend);
		}
		Presentation.ZOrder += SlotVisualConfig.PendingTargetingZOrderBoost;
	}
	else if (State.bTargetSelectDeemphasized)
	{
		Presentation.RenderOpacity = FMath::Clamp(
			Presentation.RenderOpacity * SlotVisualConfig.TargetSelectNonPendingOpacityMultiplier,
			0.0f,
			1.0f);
	}

	if (State.bHovered)
	{
		Presentation.ScreenPosition.Y -= SlotVisualConfig.HoverLiftPixels;
		Presentation.WidgetPosition = Presentation.ScreenPosition;
		Presentation.SnappedWidgetPosition = Presentation.ScreenPosition;
		Presentation.RenderScale = FMath::Max(0.01f, Presentation.RenderScale * SlotVisualConfig.HoverScale);
		Presentation.ZOrder += SlotVisualConfig.HoverZOrderBoost;
	}

	if (State.bCardDragTargetFocusActive)
	{
		Presentation.ScreenPosition.Y -= SlotVisualConfig.DragCardTargetFocusLiftPixels;
		Presentation.WidgetPosition = Presentation.ScreenPosition;
		Presentation.SnappedWidgetPosition = Presentation.ScreenPosition;
		Presentation.RenderScale =
			FMath::Max(0.01f, Presentation.RenderScale * SlotVisualConfig.DragCardTargetFocusScale);
		Presentation.ZOrder += SlotVisualConfig.DragCardTargetFocusZOrderBoost;
	}

	return Presentation;
}

void UWacomFirstPersonCardLayerSlotWidget::ApplySlotViewToWidget(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	const EWacomFirstPersonCardDragTargetFeedbackState EffectiveDragTargetFeedbackState =
		ResolveEffectiveDragTargetFeedbackState();
	const bool bDragTargetFeedbackActive =
		CardDragConfig.bEnableDragTargetFeedback
		&& EffectiveDragTargetFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None;
	const float RetainedAlpha = ComputeRetainedFeedbackAlpha();
	const bool bRetainedTransformActive = RetainedAlpha > 0.0f && !bDragTargetFeedbackActive;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(SlotView.ScreenPosition);
		CanvasSlot->SetZOrder(
			SlotView.ZOrder
			+ (bRetainedTransformActive ? SlotFeedbackConfig.RetainedFeedbackZOrderBoost : 0));
	}
	const bool bDenyActive = SlotFeedbackConfig.bEnabled
		&& DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
	const float DenyShakeOffset = bDenyActive
		? ComputeDenyShakeOffset(
			DenyFeedbackElapsedSeconds,
			SlotFeedbackConfig.DenyDuration,
			SlotFeedbackConfig.DenyShakePixels)
		: 0.0f;

	SetRenderOpacity(FMath::Clamp(SlotView.RenderOpacity, 0.0f, 1.0f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform CardRenderTransform;
	const float PressedScale = (SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer)
		? SlotFeedbackConfig.PressedScale
		: 1.0f;
	const float CommitScale =
		SlotFeedbackConfig.bEnabled
		&& SlotFeedbackConfig.bEnablePlayCommitFeedback
		&& CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration
			? SlotFeedbackConfig.PlayCommitScale
			: 1.0f;
	const float RetainedScale = bRetainedTransformActive
		? FMath::Lerp(1.0f, SlotFeedbackConfig.RetainedFeedbackScale, RetainedAlpha)
		: 1.0f;
	const float DragReadyScale =
		CardDragConfig.bEnableDragTargetFeedback
		&& EffectiveDragTargetFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CommitReady
			? CardDragConfig.DragCommitReadyScale
			: 1.0f;
	CardRenderTransform.Translation = FVector2D(
		DenyShakeOffset,
		bRetainedTransformActive
			? -SlotFeedbackConfig.RetainedFeedbackLiftPixels * RetainedAlpha
			: 0.0f);
	CardRenderTransform.Scale = FVector2D(FMath::Max(
		0.01f,
		SlotView.RenderScale
			* (bDragTargetFeedbackActive ? 1.0f : PressedScale)
			* CommitScale
			* RetainedScale
			* DragReadyScale));
	CardRenderTransform.Angle = SlotView.RenderAngleDegrees;
	SetRenderTransform(CardRenderTransform);
	ApplyFeedbackOverlay();
	ApplyInteractionFeedbackOverlay();
	UpdateVisibilityForInteractionMode();
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastVisualSlotUpdatedIfNeeded(
	const FWacomFirstPersonCardLayerSlotView& PreviousVisualSlotView,
	const FWacomFirstPersonCardLayerSlotView& CurrentVisualSlotView)
{
	if (!bIsHoveredForFirstPersonLayer || !CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		return;
	}

	const bool bVisualChanged =
		FVector2D::Distance(PreviousVisualSlotView.ScreenPosition, CurrentVisualSlotView.ScreenPosition) > 0.1f
		|| FMath::Abs(PreviousVisualSlotView.RenderAngleDegrees - CurrentVisualSlotView.RenderAngleDegrees) > 0.05f
		|| FMath::Abs(PreviousVisualSlotView.RenderScale - CurrentVisualSlotView.RenderScale) > 0.001f
		|| FMath::Abs(PreviousVisualSlotView.RenderOpacity - CurrentVisualSlotView.RenderOpacity) > 0.01f
		|| PreviousVisualSlotView.ZOrder != CurrentVisualSlotView.ZOrder
		|| PreviousVisualSlotView.bProjected != CurrentVisualSlotView.bProjected;
	if (!bVisualChanged)
	{
		return;
	}

	FWacomFirstPersonCardLayerSlotView UpdatedSlotView = CurrentVisualSlotView;
	UpdatedSlotView.bIsHovered = true;
	OnCardVisualSlotUpdatedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, UpdatedSlotView);
}

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& !IsEnterTransitionBlockingInteraction()
		&& CurrentSlotView.bProjected
		&& CurrentSlotView.Entry.CardInstanceId.IsValid();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanApplyPlayableHoverFeedback() const
{
	return CanInteractWithCurrentSlot()
		&& CurrentSlotView.Entry.bIsPlayable
		&& !CurrentSlotView.Entry.bIsPendingTargeting
		&& bIsHoveredForFirstPersonLayer;
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
	DragSlot.ScreenPosition = CurrentGestureScreenPosition;
	DragSlot.WidgetPosition = DragSlot.ScreenPosition;
	DragSlot.SnappedWidgetPosition = DragSlot.ScreenPosition;
	DragSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.CardInspectScale);
	DragSlot.RenderAngleDegrees = 0.0f;
	DragSlot.ZOrder = BaseSlotView.ZOrder + 1400;
	DragSlot.GestureState = GestureState;
	return DragSlot;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::BuildAimOverrideSlotView() const
{
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView = GetGestureBaseSlotView();
	FWacomFirstPersonCardLayerSlotView AimSlot = BaseSlotView;
	AimSlot.ScreenPosition = BaseSlotView.ScreenPosition + FVector2D(0.0f, -SlotVisualConfig.PendingTargetingLiftPixels);
	AimSlot.WidgetPosition = AimSlot.ScreenPosition;
	AimSlot.SnappedWidgetPosition = AimSlot.ScreenPosition;
	AimSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * SlotVisualConfig.PendingTargetingScale);
	if (SlotVisualConfig.bPendingTargetingStraightenAngle)
	{
		AimSlot.RenderAngleDegrees = FMath::Lerp(
			BaseSlotView.RenderAngleDegrees,
			0.0f,
			SlotVisualConfig.PendingTargetingAngleBlend);
	}
	AimSlot.ZOrder = BaseSlotView.ZOrder + SlotVisualConfig.PendingTargetingZOrderBoost;
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
	if (!ResolveAbsoluteScreenPositionToWidgetPosition(InMouseEvent.GetScreenSpacePosition(), OutScreenPosition))
	{
		return false;
	}

	const_cast<UWacomFirstPersonCardLayerSlotWidget*>(this)->UpdatePointerViewportDiagnostics(OutScreenPosition);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveAbsoluteScreenPositionToWidgetPosition(
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

bool UWacomFirstPersonCardLayerSlotWidget::IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (ResolveAbsoluteScreenPositionToWidgetPosition(ScreenPosition, WidgetPosition))
	{
		return IsWidgetPositionInsideCardBodyForFirstPersonLayer(WidgetPosition);
	}

	if (CardView && CardView->HasCardBodyHitGeometry())
	{
		return CardView->IsScreenPositionInsideCardBody(ScreenPosition);
	}

	const FGeometry& SlotGeometry = GetCachedGeometry();
	return IsLocalPositionInsideCardBody(SlotGeometry.AbsoluteToLocal(ScreenPosition));
}

bool UWacomFirstPersonCardLayerSlotWidget::IsLocalPositionInsideCardBody(const FVector2D& LocalPosition) const
{
	FVector2D BodySize = CardView
		? CardView->GetCardBodyHitSize()
		: UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		BodySize = UWacomFirstPersonCardViewWidget::GetDefaultCardBodyHitSize();
	}

	FVector2D SlotSize = GetCachedGeometry().GetLocalSize();
#if WITH_AUTOMATION_TESTS
	if (LocalHitCanvasSizeOverrideForTest.IsSet())
	{
		SlotSize = LocalHitCanvasSizeOverrideForTest.GetValue();
	}
#endif
	const FVector2D EffectiveSlotSize = (SlotSize.X > 1.0f && SlotSize.Y > 1.0f)
		? SlotSize
		: BodySize;
	const FVector2D HitMin = (EffectiveSlotSize - BodySize) * 0.5f;
	const FVector2D HitMax = HitMin + BodySize;
	return LocalPosition.X >= HitMin.X
		&& LocalPosition.Y >= HitMin.Y
		&& LocalPosition.X <= HitMax.X
		&& LocalPosition.Y <= HitMax.Y;
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateBodyHoverFromScreenPosition(const FVector2D& ScreenPosition)
{
	SetHoveredForFirstPersonLayer(CanInteractWithCurrentSlot() && IsScreenPositionInsideCardBody(ScreenPosition));
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateBodyHoverFromLocalPosition(const FVector2D& LocalPosition)
{
	SetHoveredForFirstPersonLayer(CanInteractWithCurrentSlot() && IsLocalPositionInsideCardBody(LocalPosition));
}

void UWacomFirstPersonCardLayerSlotWidget::BeginGesturePress(
	const FVector2D& ScreenPosition,
	EWacomFirstPersonCardGestureSource Source,
	EWacomFirstPersonCardGestureInputSource InputSource)
{
	if (!CanInteractWithCurrentSlot())
	{
		return;
	}

	ClearGestureState(false);
	GestureSource = Source;
	GestureInputSource = InputSource;
	GestureStartSlotView = CurrentSlotView;
	PressScreenPosition = ScreenPosition;
	CurrentGestureScreenPosition = ScreenPosition;
	UpdatePointerViewportDiagnostics(ScreenPosition);
	GestureElapsedSeconds = 0.0f;
	bGestureTargetValid = false;
	bGestureCommitArmed = false;
	GestureFeedbackTargetHandle = FWacomInteractionTargetHandle();
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	CardDragTargetFocusFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	GestureState = EWacomFirstPersonCardGestureState::Pressed;
	SetPressedForFirstPersonLayer(true);
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGesture(
	float DeltaTime,
	const FVector2D& ScreenPosition,
	bool bSuppressInspectDragPromotion)
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
			PromoteGestureToCardDrag(true);
		}
		else if (CanStartCardDragGesture()
			&& GestureElapsedSeconds >= CardDragConfig.CardInspectHoldDelaySeconds)
		{
			SetGestureState(EWacomFirstPersonCardGestureState::Inspecting, true);
		}
	}

	if (GestureState == EWacomFirstPersonCardGestureState::Inspecting
		&& !bSuppressInspectDragPromotion
		&& DragDistance >= CardDragConfig.CardDragStartThresholdPixels)
	{
		PromoteGestureToCardDrag(true);
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
		DirectDragTargetFeedbackState =
			CardDragConfig.bEnableDragTargetFeedback && bNowArmed
				? EWacomFirstPersonCardDragTargetFeedbackState::CommitReady
				: EWacomFirstPersonCardDragTargetFeedbackState::None;
		DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
		ApplyVisualSlotView();
	}

	UpdateGestureOverrideTarget();
	BroadcastDragUpdated();
	UpdateWantsTick();
}

bool UWacomFirstPersonCardLayerSlotWidget::ReleaseGesture(
	const FVector2D& ScreenPosition,
	bool bSuppressInspectDragPromotion)
{
	if (GestureState == EWacomFirstPersonCardGestureState::Idle
		|| GestureState == EWacomFirstPersonCardGestureState::Cancelled)
	{
		return false;
	}

	CurrentGestureScreenPosition = ScreenPosition;
	UpdateGesture(0.0f, ScreenPosition, bSuppressInspectDragPromotion);

	const EWacomFirstPersonCardGestureState ReleaseState = GestureState;
	SetPressedForFirstPersonLayer(false);
	BroadcastDragReleased();

	if (ReleaseState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| (ReleaseState == EWacomFirstPersonCardGestureState::AimingTargetedCard && bGestureTargetValid))
	{
		TriggerConfirmFeedback();
	}
	else if (ReleaseState == EWacomFirstPersonCardGestureState::Inspecting
		|| ReleaseState == EWacomFirstPersonCardGestureState::Pressed)
	{
		// Releasing read/inspect or an un-dragged press is a neutral return to the hand.
	}
	else if (ReleaseState != EWacomFirstPersonCardGestureState::Pressed)
	{
		TriggerDenyFeedback();
	}

	ClearGestureState(false);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::PromoteGestureToCardDrag(
	bool bBroadcastStartOrCancel)
{
	EWacomFirstPersonCardMotionIntent PromotionMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	EWacomFirstPersonCardGestureState PromotedState = EWacomFirstPersonCardGestureState::Cancelled;
	if (IsNoTargetDragCard())
	{
		PromotionMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
		PromotedState = EWacomFirstPersonCardGestureState::DraggingNoTargetCard;
	}
	else if (IsTargetedAimCard())
	{
		PromotionMotionIntent = EWacomFirstPersonCardMotionIntent::Pending;
		PromotedState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	}
	else
	{
		SetGestureState(EWacomFirstPersonCardGestureState::Cancelled, bBroadcastStartOrCancel);
		return false;
	}

	SetGestureState(PromotedState, bBroadcastStartOrCancel);
	ActiveMotionIntent = PromotionMotionIntent;
	UpdateWantsTick();
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
	const bool bHadFocusFeedback =
		CardDragTargetFocusFeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None
		|| bCardDragProbeFeedback;
	if (bHadGesture && bBroadcastCancel)
	{
		BroadcastDragCancelled();
	}

	GestureState = EWacomFirstPersonCardGestureState::Idle;
	GestureSource = EWacomFirstPersonCardGestureSource::None;
	GestureInputSource = EWacomFirstPersonCardGestureInputSource::None;
	bPreserveGestureReturnMotion = bHadGesture && SlotMotionConfig.bEnabled && bHasVisualSlotView;
	GestureStartSlotView.Reset();
	GestureOverrideTargetSlotView.Reset();
	GestureFeedbackTargetHandle = FWacomInteractionTargetHandle();
	DirectDragTargetFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	bCardDragProbeFeedback = false;
	bCardDragProbeFeedbackValid = false;
	CardDragTargetFocusFeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	ClearPointerViewportDiagnostics();
	bGestureTargetValid = false;
	bGestureCommitArmed = false;
	GestureElapsedSeconds = 0.0f;
	SetPressedForFirstPersonLayer(false);
	RefreshPresentationTarget(
		true,
		bHadFocusFeedback
			? EWacomFirstPersonCardMotionIntent::DragTargetFocus
			: EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
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
	View.GestureSource = GestureSource;
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
	View.TargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	View.bHasFeedbackTargetScreenPosition = bHasFeedbackTargetScreenPosition;
	View.FeedbackTargetScreenPosition = FeedbackTargetScreenPosition;
	return View;
}

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredFromFirstPersonLayer(bool bHovered)
{
	SetHoveredForFirstPersonLayer(bHovered, false);
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginGesturePressFromFirstPersonLayer(const FVector2D& WidgetPosition)
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	BeginGesturePress(
		WidgetPosition,
		EWacomFirstPersonCardGestureSource::MousePress,
		EWacomFirstPersonCardGestureInputSource::MousePointer);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginDragGestureFromFirstPersonLayer(const FVector2D& WidgetPosition)
{
	return BeginDragGestureFromFirstPersonLayer(WidgetPosition, WidgetPosition);
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginDragGestureFromFirstPersonLayer(
	const FVector2D& GestureOriginPosition,
	const FVector2D& InitialPointerPosition)
{
	if (!CanStartCardDragGesture())
	{
		return false;
	}

	BeginGesturePress(
		GestureOriginPosition,
		EWacomFirstPersonCardGestureSource::KeyboardShortcut,
		EWacomFirstPersonCardGestureInputSource::ExternalPointer);
	CurrentGestureScreenPosition = InitialPointerPosition;
	UpdatePointerViewportDiagnostics(InitialPointerPosition);
	if (!PromoteGestureToCardDrag(true))
	{
		return false;
	}

	UpdateGesture(0.0f, InitialPointerPosition);
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::BeginInspectScrubFromFirstPersonLayer(
	const FVector2D& WidgetPosition)
{
	if (!CanBeginInspectScrubFromFirstPersonLayer())
	{
		return false;
	}

	BeginGesturePress(
		WidgetPosition,
		EWacomFirstPersonCardGestureSource::MousePress,
		EWacomFirstPersonCardGestureInputSource::MousePointer);
	CurrentGestureScreenPosition = WidgetPosition;
	UpdatePointerViewportDiagnostics(WidgetPosition);
	GestureElapsedSeconds = FMath::Max(GestureElapsedSeconds, CardDragConfig.CardInspectHoldDelaySeconds);
	SetGestureState(EWacomFirstPersonCardGestureState::Inspecting, true);
	UpdateGesture(0.0f, WidgetPosition, true);
	return GestureState == EWacomFirstPersonCardGestureState::Inspecting;
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGestureFromFirstPersonLayer(
	float DeltaTime,
	const FVector2D& WidgetPosition,
	bool bSuppressInspectDragPromotion)
{
	UpdateGesture(DeltaTime, WidgetPosition, bSuppressInspectDragPromotion);
}

bool UWacomFirstPersonCardLayerSlotWidget::ReleaseGestureFromFirstPersonLayer(
	const FVector2D& WidgetPosition,
	bool bSuppressInspectDragPromotion)
{
	return ReleaseGesture(WidgetPosition, bSuppressInspectDragPromotion);
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInspectScrubGestureFromFirstPersonLayer()
{
	if (GestureState == EWacomFirstPersonCardGestureState::Inspecting)
	{
		ClearGestureState(false);
	}
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
FWacomFirstPersonCardSlotAutomationTestView UWacomFirstPersonCardLayerSlotWidget::GetAutomationTestViewForTest() const
{
	FWacomFirstPersonCardSlotAutomationTestView View;
	if (CardView)
	{
		const FWacomFirstPersonCardViewAutomationTestView CardViewTestView =
			CardView->GetAutomationTestViewForTest();
		View.FeedbackOverlayOpacity = CardViewTestView.FeedbackOverlayOpacity;
		View.FeedbackOverlayColor = CardViewTestView.FeedbackOverlayColor;
		View.InteractionFeedbackOpacity = CardViewTestView.InteractionFeedbackOpacity;
		View.InteractionFeedbackKind = CardViewTestView.InteractionFeedbackKind;
		View.bHasInteractionFeedbackImage =
			CardViewTestView.bHasInteractionFeedbackImage;
		View.bInteractionFeedbackMaterialConfigured =
			CardViewTestView.bInteractionFeedbackMaterialConfigured;
		View.bInteractionFeedbackMaterialLoaded =
			CardViewTestView.bInteractionFeedbackMaterialLoaded;
		View.bInteractionFeedbackUsesOverrideMaterial =
			CardViewTestView.bInteractionFeedbackUsesOverrideMaterial;
		View.bInteractionFeedbackUsesBrushMaterial =
			CardViewTestView.bInteractionFeedbackUsesBrushMaterial;
		View.bInteractionFeedbackLayerAboveFeedbackOverlay =
			CardViewTestView.bInteractionFeedbackLayerAboveFeedbackOverlay;
	}
	View.GestureSource = GestureSource;
	View.bPressed = bIsPressedForFirstPersonLayer;
	View.bDenyFeedbackActive = DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
	View.bConfirmFeedbackActive = ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration;
	View.bCommitFeedbackActive = CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration;
	View.bRetainedFeedbackActive = IsRetainedFeedbackActive();
	View.RetainedFeedbackElapsedSeconds = RetainedFeedbackElapsedSeconds;
	View.RetainedFeedbackStartDelaySeconds = RetainedFeedbackStartDelaySeconds;
	View.bCardDragProbeFeedback = bCardDragProbeFeedback;
	View.bCardDragTargetAffordanceFeedback = bCardDragTargetAffordanceFeedback;
	View.bCardDragTargetFocusActive =
		CardDragConfig.bEnableDragTargetFeedback
		&& bCardDragProbeFeedback
		&& IsCardTargetFocusFeedbackState(CardDragTargetFocusFeedbackState);
	View.CardDragTargetAffordanceFeedbackState = CardDragTargetAffordanceFeedbackState;
	View.CardDragTargetFocusFeedbackState = CardDragTargetFocusFeedbackState;
	View.DirectDragTargetFeedbackState = DirectDragTargetFeedbackState;
	View.DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	View.ActiveMotionIntent = ActiveMotionIntent;
	View.bEnterTransitionPlaybackActive = EnterTransitionPlayback.bActive;
	View.bEnterTransitionBlocksInteraction = IsEnterTransitionBlockingInteraction();
	View.EnterTransitionElapsedSeconds = EnterTransitionPlayback.ElapsedSeconds;
	View.EnterTransitionStartDelaySeconds = EnterTransitionPlayback.StartDelaySeconds;
	View.EnterTransitionDurationSeconds = EnterTransitionPlayback.DurationSeconds;
	View.SlotMotionConfig = SlotMotionConfig;
	View.CardDragConfig = CardDragConfig;
	View.SlotVisualConfig = SlotVisualConfig;
	View.SlotMotionConfigApplyCount = SlotMotionConfigApplyCountForTest;
	View.SlotVisualConfigApplyCount = SlotVisualConfigApplyCountForTest;
	View.SlotFeedbackConfigApplyCount = SlotFeedbackConfigApplyCountForTest;
	View.CardDragConfigApplyCount = CardDragConfigApplyCountForTest;
	return View;
}

void UWacomFirstPersonCardLayerSlotWidget::SetLocalHitCanvasSizeOverrideForTest(
	const TOptional<FVector2D>& InSize)
{
	LocalHitCanvasSizeOverrideForTest = InSize;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestHoverAtLocalPositionForTest(const FVector2D& LocalPosition)
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	UpdateBodyHoverFromLocalPosition(LocalPosition);
	return bIsHoveredForFirstPersonLayer;
}

void UWacomFirstPersonCardLayerSlotWidget::RequestMoveAtLocalPositionForTest(const FVector2D& LocalPosition)
{
	if (GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled)
	{
		return;
	}

	UpdateBodyHoverFromLocalPosition(LocalPosition);
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestPressAtLocalPositionForTest(const FVector2D& LocalPosition)
{
	if (!CanInteractWithCurrentSlot() || !IsLocalPositionInsideCardBody(LocalPosition))
	{
		return false;
	}

	const FVector2D PressPosition = bHasVisualSlotView ? VisualSlotView.ScreenPosition : CurrentSlotView.ScreenPosition;
	BeginGesturePress(
		PressPosition,
		EWacomFirstPersonCardGestureSource::MousePress,
		EWacomFirstPersonCardGestureInputSource::MousePointer);
	return true;
}

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
	return true;
}

bool UWacomFirstPersonCardLayerSlotWidget::RequestGesturePressForTest(const FVector2D& ScreenPosition)
{
	if (!CanInteractWithCurrentSlot())
	{
		return false;
	}

	BeginGesturePress(
		ScreenPosition,
		EWacomFirstPersonCardGestureSource::MousePress,
		EWacomFirstPersonCardGestureInputSource::MousePointer);
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
#endif

void UWacomFirstPersonCardLayerSlotWidget::SetHoveredForFirstPersonLayer(bool bHovered, bool bBroadcast)
{
	if (bIsHoveredForFirstPersonLayer == bHovered)
	{
		return;
	}

	bIsHoveredForFirstPersonLayer = bHovered;
	CurrentSlotView.bIsHovered = bIsHoveredForFirstPersonLayer;
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Hover);
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
	UpdateWantsTick();
	if (!bBroadcast)
	{
		return;
	}
	if (CurrentSlotView.Entry.CardInstanceId.IsValid())
	{
		if (bIsHoveredForFirstPersonLayer)
		{
			FWacomFirstPersonCardLayerSlotView VisualHoverSlotView = VisualSlotView;
			VisualHoverSlotView.bIsHovered = true;
			OnCardHoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, VisualHoverSlotView);
			if (const FWacomInteractionTargetHandle CardTargetHandle = BuildCardTargetHandle(); CardTargetHandle.IsValid())
			{
				OnCardTargetHoveredNative.Broadcast(CardTargetHandle, VisualHoverSlotView);
			}
		}
		else
		{
			const FWacomInteractionTargetHandle CardTargetHandle = FWacomInteractionTargetHandle::ForCardTarget(
				CurrentSlotView.Entry.CardInstanceId,
				this,
				VisualSlotView.ScreenPosition);
			FWacomFirstPersonCardLayerSlotView VisualTargetSlotView = VisualSlotView;
			VisualTargetSlotView.bIsHovered = false;
			OnCardUnhoveredNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, VisualTargetSlotView);
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

void UWacomFirstPersonCardLayerSlotWidget::TriggerRetainedFeedback(
	int32 SequenceIndex,
	int32 SequenceCount)
{
	if (!SlotFeedbackConfig.bEnabled
		|| !SlotFeedbackConfig.bEnableRetainedFeedback
		|| SlotFeedbackConfig.RetainedFeedbackDuration <= 0.0f)
	{
		return;
	}

	const int32 SafeSequenceIndex = FMath::Clamp(
		SequenceIndex,
		0,
		FMath::Max(0, SequenceCount - 1));
	RetainedFeedbackStartDelaySeconds =
		static_cast<float>(SafeSequenceIndex) * SlotFeedbackConfig.RetainedFeedbackStaggerSeconds;
	RetainedFeedbackElapsedSeconds = 0.0f;
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInteractionFeedback()
{
	bIsPressedForFirstPersonLayer = false;
	ClearCardDragTargetFeedback();
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	ConfirmFeedbackElapsedSeconds = SlotFeedbackConfig.ConfirmDuration;
	DenyFeedbackElapsedSeconds = SlotFeedbackConfig.DenyDuration;
	CommitFeedbackElapsedSeconds = SlotFeedbackConfig.PlayCommitDuration;
	RetainedFeedbackStartDelaySeconds = 0.0f;
	RetainedFeedbackElapsedSeconds = SlotFeedbackConfig.RetainedFeedbackDuration;
	ApplyFeedbackOverlay();
	ApplyInteractionFeedbackOverlay();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsDenyFeedbackActive() const
{
	return SlotFeedbackConfig.bEnabled
		&& DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsRetainedFeedbackActive() const
{
	return SlotFeedbackConfig.bEnabled
		&& SlotFeedbackConfig.bEnableRetainedFeedback
		&& SlotFeedbackConfig.RetainedFeedbackDuration > 0.0f
		&& RetainedFeedbackElapsedSeconds
			< RetainedFeedbackStartDelaySeconds + SlotFeedbackConfig.RetainedFeedbackDuration;
}

bool UWacomFirstPersonCardLayerSlotWidget::HasActivePresentationPlayback() const
{
	return IsEnterTransitionPlaybackActive()
		|| IsExitingForFirstPersonLayer()
		|| IsRetainedFeedbackActive();
}

float UWacomFirstPersonCardLayerSlotWidget::ComputeRetainedFeedbackAlpha() const
{
	if (!IsRetainedFeedbackActive())
	{
		return 0.0f;
	}

	const float PlaybackSeconds = RetainedFeedbackElapsedSeconds - RetainedFeedbackStartDelaySeconds;
	if (PlaybackSeconds < 0.0f)
	{
		return 0.0f;
	}

	return ComputeFeedbackPulseAlpha(PlaybackSeconds, SlotFeedbackConfig.RetainedFeedbackDuration);
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyFeedbackOverlay()
{
	if (!CardView)
	{
		return;
	}

	FLinearColor OverlayColor = FLinearColor::Transparent;
	float OverlayOpacity = 0.0f;
	if (SlotFeedbackConfig.bEnabled && !IsDenyFeedbackActive())
	{
		const float ConfirmAlpha = ComputeFeedbackPulseAlpha(
			ConfirmFeedbackElapsedSeconds,
			SlotFeedbackConfig.ConfirmDuration);
		const float CommitAlpha =
			SlotFeedbackConfig.bEnablePlayCommitFeedback
				? ComputeFeedbackPulseAlpha(CommitFeedbackElapsedSeconds, SlotFeedbackConfig.PlayCommitDuration)
				: 0.0f;
		const float RetainedAlpha = ComputeRetainedFeedbackAlpha();
		const bool bSourceInteractionFeedbackActive =
			bIsPressedForFirstPersonLayer
			|| ConfirmAlpha > 0.0f
			|| CommitAlpha > 0.0f
			|| RetainedAlpha > 0.0f;
		if (RetainedAlpha > 0.0f)
		{
			OverlayColor = SlotFeedbackConfig.RetainedFeedbackColor;
			OverlayOpacity = SlotFeedbackConfig.RetainedFeedbackOpacity * RetainedAlpha;
		}
		if (!bSourceInteractionFeedbackActive && CanApplyPlayableHoverFeedback())
		{
			OverlayColor = SlotFeedbackConfig.PlayableHoverColor;
			OverlayOpacity = SlotFeedbackConfig.PlayableHoverOpacity;
		}
	}
	if (CardDragConfig.bEnableDragTargetFeedback && !IsDenyFeedbackActive())
	{
		const EWacomFirstPersonCardDragTargetFeedbackState EffectiveFeedbackState =
			ResolveEffectiveDragTargetFeedbackState();
		const bool bEffectiveValidFeedback =
			bCardDragProbeFeedbackValid || bCardDragTargetAffordanceFeedbackValid;
		const bool bAnyCardTargetFeedback =
			bCardDragProbeFeedback || bCardDragTargetAffordanceFeedback;
		if (EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CommitReady
			|| EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
			|| EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget)
		{
			OverlayColor = CardDragConfig.DragValidTargetColor;
			OverlayOpacity = CardDragConfig.DragTargetFeedbackOpacity;
		}
		else if (EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::Invalid
			|| EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget)
		{
			OverlayColor = CardDragConfig.DragInvalidTargetColor;
			OverlayOpacity = EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget
				? CardDragConfig.DragTargetFeedbackOpacity * 0.6f
				: CardDragConfig.DragTargetFeedbackOpacity;
		}
		else if (bAnyCardTargetFeedback && bEffectiveValidFeedback)
		{
			OverlayColor = CardDragConfig.DragValidTargetColor;
			OverlayOpacity = CardDragConfig.DragTargetFeedbackOpacity;
		}
		else if (EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| EffectiveFeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ZoneProbe
			|| bAnyCardTargetFeedback)
		{
			OverlayColor = CardDragConfig.DragCardProbeTargetColor;
			OverlayOpacity = CardDragConfig.DragTargetFeedbackOpacity;
		}
	}

	CardView->SetFeedbackOverlayView(OverlayColor, OverlayOpacity);
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyInteractionFeedbackOverlay()
{
	if (!CardView)
	{
		return;
	}

	const float DenyAlpha = IsDenyFeedbackActive()
		? ComputeFeedbackPulseAlpha(DenyFeedbackElapsedSeconds, SlotFeedbackConfig.DenyDuration)
		: 0.0f;
	const float CommitAlpha =
		SlotFeedbackConfig.bEnabled && SlotFeedbackConfig.bEnablePlayCommitFeedback
			? ComputeFeedbackPulseAlpha(CommitFeedbackElapsedSeconds, SlotFeedbackConfig.PlayCommitDuration)
			: 0.0f;
	const float ConfirmAlpha = SlotFeedbackConfig.bEnabled
		? ComputeFeedbackPulseAlpha(ConfirmFeedbackElapsedSeconds, SlotFeedbackConfig.ConfirmDuration)
		: 0.0f;
	const float RetainedAlpha = ComputeRetainedFeedbackAlpha();
	const bool bDragTargetFeedbackActive =
		CardDragConfig.bEnableDragTargetFeedback
		&& ResolveEffectiveDragTargetFeedbackState() != EWacomFirstPersonCardDragTargetFeedbackState::None;
	const bool bPressedFeedbackActive =
		SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer && !bDragTargetFeedbackActive;

	EWacomFirstPersonCardInteractionFeedbackKind FeedbackKind =
		EWacomFirstPersonCardInteractionFeedbackKind::None;
	FLinearColor FeedbackColor = FLinearColor::Transparent;
	float FeedbackOpacity = 0.0f;
	float FeedbackPulse = 0.0f;

	if (DenyAlpha > 0.0f)
	{
		FeedbackKind = EWacomFirstPersonCardInteractionFeedbackKind::Deny;
		FeedbackColor = SlotFeedbackConfig.DenyColor;
		FeedbackOpacity = SlotFeedbackConfig.DenyOpacity;
		FeedbackPulse = DenyAlpha;
	}
	else if (bPressedFeedbackActive)
	{
		FeedbackKind = EWacomFirstPersonCardInteractionFeedbackKind::Pressed;
		FeedbackColor = SlotFeedbackConfig.PressedColor;
		FeedbackOpacity = SlotFeedbackConfig.PressedOpacity;
		FeedbackPulse = 1.0f;
	}
	else if (CommitAlpha > 0.0f)
	{
		FeedbackKind = EWacomFirstPersonCardInteractionFeedbackKind::Commit;
		FeedbackColor = SlotFeedbackConfig.PlayCommitColor;
		FeedbackOpacity = SlotFeedbackConfig.PlayCommitOpacity;
		FeedbackPulse = CommitAlpha;
	}
	else if (RetainedAlpha > 0.0f && !bDragTargetFeedbackActive)
	{
		FeedbackKind = EWacomFirstPersonCardInteractionFeedbackKind::Retained;
		FeedbackColor = SlotFeedbackConfig.RetainedFeedbackColor;
		FeedbackOpacity = SlotFeedbackConfig.RetainedFeedbackOpacity;
		FeedbackPulse = RetainedAlpha;
	}
	else if (ConfirmAlpha > 0.0f)
	{
		FeedbackKind = EWacomFirstPersonCardInteractionFeedbackKind::Confirm;
		FeedbackColor = SlotFeedbackConfig.PressedColor;
		FeedbackOpacity = SlotFeedbackConfig.ConfirmOpacity;
		FeedbackPulse = ConfirmAlpha;
	}

	if (FeedbackKind == EWacomFirstPersonCardInteractionFeedbackKind::None)
	{
		CardView->ClearInteractionFeedbackView();
		return;
	}

	FWacomFirstPersonCardInteractionFeedbackView View;
	View.Kind = FeedbackKind;
	View.Material = SlotFeedbackConfig.InteractionFeedbackMaterial;
	View.Color = FeedbackColor;
	View.Opacity = FeedbackOpacity;
	View.Pulse = FeedbackPulse;
	View.EdgeWidth = SlotFeedbackConfig.InteractionFeedbackEdgeWidth;
	View.EdgeSoftness = SlotFeedbackConfig.InteractionFeedbackEdgeSoftness;
	View.VignetteStrength = SlotFeedbackConfig.InteractionFeedbackVignetteStrength;
	View.VignetteRadius = SlotFeedbackConfig.InteractionFeedbackVignetteRadius;
	View.VignetteSoftness = SlotFeedbackConfig.InteractionFeedbackVignetteSoftness;
	CardView->SetInteractionFeedbackView(View);
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
		|| IsRetainedFeedbackActive()
		|| (CardDragConfig.bEnableDragTargetFeedback
			&& (ResolveEffectiveDragTargetFeedbackState() != EWacomFirstPersonCardDragTargetFeedbackState::None
				|| bCardDragProbeFeedback
				|| bCardDragTargetAffordanceFeedback));
	const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
	const bool bGestureActive =
		GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	bWantsSlotMotionTick = IsEnterTransitionPlaybackActive()
		|| bIsExitingForFirstPersonLayer
		|| (bHasVisualSlotView
			&& (FVector2D::Distance(VisualSlotView.ScreenPosition, EffectiveTargetSlotView.ScreenPosition) > 0.1f
				|| FMath::Abs(VisualSlotView.RenderAngleDegrees - EffectiveTargetSlotView.RenderAngleDegrees) > 0.05f
				|| FMath::Abs(VisualSlotView.RenderScale - EffectiveTargetSlotView.RenderScale) > 0.001f
				|| FMath::Abs(VisualSlotView.RenderOpacity - EffectiveTargetSlotView.RenderOpacity) > 0.01f
				|| VisualSlotView.ZOrder != EffectiveTargetSlotView.ZOrder))
		|| bFeedbackActive
		|| bGestureActive;
}

void UWacomFirstPersonCardLayerSlotWidget::StartEnterTransitionPlayback(
	const FWacomFirstPersonCardLayerSlotView& StartSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& EnterProfile)
{
	EnterTransitionPlayback.bActive = true;
	EnterTransitionPlayback.StartSlotView = StartSlotView;
	EnterTransitionPlayback.ElapsedSeconds = 0.0f;
	EnterTransitionPlayback.StartDelaySeconds = FMath::Max(0.0f, EnterProfile.StartDelaySeconds);
	EnterTransitionPlayback.DurationSeconds = FMath::Max(0.0f, EnterProfile.DurationSeconds);
	EnterTransitionPlayback.ArcLiftPixels = FMath::Max(0.0f, EnterProfile.ArcLiftPixels);
	EnterTransitionPlayback.EasePower = FMath::Max(0.1f, EnterProfile.EasePower);
	EnterTransitionPlayback.bBlockInteractionDuringPlayback =
		EnterProfile.bBlockInteractionDuringPlayback;
}

void UWacomFirstPersonCardLayerSlotWidget::ClearEnterTransitionPlayback()
{
	EnterTransitionPlayback = FWacomFirstPersonCardEnterTransitionPlayback();
}

bool UWacomFirstPersonCardLayerSlotWidget::TickEnterTransitionPlayback(float DeltaTime)
{
	if (!EnterTransitionPlayback.bActive)
	{
		return true;
	}

	EnterTransitionPlayback.ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float PlaybackSeconds =
		EnterTransitionPlayback.ElapsedSeconds - EnterTransitionPlayback.StartDelaySeconds;
	if (PlaybackSeconds < 0.0f)
	{
		VisualSlotView = EnterTransitionPlayback.StartSlotView;
		ApplyVisualSlotView();
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
	const float DurationSeconds = EnterTransitionPlayback.DurationSeconds;
	const float LinearAlpha = DurationSeconds <= 0.0f
		? 1.0f
		: FMath::Clamp(PlaybackSeconds / DurationSeconds, 0.0f, 1.0f);
	const float EasedAlpha =
		ComputeTransitionEaseAlpha(LinearAlpha, EnterTransitionPlayback.EasePower);
	VisualSlotView = LerpSlotView(
		EnterTransitionPlayback.StartSlotView,
		EffectiveTargetSlotView,
		EasedAlpha,
		EasedAlpha);
	if (EnterTransitionPlayback.ArcLiftPixels > 0.0f && LinearAlpha > 0.0f && LinearAlpha < 1.0f)
	{
		const float ArcAlpha = FMath::Sin(LinearAlpha * UE_PI);
		VisualSlotView.ScreenPosition.Y -= EnterTransitionPlayback.ArcLiftPixels * ArcAlpha;
		VisualSlotView.WidgetPosition = VisualSlotView.ScreenPosition;
		VisualSlotView.SnappedWidgetPosition = VisualSlotView.ScreenPosition;
	}
	ApplyVisualSlotView();

	if (LinearAlpha >= 1.0f)
	{
		ClearEnterTransitionPlayback();
		VisualSlotView = EffectiveTargetSlotView;
		ApplyVisualSlotView();
		return true;
	}

	return false;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsEnterTransitionBlockingInteraction() const
{
	return EnterTransitionPlayback.bActive
		&& EnterTransitionPlayback.bBlockInteractionDuringPlayback;
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
