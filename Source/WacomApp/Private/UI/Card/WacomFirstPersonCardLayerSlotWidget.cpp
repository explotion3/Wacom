// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/GameViewportClient.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerConfigUtils.h"
#include "UI/Card/WacomFirstPersonCardDataRewritePlayback.h"
#include "UI/Card/WacomFirstPersonCardEffectBadgeFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardDepthMotion.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardGainRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardGestureController.h"
#include "UI/Card/WacomFirstPersonCardRetainSealPlayback.h"
#include "UI/Card/WacomFirstPersonCardDragPickupPlayback.h"
#include "UI/Card/WacomFirstPersonCardHandTargetImpactPlayback.h"
#include "UI/Card/WacomFirstPersonCardInteractionFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardMotionMixer.h"
#include "UI/Card/WacomFirstPersonCardSlotPresentationController.h"
#include "UI/Card/WacomFirstPersonCardSurfaceEffectViewBuilder.h"
#include "UI/Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"
#include "UI/Card/WacomFirstPersonCardUseReformPlayback.h"
#include "UI/Card/WacomFirstPersonCardTransitionPlayback.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

namespace
{
	bool IsCardTargetFocusFeedbackState(EWacomFirstPersonCardDragTargetFeedbackState FeedbackState)
	{
		return FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::CardProbe
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget
			|| FeedbackState == EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	}

	bool IsFormalDragGestureStateForVisualMotion(
		EWacomFirstPersonCardGestureState InGestureState)
	{
		return InGestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| InGestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
			|| InGestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard;
	}

}

void UWacomFirstPersonCardLayerSlotWidget::SetSlotRuntimeConfig(
	const FWacomFirstPersonCardSlotRuntimeConfig& InConfig)
{
	const FWacomFirstPersonCardSlotRuntimeConfig NewConfig = NormalizeSlotRuntimeConfig(InConfig);
	FWacomFirstPersonCardSlotRuntimeConfig CurrentConfig;
	CurrentConfig.Motion = SlotMotionConfig;
	CurrentConfig.Visual = SlotVisualConfig;
	CurrentConfig.Interaction = InteractionFeedbackConfig;
	CurrentConfig.DragPickup = DragPickupConfig;
	CurrentConfig.Drag = CardDragConfig;
	if (AreSlotRuntimeConfigsEquivalent(CurrentConfig, NewConfig))
	{
		return;
	}

	const bool bMotionChanged =
		!AreSlotMotionConfigsEquivalent(SlotMotionConfig, NewConfig.Motion);
	const bool bVisualChanged =
		!AreSlotVisualConfigsEquivalent(SlotVisualConfig, NewConfig.Visual);
	const bool bInteractionChanged =
		!AreInteractionFeedbackConfigsEquivalent(
			InteractionFeedbackConfig,
			NewConfig.Interaction);
	const bool bDragPickupChanged =
		!AreDragPickupConfigsEquivalent(DragPickupConfig, NewConfig.DragPickup);
	const bool bDragChanged = !AreCardDragConfigsEquivalent(CardDragConfig, NewConfig.Drag);

	SlotMotionConfig = NewConfig.Motion;
	SlotVisualConfig = NewConfig.Visual;
	InteractionFeedbackConfig = NewConfig.Interaction;
	DragPickupConfig = NewConfig.DragPickup;
	CardDragConfig = NewConfig.Drag;
#if WITH_AUTOMATION_TESTS
	++SlotRuntimeConfigApplyCountForTest;
#endif
	if (!InteractionFeedbackPlayback)
	{
		InteractionFeedbackPlayback.Reset(
			new FWacomFirstPersonCardInteractionFeedbackPlayback());
	}
	InteractionFeedbackPlayback->SetConfig(InteractionFeedbackConfig);

	if (bVisualChanged)
	{
		if (CardView)
		{
			CardView->SetEffectBadgeFeedbackConfig(SlotVisualConfig.EffectBadgeFeedback);
		}
		if (IsSurfaceDeparturePlaybackActive())
		{
			ClearSurfaceDeparturePlayback();
		}
		if (IsCardUseReformPlaybackActive())
		{
			ClearCardUseReformPlayback(true);
		}
		if (IsHandTargetImpactPlaybackActive())
		{
			ClearHandTargetImpactPlayback();
		}
		if (IsCardDataRewritePlaybackActive() || PresentationController->State.bPendingDataRewriteHandoff)
		{
			ClearCardDataRewritePlayback();
		}
		if (IsEffectBadgeFeedbackPlaybackActive())
		{
			ClearEffectBadgeFeedbackPlayback();
		}
		if (IsDrawRevealPlaybackActive())
		{
			ClearDrawRevealPlayback();
		}
		if (IsGainRevealPlaybackActive())
		{
			ClearGainRevealPlayback();
		}
		if (IsRetainSealPlaybackActive())
		{
			ClearRetainSealPlayback();
		}
		if (CardDepthMotion)
		{
			CardDepthMotion->InvalidateTarget();
		}
	}

	if (bDragPickupChanged && DragPickupPlayback && DragPickupPlayback->IsActive())
	{
		ResetDragPickupFeedback();
	}
	if (bInteractionChanged && !InteractionFeedbackConfig.bEnabled)
	{
		ClearInteractionFeedback();
	}
	if (bVisualChanged && !SlotVisualConfig.RetainSeal.bEnabled)
	{
		ClearRetainSealPlayback();
	}
	if (bDragChanged && !CardDragConfig.bEnableFirstPersonCardDragCommit)
	{
		ClearGestureState(true);
	}

	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	if (bVisualChanged
		&& !IsHandTargetImpactPlaybackActive()
		&& !IsCardDataRewritePlaybackActive()
		&& !IsRetainSealPlaybackActive())
	{
		ResetCardSurfaceEffectView();
	}
	if (bMotionChanged && !SlotMotionConfig.bEnabled && bHasVisualSlotView)
	{
		ClearEnterTransitionPlayback();
		VisualSlotView = TargetSlotView;
	}
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void FWacomFirstPersonCardInteractionFeedbackPlaybackDeleter::operator()(
	FWacomFirstPersonCardInteractionFeedbackPlayback* Playback) const
{
	delete Playback;
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
	const bool bCardIdentityChanged =
		CurrentSlotView.Entry.CardInstanceId != InSlotView.Entry.CardInstanceId;
	const bool bResetCardDepth =
		CurrentSlotView.Entry.CardInstanceId != InSlotView.Entry.CardInstanceId
		|| !InSlotView.bProjected
		|| !InSlotView.Entry.CardInstanceId.IsValid();
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
	ClearExitTransitionPlayback();
	ClearSurfaceDeparturePlayback();
	ClearCardUseReformPlayback();
	ClearHandTargetImpactPlayback();
	ClearCardDataRewritePlayback();
	ClearEffectBadgeFeedbackPlayback();
	if (bCardIdentityChanged || !InSlotView.bProjected || !InSlotView.Entry.CardInstanceId.IsValid())
	{
		ClearRetainSealPlayback();
	}
	if (bResetCardDepth && CardDepthMotion)
	{
		CardDepthMotion->Reset();
		ClearPointerViewportDiagnostics();
	}

	if (bCardIdentityChanged)
	{
		ResetDragPickupFeedback();
		if (PresentationController)
		{
			PresentationController->ResetOwnedState();
		}
	}

	CurrentSlotView = InSlotView;
	bHasVisualSlotView = true;
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	VisualSlotView = TargetSlotView;
	ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	bIsExitingForFirstPersonLayer = false;
	bUsesFixedExitTransitionPlayback = false;
	PresentationController->State.bUsesSurfaceDepartureExit = false;
	ExitMotionElapsedSeconds = 0.0f;
	ApplyCurrentSlotView();
	ApplyActiveSurfaceEffectView();
	ApplyVisualSlotView();
	UpdateWantsTick();
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

UWacomFirstPersonCardLayerSlotWidget::UWacomFirstPersonCardLayerSlotWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GestureController(new FWacomFirstPersonCardGestureController())
	, CardDepthMotion(new FWacomFirstPersonCardDepthMotion())
	, InteractionFeedbackPlayback(new FWacomFirstPersonCardInteractionFeedbackPlayback())
	, PresentationController(new FWacomFirstPersonCardSlotPresentationController())
{
}

UWacomFirstPersonCardLayerSlotWidget::~UWacomFirstPersonCardLayerSlotWidget() = default;

void FWacomFirstPersonCardDepthMotionDeleter::operator()(
	FWacomFirstPersonCardDepthMotion* Motion) const
{
	delete Motion;
}

void FWacomFirstPersonCardDragPickupPlaybackDeleter::operator()(
	FWacomFirstPersonCardDragPickupPlayback* Playback) const
{
	delete Playback;
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
		ClearCardUseReformPlayback();
		ClearHandTargetImpactPlayback();
		ClearCardDataRewritePlayback();
		ClearEffectBadgeFeedbackPlayback();
		ClearDrawRevealPlayback();
		ClearGainRevealPlayback();
		ClearRetainSealPlayback();
		ClearGestureState(true);
		ClearInteractionFeedback();
	}
	if ((bTreatAsNewSlot
			|| CurrentSlotView.Entry.CardInstanceId != InTargetSlotView.Entry.CardInstanceId
			|| !InTargetSlotView.bProjected
			|| !InTargetSlotView.Entry.CardInstanceId.IsValid())
		&& CardDepthMotion)
	{
		CardDepthMotion->Reset();
		ClearPointerViewportDiagnostics();
	}

	const bool bCanReuseVisual =
		bHasVisualSlotView
		&& !bTreatAsNewSlot
		&& CurrentSlotView.Entry.CardInstanceId == InTargetSlotView.Entry.CardInstanceId;
	const FWacomFirstPersonCardLayerSlotView PreviousBaseSlotView = CurrentSlotView;
	const FWacomFirstPersonCardLayerSlotView PreviousPresentationSlotView = TargetSlotView;
	const FWacomFirstPersonCardLayerSlotView IncomingPresentationSlotView =
		ComposePresentationSlotView(InTargetSlotView);
	if (bTreatAsNewSlot
		|| CurrentSlotView.Entry.CardInstanceId != InTargetSlotView.Entry.CardInstanceId)
	{
		ResetDragPickupFeedback();
	}

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
	bUsesFixedExitTransitionPlayback = false;
	PresentationController->State.bUsesSurfaceDepartureExit = false;
	ExitMotionElapsedSeconds = 0.0f;
	ClearExitTransitionPlayback();
	ClearSurfaceDeparturePlayback();
	ApplyCurrentSlotView();
	if (!IsCardUseReformPlaybackActive()
		&& !IsHandTargetImpactPlaybackActive()
		&& !IsCardDataRewritePlaybackActive()
		&& !IsDrawRevealPlaybackActive()
		&& !IsGainRevealPlaybackActive()
		&& !IsRetainSealPlaybackActive())
	{
		ResetCardSurfaceEffectView();
	}

	if (!bCanReuseVisual)
	{
		GestureRuntime().bPreserveReturnMotion = false;
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
				|| EnterProfile.ArcLiftPixels > 0.0f
				|| EnterProfile.TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Default
				|| !EnterProfile.StartSound.IsNull())
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
	const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride,
	EWacomFirstPersonCardSlotTransitionKind TransitionKind)
{
	const bool bUseCardUseEffect =
		TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played
		&& CanPlayCardUseEffect();
	const bool bUseExhaustDissolve =
		TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Exhausted
		&& CanPlayExhaustDissolve();
	const bool bUseSurfaceDeparture = bUseCardUseEffect || bUseExhaustDissolve;
	const bool bUseFixedPlayback = ExitProfileOverride.IsSet()
		&& !bUseSurfaceDeparture
		&& (ExitProfileOverride.GetValue().DurationSeconds > 0.0f
			|| ExitProfileOverride.GetValue().StartDelaySeconds > 0.0f
			|| ExitProfileOverride.GetValue().ArcLiftPixels > 0.0f);
	const float ResolvedExitDuration = bUseCardUseEffect
		? (SlotVisualConfig.CardUseEffect.bReducedMotion
			? 0.12f
			: FMath::Max(0.0f, SlotVisualConfig.CardUseEffect.Style.DurationSeconds))
		: (bUseExhaustDissolve
			? (SlotVisualConfig.PlayedDissolve.bReducedMotion
				? 0.12f
				: FMath::Max(0.0f, SlotVisualConfig.PlayedDissolve.Style.DurationSeconds))
		: (bUseFixedPlayback
			? FMath::Max(0.0f, ExitProfileOverride.GetValue().DurationSeconds)
			: FMath::Max(0.0f, SlotMotionConfig.ExitDuration)));
	if ((!SlotMotionConfig.bEnabled && !bUseSurfaceDeparture)
		|| ResolvedExitDuration <= 0.0f
		|| !bHasVisualSlotView)
	{
		SetHoveredForFirstPersonLayer(false);
		ClearInteractionFeedback();
		ClearEnterTransitionPlayback();
		ClearExitTransitionPlayback();
		ClearSurfaceDeparturePlayback();
		ClearCardDataRewritePlayback();
		ClearEffectBadgeFeedbackPlayback();
		ClearRetainSealPlayback();
		bIsExitingForFirstPersonLayer = true;
		bUsesFixedExitTransitionPlayback = false;
		PresentationController->State.bUsesSurfaceDepartureExit = false;
		PresentationController->State.SurfaceDepartureTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
		ExitMotionElapsedSeconds = ResolvedExitDuration;
		SetVisibility(ESlateVisibility::Collapsed);
		ResetDragPickupFeedback();
		SetTickEnabledForMotion(false);
		return;
	}

	SetHoveredForFirstPersonLayer(false);
	ClearGestureState(true);
	ClearInteractionFeedback();
	ClearEnterTransitionPlayback();
	ClearCardDataRewritePlayback();
	ClearEffectBadgeFeedbackPlayback();
	ClearHandTargetImpactPlayback();
	ClearSurfaceDeparturePlayback();
	ClearCardUseReformPlayback();
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
	TargetSlotView.ScreenPosition = bUseSurfaceDeparture
		? VisualSlotView.ScreenPosition
		: VisualSlotView.ScreenPosition + ExitProfile.OffsetPixels;
	TargetSlotView.WidgetPosition = TargetSlotView.ScreenPosition;
	TargetSlotView.SnappedWidgetPosition = TargetSlotView.ScreenPosition;
	TargetSlotView.RenderScale = bUseSurfaceDeparture
		? VisualSlotView.RenderScale
		: FMath::Max(0.01f, VisualSlotView.RenderScale * FMath::Max(0.01f, ExitProfile.ScaleMultiplier));
	TargetSlotView.RenderAngleDegrees = bUseSurfaceDeparture
		? VisualSlotView.RenderAngleDegrees
		: VisualSlotView.RenderAngleDegrees + ExitProfile.AngleOffsetDegrees;
	TargetSlotView.RenderOpacity = bUseSurfaceDeparture ? 1.0f : 0.0f;
	TargetSlotView.bProjected = VisualSlotView.bProjected;
	bIsExitingForFirstPersonLayer = true;
	bUsesFixedExitTransitionPlayback = bUseFixedPlayback;
	PresentationController->State.bUsesSurfaceDepartureExit = bUseSurfaceDeparture;
	PresentationController->State.SurfaceDepartureTransitionKind = bUseSurfaceDeparture
		? TransitionKind
		: EWacomFirstPersonCardSlotTransitionKind::Default;
	ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Exit;
	ExitMotionElapsedSeconds = 0.0f;
	if (bUseSurfaceDeparture)
	{
		ClearExitTransitionPlayback();
		StartSurfaceDeparturePlayback(TransitionKind);
	}
	else if (bUseFixedPlayback)
	{
		StartExitTransitionPlayback(VisualSlotView, TargetSlotView, ExitProfile);
	}
	else
	{
		ClearExitTransitionPlayback();
	}
	ApplyCurrentSlotView();
	ApplyVisualSlotView();
	SetTickEnabledForMotion(true);
}

void FWacomFirstPersonCardSlotPresentationControllerDeleter::operator()(
	FWacomFirstPersonCardSlotPresentationController* Controller) const
{
	delete Controller;
}


bool UWacomFirstPersonCardLayerSlotWidget::IsExitMotionFinished() const
{
	return !PresentationController->State.bHandTargetImpactDeparturePending
		&& bIsExitingForFirstPersonLayer
		&& (PresentationController->State.bUsesSurfaceDepartureExit
			? !IsSurfaceDeparturePlaybackActive()
			: (bUsesFixedExitTransitionPlayback
				? !IsExitTransitionPlaybackActive()
				: ExitMotionElapsedSeconds >= FMath::Max(0.0f, SlotMotionConfig.ExitDuration)));
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
	CancelAllPresentationReadiness();
	SetHoveredForFirstPersonLayer(false);
	ClearGestureState(false);
	ClearInteractionFeedback();
	ClearSurfaceDeparturePlayback();
	ClearCardUseReformPlayback();
	ClearRetainSealPlayback();
	ClearHandTargetImpactPlayback();
	ClearCardDataRewritePlayback();
	ClearEffectBadgeFeedbackPlayback();
	ClearDrawRevealPlayback();
	ClearGainRevealPlayback();
	if (PresentationController)
	{
		PresentationController->ResetOwnedState();
	}
	if (CardDepthMotion)
	{
		CardDepthMotion->Reset();
	}
	if (DragPickupPlayback)
	{
		DragPickupPlayback->Reset();
	}
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
	OnEnterTransitionStartedNative.Clear();
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
	if (bIsExitingForFirstPersonLayer
		&& !bUsesFixedExitTransitionPlayback
		&& !PresentationController->State.bUsesSurfaceDepartureExit)
	{
		ExitMotionElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	}

	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Pressed)
	{
		UpdateGesture(InDeltaTime, GestureRuntime().CurrentScreenPosition);
	}
	UpdatePressedFeedback(InDeltaTime);
	TickRetainSealPlayback(InDeltaTime);
	TickHandTargetImpactPlayback(InDeltaTime);
	if (PresentationController->State.bPendingDataRewriteHandoff
		&& PresentationController->State.bHandTargetImpactDepartureGateReleased
		&& !PresentationController->State.bHandTargetImpactDeparturePending
		&& !bIsExitingForFirstPersonLayer)
	{
		BeginCardDataRewritePlayback(
			PresentationController->State.PendingDataRewriteFieldMask,
			PresentationController->State.PendingDataRewriteTone,
			PresentationController->State.PendingDataRewriteSeed,
			PresentationController->State.PendingDataRewriteSequenceIndex,
			false);
		PresentationController->State.bPendingDataRewriteHandoff = false;
	}
	TickCardDataRewritePlayback(InDeltaTime);
	TickEffectBadgeFeedbackPlayback(InDeltaTime);
	if (PresentationController->State.bHandTargetImpactDeparturePending
		&& IsHandTargetImpactDepartureGateOpen()
		&& !PresentationController->State.bHandTargetImpactDepartureOwnedByPileTransfer)
	{
		const FWacomFirstPersonCardLayerSlotView ExitSlotView = PresentationController->State.DeferredHandTargetExitSlotView;
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfile =
			PresentationController->State.DeferredHandTargetExitProfile;
		const EWacomFirstPersonCardSlotTransitionKind TransitionKind =
			PresentationController->State.DeferredHandTargetExitTransitionKind;
		PresentationController->State.bHandTargetImpactDeparturePending = false;
		BeginExitMotionWithProfile(ExitSlotView, ExitProfile, TransitionKind);
	}
	TickDragPickupFeedback(InDeltaTime);
	bool bNearTarget = true;
	const FWacomFirstPersonCardLayerSlotView PreviousVisualSlotView = VisualSlotView;
	if (IsCardUseReformPlaybackActive())
	{
		TickCardUseReformPlayback(InDeltaTime);
		ApplyVisualSlotView();
	}
	else if (IsSurfaceDeparturePlaybackActive())
	{
		TickSurfaceDeparturePlayback(InDeltaTime);
		ApplyVisualSlotView();
	}
	else if (IsEnterTransitionPlaybackActive())
	{
		bNearTarget = TickEnterTransitionPlayback(InDeltaTime);
	}
	else if (IsExitTransitionPlaybackActive())
	{
		bNearTarget = TickExitTransitionPlayback(InDeltaTime);
	}
	else if (!bIsExitingForFirstPersonLayer || SlotMotionConfig.bEnabled)
	{
		const FWacomFirstPersonCardMotionProfile& ActiveMotionProfile =
			GetMotionProfileForIntent(ActiveMotionIntent);
		const float MotionAlpha = FWacomFirstPersonCardMotionMixer::ComputeMotionAlpha(
			ActiveMotionProfile.MotionSpeed,
			InDeltaTime,
			ActiveMotionProfile.EasePower);
		const float OpacityAlpha = FWacomFirstPersonCardMotionMixer::ComputeMotionAlpha(
			ActiveMotionProfile.OpacitySpeed,
			InDeltaTime,
			ActiveMotionProfile.EasePower);
		const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
		VisualSlotView = FWacomFirstPersonCardMotionMixer::LerpSlotView(
			VisualSlotView,
			EffectiveTargetSlotView,
			MotionAlpha,
			OpacityAlpha);
		ApplyVisualSlotView();

		bNearTarget = FWacomFirstPersonCardMotionMixer::IsNearTarget(
			VisualSlotView,
			EffectiveTargetSlotView);
		if (bNearTarget)
		{
			VisualSlotView = EffectiveTargetSlotView;
			ApplyVisualSlotView();
			if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Idle
				|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Cancelled)
			{
				GestureRuntime().bPreserveReturnMotion = false;
			}
		}
	}
	else
	{
		ApplyVisualSlotView();
	}
	TryStartDeferredDragPickupFeedback();
	UpdateCardDepthMotion(InDeltaTime);

	if (IsExitMotionFinished())
	{
		VisualSlotView.bProjected = false;
		ApplyVisualSlotView();
		UpdateWantsTick();
		return;
	}

	UpdateWantsTick();
	BroadcastVisualSlotUpdatedIfNeeded(PreviousVisualSlotView, VisualSlotView);
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
	ResetCardSurfaceEffectView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCurrentSlotView()
{
	EnsureCardView();
	if (CardView)
	{
		CardView->SetCardViewData(CurrentSlotView.Entry.CardViewData);
		ApplyCardDepthView();
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
		bCardDragProbeFeedback
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

const FWacomFirstPersonCardMotionProfile& UWacomFirstPersonCardLayerSlotWidget::GetMotionProfileForIntent(
	EWacomFirstPersonCardMotionIntent Intent) const
{
	return FWacomFirstPersonCardMotionMixer::GetMotionProfileForIntent(SlotMotionConfig, Intent);
}

EWacomFirstPersonCardMotionIntent UWacomFirstPersonCardLayerSlotWidget::ResolveMotionIntentForPresentationChange(
	const FWacomFirstPersonCardLayerSlotView& PreviousBaseSlotView,
	const FWacomFirstPersonCardLayerSlotView& NewBaseSlotView,
	const FWacomFirstPersonCardLayerSlotView& PreviousPresentationSlotView,
	const FWacomFirstPersonCardLayerSlotView& NewPresentationSlotView,
	EWacomFirstPersonCardMotionIntent PreferredIntent) const
{
	const FWacomFirstPersonCardSlotVisualState PreviousState = ResolveVisualState(PreviousBaseSlotView);
	const FWacomFirstPersonCardSlotVisualState NewState = ResolveVisualState(NewBaseSlotView);
	return FWacomFirstPersonCardMotionMixer::ResolveMotionIntentForPresentationChange(
		PreviousState,
		NewState,
		PreviousPresentationSlotView,
		NewPresentationSlotView,
		PreferredIntent);
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::ComposePresentationSlotView(
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView) const
{
	return FWacomFirstPersonCardMotionMixer::ComposePresentationSlotView(
		BaseSlotView,
		ResolveVisualState(BaseSlotView),
		SlotVisualConfig);
}

void UWacomFirstPersonCardLayerSlotWidget::ApplySlotViewToWidget(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	const EWacomFirstPersonCardDragTargetFeedbackState EffectiveDragTargetFeedbackState =
		ResolveEffectiveDragTargetFeedbackState();
	FWacomFirstPersonCardLocalFeedbackView LocalFeedbackView = InteractionFeedbackPlayback
		? InteractionFeedbackPlayback->BuildLocalFeedbackView()
		: FWacomFirstPersonCardLocalFeedbackView();
	if (PresentationController->RetainSealPlayback && PresentationController->RetainSealPlayback->IsActive())
	{
		const FWacomFirstPersonCardRetainSealPlaybackView RetainView =
			PresentationController->RetainSealPlayback->BuildView();
		LocalFeedbackView.bRetainTransformActive = !RetainView.bReducedMotion;
		LocalFeedbackView.RetainLiftPixels = RetainView.LiftPixels;
		LocalFeedbackView.RetainScaleMultiplier = RetainView.ScaleMultiplier;
	}
	const float DragPickupAlpha = FMath::Clamp(GetDragPickupAlpha(), 0.0f, 1.0f);
	LocalFeedbackView.DragPickupLiftPixels = DragPickupConfig.LiftPixels * DragPickupAlpha;
	LocalFeedbackView.DragPickupScaleMultiplier = FMath::Lerp(
		1.0f,
		DragPickupConfig.ScaleMultiplier,
		DragPickupAlpha);
	LocalFeedbackView.HandTargetImpactScaleMultiplier = HandTargetImpactScaleMultiplier;
	LocalFeedbackView.HandTargetImpactTranslationYPixels = HandTargetImpactTranslationYPixels;
	LocalFeedbackView.HandTargetImpactZOrderBoost = HandTargetImpactZOrderBoost;
	FWacomFirstPersonCardLocalFeedbackMixResult FeedbackMixResult =
		FWacomFirstPersonCardMotionMixer::MixLocalFeedback(SlotView, LocalFeedbackView);
	const FWacomFirstPersonCardUseEffectStyleData& CardUseStyle = SlotVisualConfig.CardUseEffect.Style;
	if (CardUseStyle.EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		&& (IsSurfaceDeparturePlaybackActive() || IsCardUseReformPlaybackActive()))
	{
		const float MinimumHorizontalScale = FMath::Clamp(
			CardUseStyle.EdgeFlipMinimumHorizontalScale, 0.01f, 1.0f);
		const float HorizontalScale = FMath::Lerp(
			1.0f, MinimumHorizontalScale, FMath::Clamp(CardUseFlipProgress, 0.0f, 1.0f));
		const float MotionScale = FMath::Lerp(
			1.0f,
			FMath::Max(1.0f, CardUseStyle.EdgeFlipScaleMultiplier),
			FMath::Clamp(CardUseMotionAlpha, 0.0f, 1.0f));
		FeedbackMixResult.RenderTransform.Scale.X *= HorizontalScale * MotionScale;
		FeedbackMixResult.RenderTransform.Scale.Y *= MotionScale;
		FeedbackMixResult.RenderTransform.Translation.Y -=
			FMath::Max(0.0f, CardUseStyle.EdgeFlipLiftPixels)
			* FMath::Clamp(CardUseMotionAlpha, 0.0f, 1.0f);
		FeedbackMixResult.RenderTransform.Angle = FMath::Lerp(
			FeedbackMixResult.RenderTransform.Angle,
			0.0f,
			FMath::Clamp(CardUseFlipProgress, 0.0f, 1.0f));
	}
	if (PresentationController->DrawRevealPlayback && PresentationController->DrawRevealPlayback->IsActive())
	{
		const FWacomFirstPersonCardDrawRevealPlaybackView RevealView =
			PresentationController->DrawRevealPlayback->BuildView();
		FeedbackMixResult.RenderTransform.Scale.X *=
			RevealView.HorizontalScale * RevealView.LandingScale.X;
		FeedbackMixResult.RenderTransform.Scale.Y *= RevealView.LandingScale.Y;
		FeedbackMixResult.RenderTransform.Translation.Y +=
			RevealView.LandingTranslationYPixels;
	}
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(SlotView.ScreenPosition);
		CanvasSlot->SetZOrder(FeedbackMixResult.ZOrder);
	}

	SetRenderOpacity(FMath::Clamp(
		SlotView.RenderOpacity * CardUseOpacityMultiplier, 0.0f, 1.0f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SetRenderTransform(FeedbackMixResult.RenderTransform);
	ApplyInteractionCue();
	UpdateVisibilityForInteractionMode();
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastVisualSlotUpdatedIfNeeded(
	const FWacomFirstPersonCardLayerSlotView& PreviousVisualSlotView,
	const FWacomFirstPersonCardLayerSlotView& CurrentVisualSlotView)
{
	if (!CurrentSlotView.Entry.CardInstanceId.IsValid())
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
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Inspecting)
	{
		BroadcastDragUpdated();
	}
	if (!bIsHoveredForFirstPersonLayer)
	{
		return;
	}

	FWacomFirstPersonCardLayerSlotView UpdatedSlotView = CurrentVisualSlotView;
	UpdatedSlotView.bIsHovered = true;
	OnCardVisualSlotUpdatedNative.Broadcast(CurrentSlotView.Entry.CardInstanceId, UpdatedSlotView);
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
	InspectSlot.RenderScale =
		FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.CardInspectScale);
	InspectSlot.RenderAngleDegrees = 0.0f;
	InspectSlot.ZOrder = BaseSlotView.ZOrder + 1400;
	InspectSlot.GestureState = GestureRuntime().State;
	return InspectSlot;
}

FWacomFirstPersonCardLayerSlotView UWacomFirstPersonCardLayerSlotWidget::BuildNoTargetDragOverrideSlotView() const
{
	const FWacomFirstPersonCardLayerSlotView& BaseSlotView = GetGestureBaseSlotView();
	FWacomFirstPersonCardLayerSlotView DragSlot = BaseSlotView;
	DragSlot.ScreenPosition = GestureRuntime().CurrentScreenPosition;
	DragSlot.WidgetPosition = DragSlot.ScreenPosition;
	DragSlot.SnappedWidgetPosition = DragSlot.ScreenPosition;
	DragSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.SelectedSourceScale);
	DragSlot.RenderAngleDegrees = 0.0f;
	DragSlot.ZOrder = BaseSlotView.ZOrder + 1400;
	DragSlot.GestureState = GestureRuntime().State;
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
	AimSlot.GestureState = GestureRuntime().State;
	return AimSlot;
}

const FWacomFirstPersonCardLayerSlotView& UWacomFirstPersonCardLayerSlotWidget::GetGestureBaseSlotView() const
{
	return GestureRuntime().StartSlotView.IsSet()
		? GestureRuntime().StartSlotView.GetValue()
		: TargetSlotView;
}

const FWacomFirstPersonCardLayerSlotView& UWacomFirstPersonCardLayerSlotWidget::GetEffectiveTargetSlotView() const
{
	return GestureRuntime().OverrideTargetSlotView.IsSet()
		? GestureRuntime().OverrideTargetSlotView.GetValue()
		: TargetSlotView;
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
	bCardDepthPointerDirty = true;
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearPointerViewportDiagnostics()
{
	bHasPointerViewportPosition = false;
	bCardDepthPointerDirty = true;
	PointerViewportPosition = FVector2D::ZeroVector;
	PointerNormalizedViewportPosition = FVector2D::ZeroVector;
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateCardDepthMotion(float DeltaTime)
{
	if (!CardDepthMotion || !bHasVisualSlotView)
	{
		return;
	}

	FWacomFirstPersonCardDepthMotionInput Input;
	Input.bProjected = VisualSlotView.bProjected;
	Input.bHovered = ResolveVisualState(CurrentSlotView).bHovered;
	const FWacomFirstPersonCardInteractionFeedbackPlaybackView InteractionView =
		InteractionFeedbackPlayback
			? InteractionFeedbackPlayback->BuildView()
			: FWacomFirstPersonCardInteractionFeedbackPlaybackView();
	Input.bPressed = InteractionView.PressedAmount > KINDA_SMALL_NUMBER
		&& !IsFormalDragGestureStateForVisualMotion(GestureRuntime().State);
	Input.PressedFeedbackAmount = InteractionFeedbackConfig.bReduceInteractionMotion
		? 0.0f
		: InteractionView.PressedAmount;
	Input.PressedContactShadowLiftMultiplier =
		InteractionFeedbackConfig.PressedContactShadowLiftMultiplier;
	Input.bDragging = IsFormalDragGestureStateForVisualMotion(GestureRuntime().State);
	Input.bFlattenForSemanticTransition =
		IsEnterTransitionPlaybackActive()
		|| bIsExitingForFirstPersonLayer
		|| IsCardUseReformPlaybackActive();
	Input.bHasPointerPosition = bHasPointerViewportPosition;
	Input.bPointerPositionChanged = bCardDepthPointerDirty;
	Input.PointerPosition = PointerViewportPosition;
	Input.CardCenter = VisualSlotView.ScreenPosition;
	Input.CardBodySize = GetCardBodyHitSizeForFirstPersonLayer();
	Input.CardRenderScale = VisualSlotView.RenderScale;
	Input.CardRenderAngleDegrees = VisualSlotView.RenderAngleDegrees;
	CardDepthMotion->Update(SlotVisualConfig.CardDepth, Input, DeltaTime);
	bCardDepthPointerDirty = false;

	ApplyCardDepthView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCardDepthView()
{
	if (CardView && CardDepthMotion)
	{
		CardView->SetCardDepthView(CardDepthMotion->GetView());
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
		View.InteractionCueAmount = CardViewTestView.InteractionCueAmount;
		View.InteractionCueColor = CardViewTestView.InteractionCueColor;
		View.InteractionCueKind = CardViewTestView.InteractionCueKind;
		View.SelectionView = CardViewTestView.SurfaceEffectView.Selection;
		View.CardUseEffectView = CardViewTestView.SurfaceEffectView.CardUse;
		View.PlayedDissolveView = CardViewTestView.SurfaceEffectView.PlayedDissolve;
		View.HandTargetImpactView = CardViewTestView.SurfaceEffectView.HandTargetImpact;
		View.DrawRevealView = CardViewTestView.SurfaceEffectView.DrawReveal;
		View.GainRevealView = CardViewTestView.SurfaceEffectView.GainReveal;
		View.DataRewriteView = CardViewTestView.DataRewriteView;
	}
	else
	{
		if (PresentationController->HandTargetImpactPlayback && PresentationController->HandTargetImpactPlayback->IsActive())
		{
			View.HandTargetImpactView = WacomFirstPersonCardSurfaceEffectViewBuilder::BuildHandTargetImpactView(
				PresentationController->HandTargetImpactPlayback->BuildView(),
				SlotVisualConfig,
				CurrentSlotView.Entry.CardInstanceId).HandTargetImpact;
		}
		if (PresentationController->DataRewritePlayback && PresentationController->DataRewritePlayback->IsActive())
		{
			View.DataRewriteView = WacomFirstPersonCardSurfaceEffectViewBuilder::BuildDataRewriteView(
				PresentationController->DataRewritePlayback->BuildView(),
				SlotVisualConfig);
		}
		if (PresentationController->DrawRevealPlayback && PresentationController->DrawRevealPlayback->IsActive())
		{
			View.DrawRevealView = WacomFirstPersonCardSurfaceEffectViewBuilder::BuildDrawRevealView(
				PresentationController->DrawRevealPlayback->BuildView(),
				SlotVisualConfig).DrawReveal;
		}
		if (PresentationController->GainRevealPlayback && PresentationController->GainRevealPlayback->IsActive())
		{
			View.GainRevealView = WacomFirstPersonCardSurfaceEffectViewBuilder::BuildGainRevealView(
				PresentationController->GainRevealPlayback->BuildView(),
				SlotVisualConfig,
				CurrentSlotView).GainReveal;
		}
	}
	View.RenderTransform = GetRenderTransform();
	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		View.RenderZOrder = CanvasSlot->GetZOrder();
	}
	View.bDragPickupFeedbackActive = DragPickupPlayback && DragPickupPlayback->IsActive();
	View.DragPickupAlpha = GetDragPickupAlpha();
	View.DragPickupTriggerCount = DragPickupTriggerCountForTest;
	View.DragPickupSoundRequestCount = DragPickupSoundRequestCountForTest;
	View.LastDragPickupSoundPitchMultiplier = LastDragPickupSoundPitchMultiplierForTest;
	View.bCardUseEffectPlaybackActive = IsSurfaceDeparturePlaybackActive()
		&& PresentationController->State.SurfaceDepartureTransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played;
	View.bPlayedDissolvePlaybackActive = IsSurfaceDeparturePlaybackActive()
		&& PresentationController->State.SurfaceDepartureTransitionKind == EWacomFirstPersonCardSlotTransitionKind::Exhausted;
	View.bCardUseReformPlaybackActive = IsCardUseReformPlaybackActive();
	View.bCardUseReformUsingTargetSlot = IsCardUseReformPlaybackActive()
		&& PresentationController->CardUseReformPlayback->BuildView().bUseTargetSlotPosition;
	View.bHandTargetImpactCommitActive = IsHandTargetImpactCommitPlaybackActive();
	View.bDataRewritePlaybackActive = IsCardDataRewritePlaybackActive();
	View.bDataRewritePendingHandoff = PresentationController->State.bPendingDataRewriteHandoff;
	View.bEffectBadgeFeedbackPlaybackActive = IsEffectBadgeFeedbackPlaybackActive();
	if (PresentationController->EffectBadgeFeedbackPlayback && PresentationController->EffectBadgeFeedbackPlayback->IsActive())
	{
		View.EffectBadgeFeedbackView = PresentationController->EffectBadgeFeedbackPlayback->BuildView();
	}
	View.bDrawRevealPlaybackActive = IsDrawRevealPlaybackActive();
	if (PresentationController->DrawRevealPlayback && PresentationController->DrawRevealPlayback->IsActive())
	{
		const FWacomFirstPersonCardDrawRevealPlaybackView RevealView =
			PresentationController->DrawRevealPlayback->BuildView();
		View.bDrawRevealWaiting = RevealView.Phase
			== EWacomFirstPersonCardDrawRevealPhase::Waiting;
		View.DrawRevealProgress = RevealView.Progress;
		View.DrawRevealHorizontalScale = RevealView.HorizontalScale;
		View.DrawRevealLandingScale = RevealView.LandingScale;
		View.DrawRevealLandingTranslationYPixels =
			RevealView.LandingTranslationYPixels;
	}
	View.bGainRevealPlaybackActive = IsGainRevealPlaybackActive();
	if (PresentationController->GainRevealPlayback && PresentationController->GainRevealPlayback->IsActive())
	{
		const FWacomFirstPersonCardGainRevealPlaybackView RevealView =
			PresentationController->GainRevealPlayback->BuildView();
		View.bGainRevealWaiting = RevealView.Phase
			== EWacomFirstPersonCardGainRevealPhase::Waiting;
		View.GainRevealProgress = RevealView.Progress;
	}
	View.bHandTargetDeparturePending = PresentationController->State.bHandTargetImpactDeparturePending;
	View.bHandTargetDepartureGateOpen = IsHandTargetImpactDepartureGateOpen();
	View.HandTargetImpactZOrderBoost = HandTargetImpactZOrderBoost;
	View.CardUseEffectSoundRequestCount = CardUseEffectSoundRequestCountForTest;
	View.CardUseReformSoundRequestCount = CardUseReformSoundRequestCountForTest;
	View.LastCardUseEffectSoundPitchMultiplier = LastCardUseEffectSoundPitchMultiplierForTest;
	View.CardUseFlipProgress = CardUseFlipProgress;
	View.CardUseImpactProgress = CardUseImpactProgress;
	View.CardUseHorizontalScaleMultiplier = SlotVisualConfig.CardUseEffect.Style.EffectKind
		== EWacomFirstPersonCardUseEffectKind::EdgeFlip
		? FMath::Lerp(
			1.0f,
			FMath::Clamp(SlotVisualConfig.CardUseEffect.Style.EdgeFlipMinimumHorizontalScale, 0.01f, 1.0f),
			FMath::Clamp(CardUseFlipProgress, 0.0f, 1.0f))
		: 1.0f;
	View.CardUseOpacityMultiplier = CardUseOpacityMultiplier;
	View.PlayedDissolveSoundRequestCount = PlayedDissolveSoundRequestCountForTest;
	View.LastPlayedDissolveSoundPitchMultiplier = LastPlayedDissolveSoundPitchMultiplierForTest;
	View.GestureSource = GestureRuntime().Source;
	const FWacomFirstPersonCardInteractionFeedbackPlaybackView InteractionView =
		InteractionFeedbackPlayback
			? InteractionFeedbackPlayback->BuildView()
			: FWacomFirstPersonCardInteractionFeedbackPlaybackView();
	View.bPressed = InteractionView.bPressed;
	View.PressedFeedbackAmount = InteractionView.PressedAmount;
	View.bDenyFeedbackActive = InteractionView.bDenyActive;
	View.bCommitFeedbackActive = InteractionView.bCommitActive;
	View.bRetainedFeedbackActive = IsRetainedFeedbackActive();
	if (PresentationController->RetainSealPlayback && PresentationController->RetainSealPlayback->IsActive())
	{
		const FWacomFirstPersonCardRetainSealPlaybackView RetainView =
			PresentationController->RetainSealPlayback->BuildView();
		View.bRetainedFeedbackHeld = RetainView.Phase
			== EWacomFirstPersonCardRetainSealPhase::Held;
		View.bRetainedFeedbackBlocking = RetainView.bBlocksPresentation;
		View.RetainSealPhase = RetainView.Phase;
		View.RetainSealProgress = RetainView.PhaseProgress;
		View.RetainSealLiftPixels = RetainView.LiftPixels;
		View.RetainSealScaleMultiplier = RetainView.ScaleMultiplier;
	}
	View.bCardDragProbeFeedback = bCardDragProbeFeedback;
	View.bCardDragTargetAffordanceFeedback = bCardDragTargetAffordanceFeedback;
	View.bCardDragTargetFocusActive =
		bCardDragProbeFeedback
		&& IsCardTargetFocusFeedbackState(CardDragTargetFocusFeedbackState);
	View.CardDragTargetAffordanceFeedbackState = CardDragTargetAffordanceFeedbackState;
	View.CardDragTargetFocusFeedbackState = CardDragTargetFocusFeedbackState;
	View.DirectDragTargetFeedbackState = DirectDragTargetFeedbackState;
	View.DragTargetFeedbackState = ResolveEffectiveDragTargetFeedbackState();
	View.ActiveMotionIntent = ActiveMotionIntent;
	if (CardDepthMotion)
	{
		View.CardDepthView = CardDepthMotion->GetView();
	}
	View.bEnterTransitionPlaybackActive = IsEnterTransitionPlaybackActive();
	View.bEnterTransitionBlocksInteraction = IsEnterTransitionBlockingInteraction();
	View.EnterTransitionElapsedSeconds = IsEnterTransitionPlaybackActive()
		? PresentationController->TransitionPlayback->GetElapsedSeconds()
		: 0.0f;
	View.EnterTransitionStartDelaySeconds = IsEnterTransitionPlaybackActive()
		? PresentationController->TransitionPlayback->GetStartDelaySeconds()
		: 0.0f;
	View.EnterTransitionDurationSeconds = IsEnterTransitionPlaybackActive()
		? PresentationController->TransitionPlayback->GetDurationSeconds()
		: 0.0f;
	View.bExitTransitionPlaybackActive = IsExitTransitionPlaybackActive();
	View.ExitTransitionElapsedSeconds = IsExitTransitionPlaybackActive()
		? PresentationController->TransitionPlayback->GetElapsedSeconds()
		: 0.0f;
	View.ExitTransitionStartDelaySeconds = IsExitTransitionPlaybackActive()
		? PresentationController->TransitionPlayback->GetStartDelaySeconds()
		: 0.0f;
	View.ExitTransitionDurationSeconds = IsExitTransitionPlaybackActive()
		? PresentationController->TransitionPlayback->GetDurationSeconds()
		: 0.0f;
	View.EnterTransitionSoundRequestCount = EnterTransitionSoundRequestCountForTest;
	View.LastEnterTransitionSoundKind = LastEnterTransitionSoundKindForTest;
	View.SlotRuntimeConfig.Motion = SlotMotionConfig;
	View.SlotRuntimeConfig.Visual = SlotVisualConfig;
	View.SlotRuntimeConfig.Interaction = InteractionFeedbackConfig;
	View.SlotRuntimeConfig.DragPickup = DragPickupConfig;
	View.SlotRuntimeConfig.Drag = CardDragConfig;
	View.SlotRuntimeConfigApplyCount = SlotRuntimeConfigApplyCountForTest;
	View.SurfaceReadinessState = PresentationController
		? static_cast<int32>(PresentationController->Readiness.GetState(
			EWacomFirstPersonCardPresentationReadinessChannel::Surface))
		: 0;
	View.CostDigitReadinessState = PresentationController
		? static_cast<int32>(PresentationController->Readiness.GetState(
			EWacomFirstPersonCardPresentationReadinessChannel::CostDigit))
		: 0;
	View.EffectBadgeReadinessState = PresentationController
		? static_cast<int32>(PresentationController->Readiness.GetState(
			EWacomFirstPersonCardPresentationReadinessChannel::EffectBadge))
		: 0;
	View.SurfaceReadinessGeneration = PresentationController
		? PresentationController->Readiness.GetGeneration(
			EWacomFirstPersonCardPresentationReadinessChannel::Surface)
		: 0;
	View.CostDigitReadinessGeneration = PresentationController
		? PresentationController->Readiness.GetGeneration(
			EWacomFirstPersonCardPresentationReadinessChannel::CostDigit)
		: 0;
	View.EffectBadgeReadinessGeneration = PresentationController
		? PresentationController->Readiness.GetGeneration(
			EWacomFirstPersonCardPresentationReadinessChannel::EffectBadge)
		: 0;
	View.bPlaybackFrozenForReadiness = PresentationController->State.bPlaybackFrozenForReadiness;
	View.PresentationReadinessTimeoutCount = PresentationReadinessTimeoutCountForTest;
	View.PresentationReadinessFallbackCount = PresentationReadinessFallbackCountForTest;
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
	if (GestureRuntime().State != EWacomFirstPersonCardGestureState::Idle
		&& GestureRuntime().State != EWacomFirstPersonCardGestureState::Cancelled)
	{
		return;
	}

	UpdateBodyHoverFromLocalPosition(LocalPosition);
}

void UWacomFirstPersonCardLayerSlotWidget::SetCardDepthPointerPositionForTest(
	const FVector2D& WidgetPosition)
{
	UpdatePointerViewportDiagnostics(WidgetPosition);
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
	if (GestureRuntime().State == EWacomFirstPersonCardGestureState::Idle
		|| GestureRuntime().State == EWacomFirstPersonCardGestureState::Cancelled)
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

	if (GestureRuntime().State != EWacomFirstPersonCardGestureState::Idle)
	{
		return ReleaseGesture(GestureRuntime().CurrentScreenPosition);
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
		InteractionFeedbackPlayback && InteractionFeedbackPlayback->IsActive();
	const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
	const bool bGestureActive =
		GestureRuntime().State != EWacomFirstPersonCardGestureState::Idle
		&& GestureRuntime().State != EWacomFirstPersonCardGestureState::Cancelled;
	FWacomFirstPersonCardSlotPresentationActivityInput PresentationActivityInput;
	PresentationActivityInput.bSlotExiting = bIsExitingForFirstPersonLayer;
	const FWacomFirstPersonCardSlotPresentationActivityView PresentationActivity =
		PresentationController
		? PresentationController->BuildActivityView(PresentationActivityInput)
		: FWacomFirstPersonCardSlotPresentationActivityView();
	bWantsSlotMotionTick = PresentationActivity.bNeedsTick
		|| (bHasVisualSlotView
			&& (FVector2D::Distance(VisualSlotView.ScreenPosition, EffectiveTargetSlotView.ScreenPosition) > 0.1f
				|| FMath::Abs(VisualSlotView.RenderAngleDegrees - EffectiveTargetSlotView.RenderAngleDegrees) > 0.05f
				|| FMath::Abs(VisualSlotView.RenderScale - EffectiveTargetSlotView.RenderScale) > 0.001f
				|| FMath::Abs(VisualSlotView.RenderOpacity - EffectiveTargetSlotView.RenderOpacity) > 0.01f
			|| VisualSlotView.ZOrder != EffectiveTargetSlotView.ZOrder))
		|| bFeedbackActive
		|| bGestureActive
		|| (DragPickupPlayback && DragPickupPlayback->IsActive())
		|| (CardDepthMotion && CardDepthMotion->IsInMotion());
}
