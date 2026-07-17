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
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerConfigUtils.h"
#include "UI/Card/WacomFirstPersonCardDataRewritePlayback.h"
#include "UI/Card/WacomFirstPersonCardEffectBadgeFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardDepthMotion.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardGainRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardRetainSealPlayback.h"
#include "UI/Card/WacomFirstPersonCardDragPickupPlayback.h"
#include "UI/Card/WacomFirstPersonCardHandTargetImpactPlayback.h"
#include "UI/Card/WacomFirstPersonCardMotionMixer.h"
#include "UI/Card/WacomFirstPersonCardPresentationReadinessGate.h"
#include "UI/Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"
#include "UI/Card/WacomFirstPersonCardUseReformPlayback.h"
#include "UI/Card/WacomFirstPersonCardTransitionPlayback.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

namespace
{
	DEFINE_LOG_CATEGORY_STATIC(LogWacomCardPresentationReadiness, Log, All);

	const FName SurfaceEffectHandTargetImpact(TEXT("HandTargetImpact"));
	const FName SurfaceEffectDrawReveal(TEXT("DrawReveal"));
	const FName SurfaceEffectGainReveal(TEXT("GainReveal"));
	const FName SurfaceEffectRetainSeal(TEXT("RetainSeal"));
	const FName SurfaceEffectDeparture(TEXT("SurfaceDeparture"));
	const FName SurfaceEffectCardUseReform(TEXT("CardUseReform"));

	float ComputeFeedbackPulseAlpha(float ElapsedSeconds, float DurationSeconds)
	{
		if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds)
		{
			return 0.0f;
		}

		return 1.0f - FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
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

	FWacomFirstPersonCardSurfaceEffectView BuildSurfaceDepartureView(
		const FWacomFirstPersonCardSurfaceDepartureTickResult& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		if (PlaybackView.Kind == EWacomFirstPersonCardSurfaceDepartureKind::CardUse)
		{
			SurfaceView.CardUse.bActive = true;
			SurfaceView.CardUse.bReducedMotion = PlaybackView.bReducedMotion;
			SurfaceView.CardUse.Amount = PlaybackView.Amount;
			SurfaceView.CardUse.FlipProgress = PlaybackView.FlipProgress;
			SurfaceView.CardUse.ImpactProgress = PlaybackView.ImpactProgress;
			SurfaceView.CardUse.TimeSeconds = PlaybackView.TimeSeconds;
			SurfaceView.CardUse.Style = VisualConfig.CardUseEffect.Style;
		}
		else if (PlaybackView.Kind == EWacomFirstPersonCardSurfaceDepartureKind::ExhaustDissolve)
		{
			SurfaceView.PlayedDissolve.bActive = true;
			SurfaceView.PlayedDissolve.bReducedMotion = PlaybackView.bReducedMotion;
			SurfaceView.PlayedDissolve.Amount = PlaybackView.Amount;
			SurfaceView.PlayedDissolve.TimeSeconds = PlaybackView.TimeSeconds;
			SurfaceView.PlayedDissolve.Seed = PlaybackView.Seed;
			SurfaceView.PlayedDissolve.Style = VisualConfig.PlayedDissolve.Style;
		}
		return SurfaceView;
	}

	FWacomFirstPersonCardSurfaceEffectView BuildCardUseReformView(
		const FWacomFirstPersonCardUseReformTickResult& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.CardUse.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardUseReformPhase::Inactive;
		SurfaceView.CardUse.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.CardUse.Amount = PlaybackView.Amount;
		SurfaceView.CardUse.FlipProgress = PlaybackView.FlipProgress;
		SurfaceView.CardUse.ImpactProgress = PlaybackView.ImpactProgress;
		SurfaceView.CardUse.TimeSeconds = PlaybackView.TimeSeconds;
		SurfaceView.CardUse.Style = VisualConfig.CardUseEffect.Style;
		return SurfaceView;
	}

	FWacomFirstPersonCardSurfaceEffectView BuildHandTargetImpactView(
		const FWacomFirstPersonCardHandTargetImpactPlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig,
		const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.HandTargetImpact.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardHandTargetImpactPhase::Inactive;
		SurfaceView.HandTargetImpact.bPreview =
			PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering
			|| PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain
			|| PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewExiting;
		SurfaceView.HandTargetImpact.bCommitted =
			PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::Commit;
		SurfaceView.HandTargetImpact.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.HandTargetImpact.PreviewAmount = PlaybackView.PreviewAmount;
		SurfaceView.HandTargetImpact.CommitProgress = PlaybackView.CommitProgress;
		SurfaceView.HandTargetImpact.TimeSeconds = PlaybackView.TimeSeconds;
		SurfaceView.HandTargetImpact.Seed =
			static_cast<float>(GetTypeHash(CardInstanceId) & 0xFFFFu) / 65535.0f;
		SurfaceView.HandTargetImpact.Style = VisualConfig.HandTargetImpact.Style;
		return SurfaceView;
	}

	FWacomFirstPersonCardDataRewriteView BuildDataRewriteView(
		const FWacomFirstPersonCardDataRewritePlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardDataRewriteView RewriteView;
		RewriteView.bActive = PlaybackView.bActive;
		RewriteView.bReducedMotion = PlaybackView.bReducedMotion;
		RewriteView.FieldMask = PlaybackView.FieldMask;
		RewriteView.Tone = PlaybackView.Tone;
		RewriteView.Progress = PlaybackView.Progress;
		RewriteView.OldDissolveAmount = PlaybackView.OldDissolveAmount;
		RewriteView.NewRevealAmount = PlaybackView.NewRevealAmount;
		RewriteView.DigitScale = PlaybackView.DigitScale;
		RewriteView.Seed = PlaybackView.Seed;
		RewriteView.Style = VisualConfig.DataRewrite.Style;
		return RewriteView;
	}

	FWacomFirstPersonCardSurfaceEffectView BuildDrawRevealView(
		const FWacomFirstPersonCardDrawRevealPlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.DrawReveal.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardDrawRevealPhase::Inactive;
		SurfaceView.DrawReveal.bWaiting =
			PlaybackView.Phase == EWacomFirstPersonCardDrawRevealPhase::Waiting;
		SurfaceView.DrawReveal.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.DrawReveal.Progress = PlaybackView.Progress;
		SurfaceView.DrawReveal.Style = VisualConfig.DrawReveal.Style;
		return SurfaceView;
	}

	EWacomFirstPersonCardGainRevealRarity ResolveGainRevealRarity(
		const FGameplayTag& RarityTag)
	{
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_White))
		{
			return EWacomFirstPersonCardGainRevealRarity::White;
		}
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_Blue))
		{
			return EWacomFirstPersonCardGainRevealRarity::Blue;
		}
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_Yellow))
		{
			return EWacomFirstPersonCardGainRevealRarity::Yellow;
		}
		if (RarityTag.MatchesTagExact(WacomTags::Card_Rarity_Purple))
		{
			return EWacomFirstPersonCardGainRevealRarity::Purple;
		}
		return EWacomFirstPersonCardGainRevealRarity::Neutral;
	}

	FWacomFirstPersonCardSurfaceEffectView BuildGainRevealView(
		const FWacomFirstPersonCardGainRevealPlaybackView& PlaybackView,
		const FWacomFirstPersonCardSlotVisualConfig& VisualConfig,
		const FWacomFirstPersonCardLayerSlotView& SlotView)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.GainReveal.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardGainRevealPhase::Inactive;
		SurfaceView.GainReveal.bWaiting =
			PlaybackView.Phase == EWacomFirstPersonCardGainRevealPhase::Waiting;
		SurfaceView.GainReveal.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.GainReveal.Progress = PlaybackView.Progress;
		SurfaceView.GainReveal.Seed =
			static_cast<float>(GetTypeHash(SlotView.Entry.CardInstanceId) & 0xFFFFu)
			/ 65535.0f;
		SurfaceView.GainReveal.Rarity = ResolveGainRevealRarity(
			SlotView.Entry.CardViewData.Rarity);
		SurfaceView.GainReveal.Style = VisualConfig.GainReveal.Style;
		return SurfaceView;
	}

	FWacomFirstPersonCardSurfaceEffectView BuildRetainSealView(
		const FWacomFirstPersonCardRetainSealPlaybackView& PlaybackView)
	{
		FWacomFirstPersonCardSurfaceEffectView SurfaceView;
		SurfaceView.RetainSeal.bActive =
			PlaybackView.Phase != EWacomFirstPersonCardRetainSealPhase::Inactive;
		SurfaceView.RetainSeal.bReducedMotion = PlaybackView.bReducedMotion;
		SurfaceView.RetainSeal.Phase = PlaybackView.Phase;
		SurfaceView.RetainSeal.Progress = PlaybackView.PhaseProgress;
		SurfaceView.RetainSeal.Seed = PlaybackView.Seed;
		SurfaceView.RetainSeal.Style = PlaybackView.Style;
		return SurfaceView;
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
	}

	CurrentSlotView = InSlotView;
	bHasVisualSlotView = true;
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	VisualSlotView = TargetSlotView;
	ActiveMotionIntent = EWacomFirstPersonCardMotionIntent::Layout;
	bIsExitingForFirstPersonLayer = false;
	bUsesFixedExitTransitionPlayback = false;
	bUsesSurfaceDepartureExit = false;
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
	, TransitionPlayback(new FWacomFirstPersonCardTransitionPlayback())
	, CardDepthMotion(new FWacomFirstPersonCardDepthMotion())
	, SurfaceReadinessGate(new FWacomFirstPersonCardPresentationReadinessGate())
	, CostDigitReadinessGate(new FWacomFirstPersonCardPresentationReadinessGate())
	, EffectBadgeReadinessGate(new FWacomFirstPersonCardPresentationReadinessGate())
{
}

UWacomFirstPersonCardLayerSlotWidget::~UWacomFirstPersonCardLayerSlotWidget() = default;

void FWacomFirstPersonCardTransitionPlaybackDeleter::operator()(
	FWacomFirstPersonCardTransitionPlayback* Playback) const
{
	delete Playback;
}

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

void FWacomFirstPersonCardSurfaceDeparturePlaybackDeleter::operator()(
	FWacomFirstPersonCardSurfaceDeparturePlayback* Playback) const
{
	delete Playback;
}

void FWacomFirstPersonCardUseReformPlaybackDeleter::operator()(
	FWacomFirstPersonCardUseReformPlayback* Playback) const
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
	bUsesSurfaceDepartureExit = false;
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
		bUsesSurfaceDepartureExit = false;
		SurfaceDepartureTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
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
	bUsesSurfaceDepartureExit = bUseSurfaceDeparture;
	SurfaceDepartureTransitionKind = bUseSurfaceDeparture
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
	if (IsCardDataRewritePlaybackActive() || bPendingDataRewriteHandoff)
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
#if WITH_AUTOMATION_TESTS
	++SlotVisualConfigApplyCountForTest;
#endif
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	if (!IsHandTargetImpactPlaybackActive()
		&& !IsCardDataRewritePlaybackActive()
		&& !IsRetainSealPlaybackActive())
	{
		ResetCardSurfaceEffectView();
	}
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
	if (DragPickupPlayback && DragPickupPlayback->IsActive())
	{
		ResetDragPickupFeedback();
	}
	if (!SlotFeedbackConfig.bEnabled)
	{
		ClearInteractionFeedback();
	}
	if (!SlotFeedbackConfig.bEnabled || !SlotFeedbackConfig.bEnableRetainedFeedback)
	{
		ClearRetainSealPlayback();
	}
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void FWacomFirstPersonCardPresentationReadinessGateDeleter::operator()(
	FWacomFirstPersonCardPresentationReadinessGate* Gate) const
{
	delete Gate;
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
	DirectDragTargetFeedbackState = FeedbackState;
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
	const bool bEnableFeedback = FeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None;
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
	const bool bEnableFeedback = FeedbackState != EWacomFirstPersonCardDragTargetFeedbackState::None;
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
	if (bValidFeedback)
	{
		BeginHandTargetImpactPreview();
	}
	else
	{
		EndHandTargetImpactPreview();
	}
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
	EndHandTargetImpactPreview();
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
	return !bHandTargetImpactDeparturePending
		&& bIsExitingForFirstPersonLayer
		&& (bUsesSurfaceDepartureExit
			? !IsSurfaceDeparturePlaybackActive()
			: (bUsesFixedExitTransitionPlayback
				? !IsExitTransitionPlaybackActive()
				: ExitMotionElapsedSeconds >= FMath::Max(0.0f, SlotMotionConfig.ExitDuration)));
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
	TransitionPlayback.Reset();
	CardDepthMotion.Reset();
	DragPickupPlayback.Reset();
	SurfaceDeparturePlayback.Reset();
	CardUseReformPlayback.Reset();
	HandTargetImpactPlayback.Reset();
	DataRewritePlayback.Reset();
	EffectBadgeFeedbackPlayback.Reset();
	DrawRevealPlayback.Reset();
	GainRevealPlayback.Reset();
	RetainSealPlayback.Reset();
	SurfaceReadinessGate.Reset();
	CostDigitReadinessGate.Reset();
	EffectBadgeReadinessGate.Reset();
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
		&& !bUsesSurfaceDepartureExit)
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
	TickRetainSealPlayback(InDeltaTime);
	TickHandTargetImpactPlayback(InDeltaTime);
	if (bPendingDataRewriteHandoff
		&& bHandTargetImpactDepartureGateReleased
		&& !bHandTargetImpactDeparturePending
		&& !bIsExitingForFirstPersonLayer)
	{
		BeginCardDataRewritePlayback(
			PendingDataRewriteFieldMask,
			PendingDataRewriteTone,
			PendingDataRewriteSeed,
			PendingDataRewriteSequenceIndex,
			false);
		bPendingDataRewriteHandoff = false;
	}
	TickCardDataRewritePlayback(InDeltaTime);
	TickEffectBadgeFeedbackPlayback(InDeltaTime);
	if (bHandTargetImpactDeparturePending
		&& IsHandTargetImpactDepartureGateOpen()
		&& !bHandTargetImpactDepartureOwnedByPileTransfer)
	{
		const FWacomFirstPersonCardLayerSlotView ExitSlotView = DeferredHandTargetExitSlotView;
		const TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfile =
			DeferredHandTargetExitProfile;
		const EWacomFirstPersonCardSlotTransitionKind TransitionKind =
			DeferredHandTargetExitTransitionKind;
		bHandTargetImpactDeparturePending = false;
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
	TryStartDeferredDragPickupFeedback();
	UpdateCardDepthMotion(InDeltaTime);

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
	FWacomFirstPersonCardLocalFeedbackMixInput FeedbackMixInput;
	FeedbackMixInput.SlotView = &SlotView;
	FeedbackMixInput.FeedbackConfig = &SlotFeedbackConfig;
	FeedbackMixInput.DenyFeedbackElapsedSeconds = DenyFeedbackElapsedSeconds;
	if (RetainSealPlayback && RetainSealPlayback->IsActive())
	{
		const FWacomFirstPersonCardRetainSealPlaybackView RetainView =
			RetainSealPlayback->BuildView();
		FeedbackMixInput.bRetainTransformActive = !RetainView.bReducedMotion;
		FeedbackMixInput.RetainLiftPixels = RetainView.LiftPixels;
		FeedbackMixInput.RetainScaleMultiplier = RetainView.ScaleMultiplier;
	}
	FeedbackMixInput.DragPickupAlpha = GetDragPickupAlpha();
	FeedbackMixInput.HandTargetImpactScaleMultiplier = HandTargetImpactScaleMultiplier;
	FeedbackMixInput.HandTargetImpactTranslationYPixels = HandTargetImpactTranslationYPixels;
	FeedbackMixInput.HandTargetImpactZOrderBoost = HandTargetImpactZOrderBoost;
	FeedbackMixInput.bPressed = bIsPressedForFirstPersonLayer;
	FeedbackMixInput.bCommitFeedbackActive =
		CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration;
	FWacomFirstPersonCardLocalFeedbackMixResult FeedbackMixResult =
		FWacomFirstPersonCardMotionMixer::MixLocalFeedback(FeedbackMixInput);
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
	if (DrawRevealPlayback && DrawRevealPlayback->IsActive())
	{
		const FWacomFirstPersonCardDrawRevealPlaybackView RevealView =
			DrawRevealPlayback->BuildView();
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
	ApplyFeedbackOverlay();
	ApplyInteractionFeedbackOverlay();
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
	if (GestureState == EWacomFirstPersonCardGestureState::Inspecting)
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

bool UWacomFirstPersonCardLayerSlotWidget::CanInteractWithCurrentSlot() const
{
	return bCardLayerInteractionEnabled
		&& !IsEnterTransitionBlockingInteraction()
		&& !IsCardUseReformPlaybackActive()
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
	return CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::CommitNoTarget
		|| CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsTargetedAimCard() const
{
	return CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::AimWorldTarget
		|| CurrentSlotView.Entry.InteractionIntent
			== EWacomFirstPersonCardInteractionIntent::AimCardTarget;
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
	DragSlot.RenderScale = FMath::Max(0.01f, BaseSlotView.RenderScale * CardDragConfig.SelectedSourceScale);
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

bool UWacomFirstPersonCardLayerSlotWidget::ResolveInspectScreenPosition(
	FVector2D& OutScreenPosition) const
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
	bool bSuppressInspectDragPromotion,
	bool bBroadcastDragUpdate)
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
		else if (GestureElapsedSeconds >= CardDragConfig.CardInspectHoldDelaySeconds)
		{
			SetGestureState(EWacomFirstPersonCardGestureState::Inspecting, true);
		}
	}
	else if (GestureState == EWacomFirstPersonCardGestureState::Inspecting
		&& !bSuppressInspectDragPromotion
		&& CanStartCardDragGesture()
		&& CurrentSlotView.Entry.InteractionIntent
			!= EWacomFirstPersonCardInteractionIntent::InspectOnly
		&& DragDistance >= CardDragConfig.CardDragStartThresholdPixels)
	{
		PromoteGestureToCardDrag(true);
	}

	const bool bBattleNoTargetCommit =
		CurrentSlotView.Entry.InteractionIntent
		== EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
	if (bBattleNoTargetCommit
		&& (GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			|| GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit))
	{
		const bool bNowArmed =
			ComputeNoTargetDragOutDistance()
			>= CardDragConfig.NoTargetCardDragOutCommitDistancePixels;
		SetGestureState(
			bNowArmed
				? EWacomFirstPersonCardGestureState::ArmedForCommit
				: EWacomFirstPersonCardGestureState::DraggingNoTargetCard,
			false);
	}

	UpdateGestureOverrideTarget();
	if (bBroadcastDragUpdate)
	{
		BroadcastDragUpdated();
	}
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
	UpdateGesture(0.0f, ScreenPosition, bSuppressInspectDragPromotion, false);

	const EWacomFirstPersonCardGestureState ReleaseState = GestureState;
	// Release delegates synchronously submit commands and may refresh the entire
	// card layer. Capture the semantic outcome before broadcasting so that a
	// successful refresh cannot clear bGestureTargetValid and turn acceptance
	// into a false Deny pulse on return.
	const bool bAcceptedRelease =
		ReleaseState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| (ReleaseState == EWacomFirstPersonCardGestureState::AimingTargetedCard
			&& bGestureTargetValid);
	const bool bNeutralRelease =
		ReleaseState == EWacomFirstPersonCardGestureState::Inspecting
		|| ReleaseState == EWacomFirstPersonCardGestureState::Pressed
		|| (ReleaseState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
			&& CurrentSlotView.Entry.InteractionIntent
				== EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
	SetPressedForFirstPersonLayer(false);
	BroadcastDragReleased();

	if (bAcceptedRelease)
	{
		TriggerConfirmFeedback();
	}
	else if (bNeutralRelease)
	{
		// Inspect/press is a neutral return. Run drag-to-drop owns its own acceptance feedback.
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
	const bool bWasFormalDrag = IsFormalDragGestureState(PreviousState);
	const bool bIsFormalDrag = IsFormalDragGestureState(NewState);
	if (!bWasFormalDrag && bIsFormalDrag)
	{
		SetPressedForFirstPersonLayer(false);
		BeginDragPickupFeedback();
	}
	else if (bWasFormalDrag && !bIsFormalDrag)
	{
		ResetDragPickupFeedback();
	}
	UpdateWantsTick();

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
	ResetDragPickupFeedback();
	RefreshPresentationTarget(true, EWacomFirstPersonCardMotionIntent::Layout);
	ApplyVisualSlotView();
	UpdateWantsTick();
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

float UWacomFirstPersonCardLayerSlotWidget::ComputeNoTargetDragOutDistance() const
{
	switch (CardDragConfig.NoTargetCardDragOutDirection)
	{
	case EWacomFirstPersonCardDragOutDirection::Up:
	default:
		return FMath::Max(0.0f, PressScreenPosition.Y - CurrentGestureScreenPosition.Y);
	}
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
	Input.bPressed = bIsPressedForFirstPersonLayer && !IsFormalDragGestureState(GestureState);
	Input.bDragging = IsFormalDragGestureState(GestureState);
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

void UWacomFirstPersonCardLayerSlotWidget::BeginDragPickupFeedback()
{
	if (!DragPickupPlayback)
	{
		DragPickupPlayback.Reset(new FWacomFirstPersonCardDragPickupPlayback());
	}

	const bool bIsFarKeyboardNoTargetDrag =
		GestureSource == EWacomFirstPersonCardGestureSource::KeyboardShortcut
		&& GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		&& GestureOverrideTargetSlotView.IsSet()
		&& FVector2D::Distance(
			VisualSlotView.ScreenPosition,
			GestureOverrideTargetSlotView->ScreenPosition)
			> FMath::Max(1.0f, CardDragConfig.CardDragStartThresholdPixels);
	DragPickupPlayback->Begin(SlotFeedbackConfig, !bIsFarKeyboardNoTargetDrag);
#if WITH_AUTOMATION_TESTS
	++DragPickupTriggerCountForTest;
#endif
	PlayPendingDragPickupSound();
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TickDragPickupFeedback(float DeltaTime)
{
	if (!DragPickupPlayback || !DragPickupPlayback->IsActive())
	{
		return;
	}

	DragPickupPlayback->Tick(DeltaTime);
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::TryStartDeferredDragPickupFeedback()
{
	const float PointerAcquireDistancePixels = FMath::Max(
		24.0f,
		CardDragConfig.CardDragStartThresholdPixels * 2.0f);
	if (!DragPickupPlayback
		|| !DragPickupPlayback->IsWaitingForVisualStart()
		|| !IsFormalDragGestureState(GestureState)
		|| FVector2D::Distance(
			VisualSlotView.ScreenPosition,
			GetEffectiveTargetSlotView().ScreenPosition) > PointerAcquireDistancePixels)
	{
		return;
	}

	DragPickupPlayback->StartVisualPlayback();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::ResetDragPickupFeedback()
{
	if (DragPickupPlayback)
	{
		DragPickupPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingDragPickupSound()
{
	if (!DragPickupPlayback)
	{
		return;
	}

	const TOptional<FWacomFirstPersonCardDragPickupSoundRequest> PendingRequest =
		DragPickupPlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}

	const FWacomFirstPersonCardDragPickupSoundRequest& Request = PendingRequest.GetValue();
#if WITH_AUTOMATION_TESTS
	++DragPickupSoundRequestCountForTest;
	LastDragPickupSoundPitchMultiplierForTest = Request.PitchMultiplier;
#endif
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

float UWacomFirstPersonCardLayerSlotWidget::GetDragPickupAlpha() const
{
	return DragPickupPlayback ? DragPickupPlayback->GetAlpha() : 0.0f;
}

void UWacomFirstPersonCardLayerSlotWidget::ResetCardSurfaceEffectView()
{
	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(FWacomFirstPersonCardSurfaceEffectView());
	}
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSurfacePresentationReadiness(
	FName EffectName,
	bool bReuseReadyGeneration,
	bool bBlocksPresentationPhase)
{
	if (!CardView || !SurfaceReadinessGate)
	{
		SurfaceReadinessEffectName = EffectName;
		RecordPresentationReadinessFailure(TEXT("Surface"), EffectName, false);
		HandleSurfacePresentationReadinessFailure();
		return;
	}
	SurfaceReadinessEffectName = EffectName;
	bSurfaceReadinessBlocksPresentationPhase = bBlocksPresentationPhase;
	const uint32 Generation =
		CardView->BeginSurfacePresentationPreparation(bReuseReadyGeneration);
	if (Generation == 0)
	{
		RecordPresentationReadinessFailure(TEXT("Surface"), EffectName, false);
		HandleSurfacePresentationReadinessFailure();
		return;
	}
	SurfaceReadinessGate->Begin(
		Generation,
		CardView->IsSurfacePresentationMaterialReady(Generation)
			&& CardView->IsSurfacePresentationPainted(Generation));
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::BeginCostDigitPresentationReadiness()
{
	if (!CardView || !CostDigitReadinessGate)
	{
		RecordPresentationReadinessFailure(TEXT("CostDigit"), TEXT("DataRewrite"), false);
		ClearCardDataRewritePlayback();
		return;
	}
	const uint32 Generation = CardView->BeginCostDigitPresentationPreparation();
	if (Generation == 0)
	{
		RecordPresentationReadinessFailure(TEXT("CostDigit"), TEXT("DataRewrite"), false);
		ClearCardDataRewritePlayback();
		return;
	}
	CostDigitReadinessGate->Begin(
		Generation,
		CardView->IsCostDigitPresentationMaterialReady(Generation)
			&& CardView->IsCostDigitPresentationPainted(Generation));
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::BeginEffectBadgePresentationReadiness()
{
	if (!CardView || !EffectBadgeReadinessGate)
	{
		RecordPresentationReadinessFailure(TEXT("EffectBadge"), TEXT("EffectBadgeRewrite"), false);
		ClearEffectBadgeFeedbackPlayback();
		return;
	}
	const uint32 Generation = CardView->BeginEffectBadgePresentationPreparation();
	if (Generation == 0)
	{
		RecordPresentationReadinessFailure(TEXT("EffectBadge"), TEXT("EffectBadgeRewrite"), false);
		ClearEffectBadgeFeedbackPlayback();
		return;
	}
	EffectBadgeReadinessGate->Begin(
		Generation,
		CardView->IsEffectBadgePresentationMaterialReady(Generation)
			&& CardView->IsEffectBadgePresentationPainted(Generation));
	RefreshPresentationReadinessFrozenFlag();
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveSurfacePresentationReadiness(
	float DeltaTime,
	float& OutPlaybackDeltaTime)
{
	OutPlaybackDeltaTime = 0.0f;
	if (!SurfaceReadinessGate || !SurfaceReadinessGate->IsActive())
	{
		OutPlaybackDeltaTime = DeltaTime;
		return true;
	}
	if (CardView)
	{
		CardView->RefreshSurfacePresentationPreparation(
			SurfaceReadinessGate->GetGeneration());
	}
	const uint32 Generation = SurfaceReadinessGate->GetGeneration();
	const EWacomFirstPersonCardPresentationReadinessPollResult Result =
		SurfaceReadinessGate->Poll(
			DeltaTime,
			CardView && CardView->IsSurfacePresentationMaterialReady(Generation),
			CardView && CardView->IsSurfacePresentationPainted(Generation));
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::Ready)
	{
		OutPlaybackDeltaTime = DeltaTime;
		RefreshPresentationReadinessFrozenFlag();
		return true;
	}
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady)
	{
		RefreshPresentationReadinessFrozenFlag();
		return true;
	}
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::Failed)
	{
		RecordPresentationReadinessFailure(TEXT("Surface"), SurfaceReadinessEffectName, true);
		HandleSurfacePresentationReadinessFailure();
	}
	RefreshPresentationReadinessFrozenFlag();
	return false;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveCostDigitPresentationReadiness(
	float DeltaTime,
	float& OutPlaybackDeltaTime)
{
	OutPlaybackDeltaTime = 0.0f;
	if (!CostDigitReadinessGate || !CostDigitReadinessGate->IsActive())
	{
		OutPlaybackDeltaTime = DeltaTime;
		return true;
	}
	if (CardView)
	{
		CardView->RefreshCostDigitPresentationPreparation(CostDigitReadinessGate->GetGeneration());
	}
	const uint32 Generation = CostDigitReadinessGate->GetGeneration();
	const EWacomFirstPersonCardPresentationReadinessPollResult Result =
		CostDigitReadinessGate->Poll(
			DeltaTime,
			CardView && CardView->IsCostDigitPresentationMaterialReady(Generation),
			CardView && CardView->IsCostDigitPresentationPainted(Generation));
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::Ready)
	{
		OutPlaybackDeltaTime = DeltaTime;
		RefreshPresentationReadinessFrozenFlag();
		return true;
	}
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady)
	{
		RefreshPresentationReadinessFrozenFlag();
		return true;
	}
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::Failed)
	{
		RecordPresentationReadinessFailure(TEXT("CostDigit"), TEXT("DataRewrite"), true);
		CancelCostDigitPresentationReadiness();
		ClearCardDataRewritePlayback();
	}
	RefreshPresentationReadinessFrozenFlag();
	return false;
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveEffectBadgePresentationReadiness(
	float DeltaTime,
	float& OutPlaybackDeltaTime)
{
	OutPlaybackDeltaTime = 0.0f;
	if (!EffectBadgeReadinessGate || !EffectBadgeReadinessGate->IsActive())
	{
		OutPlaybackDeltaTime = DeltaTime;
		return true;
	}
	if (CardView)
	{
		CardView->RefreshEffectBadgePresentationPreparation(
			EffectBadgeReadinessGate->GetGeneration());
	}
	const uint32 Generation = EffectBadgeReadinessGate->GetGeneration();
	const EWacomFirstPersonCardPresentationReadinessPollResult Result =
		EffectBadgeReadinessGate->Poll(
			DeltaTime,
			CardView && CardView->IsEffectBadgePresentationMaterialReady(Generation),
			CardView && CardView->IsEffectBadgePresentationPainted(Generation));
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::Ready)
	{
		OutPlaybackDeltaTime = DeltaTime;
		RefreshPresentationReadinessFrozenFlag();
		return true;
	}
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::BecameReady)
	{
		RefreshPresentationReadinessFrozenFlag();
		return true;
	}
	if (Result == EWacomFirstPersonCardPresentationReadinessPollResult::Failed)
	{
		RecordPresentationReadinessFailure(TEXT("EffectBadge"), TEXT("EffectBadgeRewrite"), true);
		CancelEffectBadgePresentationReadiness();
		ClearEffectBadgeFeedbackPlayback();
	}
	RefreshPresentationReadinessFrozenFlag();
	return false;
}

void UWacomFirstPersonCardLayerSlotWidget::CancelSurfacePresentationReadiness()
{
	if (SurfaceReadinessGate)
	{
		SurfaceReadinessGate->Reset();
	}
	SurfaceReadinessEffectName = NAME_None;
	bSurfaceReadinessBlocksPresentationPhase = true;
	if (CardView)
	{
		CardView->CancelSurfacePresentationPreparation();
	}
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::CancelSurfacePresentationReadinessIfOwnedBy(
	FName EffectName)
{
	if (SurfaceReadinessEffectName == EffectName)
	{
		CancelSurfacePresentationReadiness();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::CancelCostDigitPresentationReadiness()
{
	if (CostDigitReadinessGate)
	{
		CostDigitReadinessGate->Reset();
	}
	if (CardView)
	{
		CardView->CancelCostDigitPresentationPreparation();
	}
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::CancelEffectBadgePresentationReadiness()
{
	if (EffectBadgeReadinessGate)
	{
		EffectBadgeReadinessGate->Reset();
	}
	if (CardView)
	{
		CardView->CancelEffectBadgePresentationPreparation();
	}
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::CancelAllPresentationReadiness()
{
	CancelSurfacePresentationReadiness();
	CancelCostDigitPresentationReadiness();
	CancelEffectBadgePresentationReadiness();
	if (CardView)
	{
		CardView->CancelAllPresentationPreparations();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::RefreshPresentationReadinessFrozenFlag()
{
	bPlaybackFrozenForReadiness =
		(SurfaceReadinessGate && SurfaceReadinessGate->IsPending())
		|| (CostDigitReadinessGate && CostDigitReadinessGate->IsPending())
		|| (EffectBadgeReadinessGate && EffectBadgeReadinessGate->IsPending());
}

void UWacomFirstPersonCardLayerSlotWidget::RecordPresentationReadinessFailure(
	FName ChannelName,
	FName EffectName,
	bool bTimedOut)
{
#if WITH_AUTOMATION_TESTS
	if (bTimedOut)
	{
		++PresentationReadinessTimeoutCountForTest;
	}
	++PresentationReadinessFallbackCountForTest;
#endif
	UE_LOG(
		LogWacomCardPresentationReadiness,
		Warning,
		TEXT("Card presentation readiness %s. Card=%s Channel=%s Effect=%s"),
		bTimedOut ? TEXT("timed out") : TEXT("contract is invalid"),
		*CurrentSlotView.Entry.CardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*ChannelName.ToString(),
		*EffectName.ToString());
}

void UWacomFirstPersonCardLayerSlotWidget::HandleSurfacePresentationReadinessFailure()
{
	const FName FailedEffect = SurfaceReadinessEffectName;
	CancelSurfacePresentationReadiness();
	if (FailedEffect == SurfaceEffectDrawReveal)
	{
		ClearDrawRevealPlayback();
		if (TransitionPlayback)
		{
			TransitionPlayback->ConsumePendingSoundRequest();
		}
	}
	else if (FailedEffect == SurfaceEffectGainReveal)
	{
		ClearGainRevealPlayback();
		if (TransitionPlayback)
		{
			TransitionPlayback->ConsumePendingSoundRequest();
		}
	}
	else if (FailedEffect == SurfaceEffectHandTargetImpact)
	{
		if (HandTargetImpactPlayback)
		{
			HandTargetImpactPlayback->Reset();
		}
		HandTargetImpactScaleMultiplier = 1.0f;
		HandTargetImpactTranslationYPixels = 0.0f;
		HandTargetImpactZOrderBoost = 0;
		bHandTargetImpactDepartureGateReleased = true;
		ApplyActiveSurfaceEffectView();
		ApplyVisualSlotView();
	}
	else if (FailedEffect == SurfaceEffectDeparture)
	{
		ClearSurfaceDeparturePlayback();
		bUsesSurfaceDepartureExit = false;
		bUsesFixedExitTransitionPlayback = false;
		TargetSlotView = VisualSlotView;
		TargetSlotView.ScreenPosition += SlotMotionConfig.ExitOffsetPixels;
		TargetSlotView.WidgetPosition += SlotMotionConfig.ExitOffsetPixels;
		TargetSlotView.RenderOpacity = 0.0f;
		TargetSlotView.bProjected = false;
		ExitMotionElapsedSeconds = 0.0f;
	}
	else if (FailedEffect == SurfaceEffectCardUseReform)
	{
		ClearCardUseReformPlayback(true);
	}
	else if (FailedEffect == SurfaceEffectRetainSeal)
	{
		bSuppressRetainSealSurfaceForReadinessFailure = true;
		ResetCardSurfaceEffectView();
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayHandTargetImpact() const
{
	const FWacomFirstPersonCardHandTargetImpactConfig& Config =
		SlotVisualConfig.HandTargetImpact;
	return Config.bEnabled
		&& Config.Style.SurfaceEffectMaterialInstance != nullptr
		&& (Config.bReducedMotion || Config.Style.CommitDurationSeconds > KINDA_SMALL_NUMBER);
}

void UWacomFirstPersonCardLayerSlotWidget::BeginHandTargetImpactPreview()
{
	if (!CanPlayHandTargetImpact() || IsSurfaceDeparturePlaybackActive()
		|| IsCardUseReformPlaybackActive() || bIsExitingForFirstPersonLayer)
	{
		return;
	}
	if (!HandTargetImpactPlayback)
	{
		HandTargetImpactPlayback.Reset(new FWacomFirstPersonCardHandTargetImpactPlayback());
	}
	HandTargetImpactPlayback->BeginPreview(SlotVisualConfig.HandTargetImpact);
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectHandTargetImpact, false, false);
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::EndHandTargetImpactPreview()
{
	if (!HandTargetImpactPlayback || HandTargetImpactPlayback->IsCommitActive())
	{
		return;
	}
	HandTargetImpactPlayback->EndPreview();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerHandTargetImpactFeedback()
{
	if (!CanPlayHandTargetImpact() || !bHasVisualSlotView)
	{
		return;
	}
	if (!HandTargetImpactPlayback)
	{
		HandTargetImpactPlayback.Reset(new FWacomFirstPersonCardHandTargetImpactPlayback());
	}
	ClearCardDragTargetFeedback();
	if (CardView)
	{
		CardView->ResetCostDigitPreviewView();
	}
	const uint32 Seed = GetTypeHash(CurrentSlotView.Entry.CardInstanceId);
	HandTargetImpactPlayback->BeginCommit(SlotVisualConfig.HandTargetImpact, Seed);
	bHandTargetImpactDepartureGateReleased = false;
	HandTargetImpactScaleMultiplier = 1.0f;
	HandTargetImpactTranslationYPixels = 0.0f;
	HandTargetImpactZOrderBoost = SlotVisualConfig.HandTargetImpact.Style.ZOrderBoost;
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectHandTargetImpact, true);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

bool UWacomFirstPersonCardLayerSlotWidget::PrepareCardDataRewriteForSlotView(
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	const FWacomFirstPersonCardLayerResolvedFeedbackHint& RewriteHint)
{
	EnsureCardView();
	if (!CanPlayCardDataRewrite()
		|| !CardView
		|| !InTargetSlotView.Entry.CardInstanceId.IsValid()
		|| InTargetSlotView.Entry.CardInstanceId != CurrentSlotView.Entry.CardInstanceId)
	{
		return false;
	}

	const bool bRewritesCost = (RewriteHint.DataRewriteFieldMask
		& static_cast<int32>(EWacomFirstPersonCardDataRewriteField::Cost)) != 0;
	if (bRewritesCost && RewriteHint.bHasDataRewriteCostValues)
	{
		if (InTargetSlotView.Entry.CardViewData.Cost != RewriteHint.DataRewriteCostAfter)
		{
			return false;
		}
		FWacomCardViewData OldData = InTargetSlotView.Entry.CardViewData;
		OldData.Cost = RewriteHint.DataRewriteCostBefore;
		return CardView->PrepareCostDigitRewrite(
			OldData,
			InTargetSlotView.Entry.CardViewData);
	}

	return CardView->PrepareCostDigitRewrite(InTargetSlotView.Entry.CardViewData);
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCardDataRewriteFeedback(
	int32 FieldMask,
	EWacomFirstPersonCardDataRewriteTone Tone,
	int32 Seed,
	int32 SequenceIndex,
	int32 SequenceCount,
	bool bBlocksPresentationPhase)
{
	if (!CanPlayCardDataRewrite()
		|| FieldMask == 0
		|| bIsExitingForFirstPersonLayer
		|| IsSurfaceDeparturePlaybackActive()
		|| IsCardUseReformPlaybackActive())
	{
		return;
	}

	if (IsHandTargetImpactCommitPlaybackActive()
		&& !bHandTargetImpactDepartureGateReleased)
	{
		PendingDataRewriteFieldMask |= FieldMask;
		PendingDataRewriteTone = Tone;
		PendingDataRewriteSeed = Seed;
		PendingDataRewriteSequenceIndex = FMath::Max(0, SequenceIndex);
		PendingDataRewriteSequenceCount = FMath::Max(1, SequenceCount);
		bPendingDataRewriteHandoff = true;
		bDataRewriteBlocksPresentationPhase = bBlocksPresentationPhase;
		UpdateWantsTick();
		return;
	}

	BeginCardDataRewritePlayback(
		FieldMask,
		Tone,
		Seed,
		SequenceIndex,
		true);
	bDataRewriteBlocksPresentationPhase = bBlocksPresentationPhase;
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerEffectBadgeFeedback(
	const TArray<FWacomFirstPersonCardEffectBadgeChange>& Changes,
	bool bBlocksPresentationPhase)
{
	if (!SlotVisualConfig.EffectBadgeFeedback.bEnabled
		|| Changes.IsEmpty()
		|| bIsExitingForFirstPersonLayer
		|| IsSurfaceDeparturePlaybackActive()
		|| IsCardUseReformPlaybackActive())
	{
		return;
	}
	if (!EffectBadgeFeedbackPlayback)
	{
		EffectBadgeFeedbackPlayback.Reset(
			new FWacomFirstPersonCardEffectBadgeFeedbackPlayback());
	}
	EffectBadgeFeedbackPlayback->Begin(SlotVisualConfig.EffectBadgeFeedback, Changes);
	bEffectBadgeFeedbackBlocksPresentationPhase =
		bBlocksPresentationPhase && EffectBadgeFeedbackPlayback->IsActive();
	ApplyEffectBadgeFeedbackView();
	BeginEffectBadgePresentationReadiness();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::BeginDeferredExitWithHandTargetImpact(
	const FWacomFirstPersonCardLayerSlotView& InExitTargetSlotView,
	const TOptional<FWacomFirstPersonCardTransitionMotionProfile>& ExitProfileOverride,
	EWacomFirstPersonCardSlotTransitionKind TransitionKind)
{
	if (!CanPlayHandTargetImpact())
	{
		BeginExitMotionWithProfile(InExitTargetSlotView, ExitProfileOverride, TransitionKind);
		return;
	}
	DeferredHandTargetExitSlotView = InExitTargetSlotView;
	DeferredHandTargetExitProfile = ExitProfileOverride;
	DeferredHandTargetExitTransitionKind = TransitionKind;
	bHandTargetImpactDeparturePending = true;
	bHandTargetImpactDepartureOwnedByPileTransfer = false;
	bHandTargetImpactDepartureGateReleased = false;
	TriggerHandTargetImpactFeedback();
	UpdateWantsTick();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsHandTargetImpactDepartureGateOpen() const
{
	return bHandTargetImpactDeparturePending
		&& (bHandTargetImpactDepartureGateReleased
			|| (HandTargetImpactPlayback
				&& HandTargetImpactPlayback->IsDepartureGateOpen()));
}

void UWacomFirstPersonCardLayerSlotWidget::SetHandTargetImpactDepartureOwnedByPileTransfer(
	bool bOwned)
{
	bHandTargetImpactDepartureOwnedByPileTransfer =
		bOwned && bHandTargetImpactDeparturePending;
}

void UWacomFirstPersonCardLayerSlotWidget::ReleaseDeferredHandTargetExitNow()
{
	if (!bHandTargetImpactDeparturePending)
	{
		return;
	}
	const FWacomFirstPersonCardLayerSlotView ExitSlotView = DeferredHandTargetExitSlotView;
	const TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfile =
		DeferredHandTargetExitProfile;
	const EWacomFirstPersonCardSlotTransitionKind TransitionKind =
		DeferredHandTargetExitTransitionKind;
	bHandTargetImpactDeparturePending = false;
	BeginExitMotionWithProfile(ExitSlotView, ExitProfile, TransitionKind);
}

void UWacomFirstPersonCardLayerSlotWidget::TickHandTargetImpactPlayback(float DeltaTime)
{
	if (!HandTargetImpactPlayback || !HandTargetImpactPlayback->IsActive())
	{
		return;
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardHandTargetImpactPlaybackView View =
		HandTargetImpactPlayback->Tick(PlaybackDeltaTime);
	bHandTargetImpactDepartureGateReleased =
		bHandTargetImpactDepartureGateReleased || View.bDepartureGateOpen;
	if (!HandTargetImpactPlayback->IsActive())
	{
		CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectHandTargetImpact);
		HandTargetImpactScaleMultiplier = 1.0f;
		HandTargetImpactTranslationYPixels = 0.0f;
		HandTargetImpactZOrderBoost = 0;
		ApplyActiveSurfaceEffectView();
		ApplyVisualSlotView();
		UpdateWantsTick();
		return;
	}
	HandTargetImpactScaleMultiplier = View.ScaleMultiplier;
	HandTargetImpactTranslationYPixels = View.TranslationYPixels;
	HandTargetImpactZOrderBoost = View.ZOrderBoost;
	PlayPendingHandTargetImpactSound();
	if (View.bCompleted)
	{
		HandTargetImpactPlayback->Reset();
		CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectHandTargetImpact);
		HandTargetImpactScaleMultiplier = 1.0f;
		HandTargetImpactTranslationYPixels = 0.0f;
		HandTargetImpactZOrderBoost = 0;
		ApplyActiveSurfaceEffectView();
		ApplyVisualSlotView();
		UpdateWantsTick();
		return;
	}
	ApplyActiveSurfaceEffectView();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearHandTargetImpactPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectHandTargetImpact);
	if (HandTargetImpactPlayback)
	{
		HandTargetImpactPlayback->Reset();
	}
	HandTargetImpactScaleMultiplier = 1.0f;
	HandTargetImpactTranslationYPixels = 0.0f;
	HandTargetImpactZOrderBoost = 0;
	bHandTargetImpactDeparturePending = false;
	bHandTargetImpactDepartureOwnedByPileTransfer = false;
	bHandTargetImpactDepartureGateReleased = false;
	DeferredHandTargetExitProfile.Reset();
	DeferredHandTargetExitTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	if (CardView)
	{
		CardView->ResetCostDigitPreviewView();
	}
	ApplyActiveSurfaceEffectView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyHandTargetImpactSurfaceView()
{
	if (!CardView || !HandTargetImpactPlayback || !HandTargetImpactPlayback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardHandTargetImpactPlaybackView PlaybackView =
		HandTargetImpactPlayback->BuildView();
	CardView->SetCardSurfaceEffectView(BuildHandTargetImpactView(
		PlaybackView,
		SlotVisualConfig,
		CurrentSlotView.Entry.CardInstanceId));

	const bool bIsPreview =
		PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering
		|| PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain
		|| PlaybackView.Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewExiting;
	const FWacomCardViewData& CardData = CurrentSlotView.Entry.CardViewData;
	if (bIsPreview && CardData.bHasCostPreview && CardData.PreviewCost != CardData.Cost)
	{
		FWacomFirstPersonCardCostPreviewView PreviewView;
		PreviewView.bActive = true;
		PreviewView.PreviewAmount = PlaybackView.PreviewAmount;
		const float Period = FMath::Max(
			0.01f,
			SlotVisualConfig.DataRewrite.Style.PreviewPulsePeriodSeconds);
		PreviewView.PulseAmount = 0.5f + 0.5f * FMath::Cos(
			(PlaybackView.TimeSeconds / Period) * 2.0f * UE_PI);
		PreviewView.Tone = CardData.PreviewCost < CardData.Cost
			? EWacomFirstPersonCardDataRewriteTone::Beneficial
			: EWacomFirstPersonCardDataRewriteTone::Detrimental;
		PreviewView.Seed = static_cast<int32>(GetTypeHash(CurrentSlotView.Entry.CardInstanceId));
		PreviewView.Style = SlotVisualConfig.DataRewrite.Style;
		CardView->SetCostDigitPreviewView(PreviewView);
	}
	else
	{
		CardView->ResetCostDigitPreviewView();
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayCardDataRewrite() const
{
	const FWacomFirstPersonCardDataRewriteConfig& Config = SlotVisualConfig.DataRewrite;
	return Config.bEnabled
		&& Config.Style.DigitRewriteMaterialInstance != nullptr
		&& Config.Style.DurationSeconds > 0.0f;
}

void UWacomFirstPersonCardLayerSlotWidget::BeginCardDataRewritePlayback(
	int32 FieldMask,
	EWacomFirstPersonCardDataRewriteTone Tone,
	int32 Seed,
	int32 SequenceIndex,
	bool bAllowSequenceDelay)
{
	if (!CanPlayCardDataRewrite() || FieldMask == 0)
	{
		ClearCardDataRewritePlayback();
		return;
	}
	if (!DataRewritePlayback)
	{
		DataRewritePlayback.Reset(new FWacomFirstPersonCardDataRewritePlayback());
	}
	DataRewritePlayback->BeginOrRetarget(
		SlotVisualConfig.DataRewrite,
		FieldMask,
		Tone,
		static_cast<uint32>(Seed),
		SequenceIndex,
		bAllowSequenceDelay);
	PendingDataRewriteFieldMask = 0;
	PendingDataRewriteTone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	PendingDataRewriteSeed = 0;
	PendingDataRewriteSequenceIndex = 0;
	PendingDataRewriteSequenceCount = 1;
	bPendingDataRewriteHandoff = false;
	if (!DataRewritePlayback || !DataRewritePlayback->IsActive())
	{
		bDataRewriteBlocksPresentationPhase = false;
	}
	ApplyCardDataRewriteView();
	if (DataRewritePlayback->BuildView().bActive)
	{
		BeginCostDigitPresentationReadiness();
	}
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TickCardDataRewritePlayback(float DeltaTime)
{
	if (!DataRewritePlayback || !DataRewritePlayback->IsActive())
	{
		return;
	}
	if (!DataRewritePlayback->BuildView().bActive)
	{
		const FWacomFirstPersonCardDataRewritePlaybackView DelayView =
			DataRewritePlayback->Tick(DeltaTime);
		if (!DelayView.bActive)
		{
			return;
		}
		ApplyCardDataRewriteView();
		BeginCostDigitPresentationReadiness();
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveCostDigitPresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardDataRewritePlaybackView View =
		DataRewritePlayback->Tick(PlaybackDeltaTime);
	PlayPendingCardDataRewriteSound();
	if (View.bCompleted || !DataRewritePlayback->IsActive())
	{
		DataRewritePlayback->Reset();
		if (CardView)
		{
			CardView->ResetCardDataRewriteView();
		}
		bDataRewriteBlocksPresentationPhase = false;
		UpdateWantsTick();
		return;
	}
	ApplyCardDataRewriteView();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearCardDataRewritePlayback()
{
	CancelCostDigitPresentationReadiness();
	if (DataRewritePlayback)
	{
		DataRewritePlayback->Reset();
	}
	PendingDataRewriteFieldMask = 0;
	PendingDataRewriteTone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	PendingDataRewriteSeed = 0;
	PendingDataRewriteSequenceIndex = 0;
	PendingDataRewriteSequenceCount = 1;
	bPendingDataRewriteHandoff = false;
	bDataRewriteBlocksPresentationPhase = false;
	if (CardView)
	{
		CardView->ResetCardDataRewriteView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCardDataRewriteView()
{
	if (!CardView || !DataRewritePlayback || !DataRewritePlayback->IsActive())
	{
		return;
	}
	CardView->SetCardDataRewriteView(BuildDataRewriteView(
		DataRewritePlayback->BuildView(),
		SlotVisualConfig));
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingCardDataRewriteSound()
{
	if (!DataRewritePlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardDataRewriteSoundRequest> PendingRequest =
		DataRewritePlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}
	const FWacomFirstPersonCardDataRewriteSoundRequest& Request = PendingRequest.GetValue();
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsCardDataRewritePlaybackActive() const
{
	return DataRewritePlayback && DataRewritePlayback->IsActive();
}

void UWacomFirstPersonCardLayerSlotWidget::TickEffectBadgeFeedbackPlayback(float DeltaTime)
{
	if (!EffectBadgeFeedbackPlayback || !EffectBadgeFeedbackPlayback->IsActive())
	{
		return;
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveEffectBadgePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardEffectBadgeFeedbackView View =
		EffectBadgeFeedbackPlayback->Tick(PlaybackDeltaTime);
	PlayPendingEffectBadgeFeedbackSound();
	if (View.bCompleted || !EffectBadgeFeedbackPlayback->IsActive())
	{
		ClearEffectBadgeFeedbackPlayback();
		UpdateWantsTick();
		return;
	}
	ApplyEffectBadgeFeedbackView();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearEffectBadgeFeedbackPlayback()
{
	CancelEffectBadgePresentationReadiness();
	if (EffectBadgeFeedbackPlayback)
	{
		EffectBadgeFeedbackPlayback->Reset();
	}
	bEffectBadgeFeedbackBlocksPresentationPhase = false;
	if (CardView)
	{
		CardView->ResetEffectBadgeFeedbackView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyEffectBadgeFeedbackView()
{
	if (!CardView || !EffectBadgeFeedbackPlayback || !EffectBadgeFeedbackPlayback->IsActive())
	{
		return;
	}
	CardView->SetEffectBadgeFeedbackConfig(SlotVisualConfig.EffectBadgeFeedback);
	CardView->SetEffectBadgeFeedbackView(EffectBadgeFeedbackPlayback->BuildView());
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingEffectBadgeFeedbackSound()
{
	if (!EffectBadgeFeedbackPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest> PendingRequest =
		EffectBadgeFeedbackPlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}
	const FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest& Request = PendingRequest.GetValue();
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsEffectBadgeFeedbackPlaybackActive() const
{
	return EffectBadgeFeedbackPlayback && EffectBadgeFeedbackPlayback->IsActive();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyActiveSurfaceEffectView()
{
	if (!CardView)
	{
		return;
	}
	if (IsSurfaceDeparturePlaybackActive() || IsCardUseReformPlaybackActive())
	{
		return;
	}
	if (IsHandTargetImpactPlaybackActive())
	{
		ApplyHandTargetImpactSurfaceView();
		return;
	}
	if (IsDrawRevealPlaybackActive())
	{
		ApplyDrawRevealSurfaceView();
		return;
	}
	if (IsGainRevealPlaybackActive())
	{
		ApplyGainRevealSurfaceView();
		return;
	}
	if (IsRetainSealPlaybackActive())
	{
		ApplyRetainSealSurfaceView();
		return;
	}
	CardView->SetEffectBadgeFeedbackConfig(SlotVisualConfig.EffectBadgeFeedback);
	ResetCardSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayDrawReveal() const
{
	const FWacomFirstPersonCardDrawRevealConfig& Config = SlotVisualConfig.DrawReveal;
	return Config.bEnabled && Config.Style.SurfaceEffectMaterialInstance != nullptr;
}

void UWacomFirstPersonCardLayerSlotWidget::PrepareDrawRevealPlayback(
	EWacomFirstPersonCardSlotTransitionKind TransitionKind)
{
	ClearDrawRevealPlayback();
	if (TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Drawn
		|| !CanPlayDrawReveal())
	{
		return;
	}
	if (!DrawRevealPlayback)
	{
		DrawRevealPlayback.Reset(new FWacomFirstPersonCardDrawRevealPlayback());
	}
	DrawRevealPlayback->Prepare(SlotVisualConfig.DrawReveal);
	ApplyDrawRevealSurfaceView();
	BeginSurfacePresentationReadiness(SurfaceEffectDrawReveal);
}

void UWacomFirstPersonCardLayerSlotWidget::StartDrawRevealPlayback()
{
	if (!DrawRevealPlayback)
	{
		return;
	}
	DrawRevealPlayback->Start();
	ApplyDrawRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateDrawRevealPlayback(
	float NormalizedEnterProgress)
{
	if (!DrawRevealPlayback || !DrawRevealPlayback->IsActive())
	{
		return;
	}
	DrawRevealPlayback->Update(NormalizedEnterProgress);
	ApplyDrawRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyDrawRevealSurfaceView()
{
	if (!CardView || !DrawRevealPlayback || !DrawRevealPlayback->IsActive())
	{
		return;
	}
	CardView->SetCardSurfaceEffectView(BuildDrawRevealView(
		DrawRevealPlayback->BuildView(),
		SlotVisualConfig));
}

void UWacomFirstPersonCardLayerSlotWidget::ClearDrawRevealPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectDrawReveal);
	if (DrawRevealPlayback)
	{
		DrawRevealPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsDrawRevealPlaybackActive() const
{
	return DrawRevealPlayback && DrawRevealPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayGainReveal() const
{
	const FWacomFirstPersonCardGainRevealConfig& Config = SlotVisualConfig.GainReveal;
	return Config.bEnabled && Config.Style.SurfaceEffectMaterialInstance != nullptr;
}

void UWacomFirstPersonCardLayerSlotWidget::PrepareGainRevealPlayback(
	EWacomFirstPersonCardSlotTransitionKind TransitionKind)
{
	ClearGainRevealPlayback();
	if (TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Gained
		|| !CanPlayGainReveal())
	{
		return;
	}
	if (!GainRevealPlayback)
	{
		GainRevealPlayback.Reset(new FWacomFirstPersonCardGainRevealPlayback());
	}
	GainRevealPlayback->Prepare(SlotVisualConfig.GainReveal);
	ApplyGainRevealSurfaceView();
	BeginSurfacePresentationReadiness(SurfaceEffectGainReveal);
}

void UWacomFirstPersonCardLayerSlotWidget::StartGainRevealPlayback()
{
	if (!GainRevealPlayback)
	{
		return;
	}
	GainRevealPlayback->Start();
	ApplyGainRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGainRevealPlayback(
	float NormalizedEnterProgress)
{
	if (!GainRevealPlayback || !GainRevealPlayback->IsActive())
	{
		return;
	}
	GainRevealPlayback->Update(NormalizedEnterProgress);
	ApplyGainRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyGainRevealSurfaceView()
{
	if (!CardView || !GainRevealPlayback || !GainRevealPlayback->IsActive())
	{
		return;
	}
	CardView->SetCardSurfaceEffectView(BuildGainRevealView(
		GainRevealPlayback->BuildView(),
		SlotVisualConfig,
		CurrentSlotView));
}

void UWacomFirstPersonCardLayerSlotWidget::ClearGainRevealPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectGainReveal);
	if (GainRevealPlayback)
	{
		GainRevealPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsGainRevealPlaybackActive() const
{
	return GainRevealPlayback && GainRevealPlayback->IsActive();
}

void UWacomFirstPersonCardLayerSlotWidget::TickRetainSealPlayback(float DeltaTime)
{
	if (!RetainSealPlayback || !RetainSealPlayback->IsActive())
	{
		return;
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	RetainSealPlayback->Tick(PlaybackDeltaTime);
	if (!RetainSealPlayback->IsActive())
	{
		CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectRetainSeal);
	}
	ApplyActiveSurfaceEffectView();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyRetainSealSurfaceView()
{
	if (!CardView || !RetainSealPlayback || !RetainSealPlayback->IsActive())
	{
		return;
	}
	if (bSuppressRetainSealSurfaceForReadinessFailure)
	{
		ResetCardSurfaceEffectView();
		return;
	}
	CardView->SetCardSurfaceEffectView(BuildRetainSealView(
		RetainSealPlayback->BuildView()));
}

void UWacomFirstPersonCardLayerSlotWidget::ClearRetainSealPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectRetainSeal);
	bSuppressRetainSealSurfaceForReadinessFailure = false;
	if (RetainSealPlayback)
	{
		RetainSealPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsRetainSealPlaybackActive() const
{
	return RetainSealPlayback && RetainSealPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsRetainSealPlaybackBlockingPresentation() const
{
	return RetainSealPlayback && RetainSealPlayback->IsBlockingPresentation();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingHandTargetImpactSound()
{
	if (!HandTargetImpactPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardHandTargetImpactSoundRequest> PendingRequest =
		HandTargetImpactPlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}
	const FWacomFirstPersonCardHandTargetImpactSoundRequest& Request = PendingRequest.GetValue();
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsHandTargetImpactPlaybackActive() const
{
	return HandTargetImpactPlayback && HandTargetImpactPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsHandTargetImpactCommitPlaybackActive() const
{
	return HandTargetImpactPlayback && HandTargetImpactPlayback->IsCommitActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayCardUseEffect() const
{
	const FWacomFirstPersonCardUseEffectConfig& Config = SlotVisualConfig.CardUseEffect;
	return Config.bEnabled
		&& Config.Style.SurfaceEffectMaterialInstance != nullptr
		&& (Config.bReducedMotion || Config.Style.DurationSeconds > KINDA_SMALL_NUMBER);
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayCardUseReformEffect() const
{
	const FWacomFirstPersonCardUseEffectConfig& Config = SlotVisualConfig.CardUseEffect;
	const bool bHasValidTiming = Config.Style.EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		? Config.Style.EdgeFlipReformOutSeconds > KINDA_SMALL_NUMBER
			&& Config.Style.EdgeFlipReformInSeconds > KINDA_SMALL_NUMBER
		: Config.Style.ReformDissolveOutSeconds > KINDA_SMALL_NUMBER
			&& Config.Style.ReformBuildInSeconds > KINDA_SMALL_NUMBER;
	return Config.bEnabled
		&& Config.Style.SurfaceEffectMaterialInstance != nullptr
		&& (Config.bReducedMotion || bHasValidTiming);
}

bool UWacomFirstPersonCardLayerSlotWidget::CanPlayExhaustDissolve() const
{
	const FWacomFirstPersonCardPlayedDissolveConfig& Config = SlotVisualConfig.PlayedDissolve;
	return Config.bEnabled
		&& Config.Style.SurfaceEffectMaterial != nullptr
		&& Config.Style.NoiseTexture != nullptr
		&& (Config.bReducedMotion || Config.Style.DurationSeconds > KINDA_SMALL_NUMBER);
}

void UWacomFirstPersonCardLayerSlotWidget::StartSurfaceDeparturePlayback(
	EWacomFirstPersonCardSlotTransitionKind TransitionKind)
{
	if (!SurfaceDeparturePlayback)
	{
		SurfaceDeparturePlayback.Reset(new FWacomFirstPersonCardSurfaceDeparturePlayback());
	}

	const uint32 CardHash = GetTypeHash(CurrentSlotView.Entry.CardInstanceId);
	const float Seed = static_cast<float>(CardHash & 0xFFFFu) / 65535.0f;
	FWacomFirstPersonCardSurfaceDeparturePlaybackConfig PlaybackConfig;
	PlaybackConfig.Seed = Seed;
	if (TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played)
	{
		const FWacomFirstPersonCardUseEffectConfig& Config = SlotVisualConfig.CardUseEffect;
		PlaybackConfig.Kind = EWacomFirstPersonCardSurfaceDepartureKind::CardUse;
		PlaybackConfig.bReducedMotion = Config.bReducedMotion;
		PlaybackConfig.DurationSeconds = Config.bReducedMotion
			? 0.12f
			: Config.Style.DurationSeconds;
		PlaybackConfig.ConfirmHoldSeconds = Config.Style.ConfirmHoldSeconds;
		PlaybackConfig.ImpactSeconds = Config.Style.EdgeFlipImpactSeconds;
		PlaybackConfig.CardUseEffectKind = Config.Style.EffectKind;
		PlaybackConfig.StartSound = Config.Style.StartSound;
		PlaybackConfig.SoundVolumeMultiplier = Config.Style.StartSoundVolumeMultiplier;
		PlaybackConfig.SoundPitchMultiplier = Config.Style.StartSoundPitchMultiplier;
		PlaybackConfig.SoundPitchVariation = Config.Style.StartSoundPitchVariation;
	}
	else if (TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Exhausted)
	{
		const FWacomFirstPersonCardPlayedDissolveConfig& Config = SlotVisualConfig.PlayedDissolve;
		PlaybackConfig.Kind = EWacomFirstPersonCardSurfaceDepartureKind::ExhaustDissolve;
		PlaybackConfig.bReducedMotion = Config.bReducedMotion;
		PlaybackConfig.DurationSeconds = Config.bReducedMotion
			? 0.12f
			: Config.Style.DurationSeconds;
		PlaybackConfig.ConfirmHoldSeconds = Config.Style.ConfirmHoldSeconds;
		PlaybackConfig.StartSound = Config.Style.StartSound;
		PlaybackConfig.SoundVolumeMultiplier = Config.Style.StartSoundVolumeMultiplier;
		PlaybackConfig.SoundPitchMultiplier = Config.Style.StartSoundPitchMultiplier;
		PlaybackConfig.SoundPitchVariation = Config.Style.StartSoundPitchVariation;
	}

	SurfaceDeparturePlayback->Begin(PlaybackConfig);
	if (!SurfaceDeparturePlayback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardSurfaceDepartureTickResult InitialView =
		SurfaceDeparturePlayback->BuildView();
	CardUseFlipProgress = InitialView.FlipProgress;
	CardUseImpactProgress = InitialView.ImpactProgress;
	CardUseMotionAlpha = InitialView.MotionAlpha;
	CardUseOpacityMultiplier = 1.0f;

	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(BuildSurfaceDepartureView(
			SurfaceDeparturePlayback->BuildView(),
			SlotVisualConfig));
	}
	BeginSurfacePresentationReadiness(SurfaceEffectDeparture);
}

void UWacomFirstPersonCardLayerSlotWidget::TickSurfaceDeparturePlayback(float DeltaTime)
{
	if (!SurfaceDeparturePlayback || !SurfaceDeparturePlayback->IsActive())
	{
		return;
	}

	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardSurfaceDepartureTickResult TickResult =
		SurfaceDeparturePlayback->Tick(PlaybackDeltaTime);
	PlayPendingSurfaceDepartureSound();
	CardUseFlipProgress = TickResult.FlipProgress;
	CardUseImpactProgress = TickResult.ImpactProgress;
	CardUseMotionAlpha = TickResult.MotionAlpha;
	CardUseOpacityMultiplier = 1.0f;
	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(BuildSurfaceDepartureView(TickResult, SlotVisualConfig));
	}
	if (TickResult.bCompleted)
	{
		ClearSurfaceDeparturePlayback();
	}
	else
	{
		ApplyVisualSlotView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ClearSurfaceDeparturePlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectDeparture);
	if (SurfaceDeparturePlayback)
	{
		SurfaceDeparturePlayback->Reset();
	}
	SurfaceDepartureTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	CardUseFlipProgress = 0.0f;
	CardUseImpactProgress = 0.0f;
	CardUseMotionAlpha = 0.0f;
	CardUseOpacityMultiplier = 1.0f;
	ApplyActiveSurfaceEffectView();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingSurfaceDepartureSound()
{
	if (!SurfaceDeparturePlayback)
	{
		return;
	}
	const EWacomFirstPersonCardSurfaceDepartureKind Kind = SurfaceDeparturePlayback->GetKind();
	const TOptional<FWacomFirstPersonCardSurfaceDepartureSoundRequest> PendingRequest =
		SurfaceDeparturePlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}

	const FWacomFirstPersonCardSurfaceDepartureSoundRequest& Request = PendingRequest.GetValue();
#if WITH_AUTOMATION_TESTS
	if (Kind == EWacomFirstPersonCardSurfaceDepartureKind::CardUse)
	{
		++CardUseEffectSoundRequestCountForTest;
		LastCardUseEffectSoundPitchMultiplierForTest = Request.PitchMultiplier;
	}
	else if (Kind == EWacomFirstPersonCardSurfaceDepartureKind::ExhaustDissolve)
	{
		++PlayedDissolveSoundRequestCountForTest;
		LastPlayedDissolveSoundPitchMultiplierForTest = Request.PitchMultiplier;
	}
#endif
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsSurfaceDeparturePlaybackActive() const
{
	return SurfaceDeparturePlayback && SurfaceDeparturePlayback->IsActive();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCardUseReformFeedback()
{
	TriggerCardUseReformFeedbackInternal(false, false);
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCardUseReformOutFeedback()
{
	TriggerCardUseReformFeedbackInternal(true, false);
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCardUseReformInFeedback()
{
	TriggerCardUseReformFeedbackInternal(false, true);
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerCardUseReformFeedbackInternal(
	bool bOutboundOnly,
	bool bInboundOnly)
{
	if (!CanPlayCardUseReformEffect() || bIsExitingForFirstPersonLayer || !bHasVisualSlotView)
	{
		return;
	}

	if (!bInboundOnly)
	{
		ClearCardUseReformPlayback();
	}
	ClearCardDataRewritePlayback();
	ClearEffectBadgeFeedbackPlayback();
	ClearEnterTransitionPlayback();
	ClearExitTransitionPlayback();
	ClearSurfaceDeparturePlayback();
	if (!CardUseReformPlayback)
	{
		CardUseReformPlayback.Reset(new FWacomFirstPersonCardUseReformPlayback());
	}

	const FWacomFirstPersonCardUseEffectConfig& Config = SlotVisualConfig.CardUseEffect;
	FWacomFirstPersonCardUseReformPlaybackConfig PlaybackConfig;
	PlaybackConfig.EffectKind = Config.Style.EffectKind;
	PlaybackConfig.DissolveOutSeconds = Config.Style.EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		? Config.Style.EdgeFlipReformOutSeconds
		: Config.Style.ReformDissolveOutSeconds;
	PlaybackConfig.HiddenHoldSeconds = Config.Style.EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		? Config.Style.EdgeFlipReformHiddenHoldSeconds
		: Config.Style.ReformHiddenHoldSeconds;
	PlaybackConfig.ReformSeconds = Config.Style.EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		? Config.Style.EdgeFlipReformInSeconds
		: Config.Style.ReformBuildInSeconds;
	PlaybackConfig.SettleSeconds = Config.Style.EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		? Config.Style.EdgeFlipReformSettleSeconds
		: 0.0f;
	PlaybackConfig.ImpactSeconds = Config.Style.EdgeFlipImpactSeconds;
	PlaybackConfig.bReducedMotion = Config.bReducedMotion;
	PlaybackConfig.StartSound = Config.Style.StartSound;
	PlaybackConfig.SoundVolumeMultiplier = Config.Style.StartSoundVolumeMultiplier;
	PlaybackConfig.SoundPitchMultiplier = Config.Style.StartSoundPitchMultiplier;
	PlaybackConfig.SoundPitchVariation = Config.Style.StartSoundPitchVariation;
	if (!bInboundOnly)
	{
		CardUseReformStartSlotView = VisualSlotView;
		CardUseReformStartSlotView.RenderOpacity = 1.0f;
	}
	if (bOutboundOnly)
	{
		CardUseReformPlayback->BeginOutbound(PlaybackConfig);
	}
	else if (bInboundOnly)
	{
		CardUseReformPlayback->BeginInbound(PlaybackConfig);
	}
	else
	{
		CardUseReformPlayback->Begin(PlaybackConfig);
	}
	if (!CardUseReformPlayback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardUseReformTickResult InitialView =
		CardUseReformPlayback->BuildView();
	CardUseFlipProgress = InitialView.FlipProgress;
	CardUseImpactProgress = InitialView.ImpactProgress;
	CardUseMotionAlpha = InitialView.MotionAlpha;
	CardUseOpacityMultiplier = InitialView.OpacityMultiplier;

	VisualSlotView = InitialView.bUseTargetSlotPosition
		? TargetSlotView
		: CardUseReformStartSlotView;
	VisualSlotView.RenderOpacity = 1.0f;
	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(BuildCardUseReformView(
			CardUseReformPlayback->BuildView(),
			SlotVisualConfig));
	}
	BeginSurfacePresentationReadiness(SurfaceEffectCardUseReform);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TickCardUseReformPlayback(float DeltaTime)
{
	if (!CardUseReformPlayback || !CardUseReformPlayback->IsActive())
	{
		return;
	}

	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardUseReformTickResult TickResult =
		CardUseReformPlayback->Tick(PlaybackDeltaTime);
	PlayPendingCardUseReformSound();
	CardUseFlipProgress = TickResult.FlipProgress;
	CardUseImpactProgress = TickResult.ImpactProgress;
	CardUseMotionAlpha = TickResult.MotionAlpha;
	CardUseOpacityMultiplier = TickResult.OpacityMultiplier;
	VisualSlotView = TickResult.bUseTargetSlotPosition
		? TargetSlotView
		: CardUseReformStartSlotView;
	VisualSlotView.RenderOpacity = 1.0f;
	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(BuildCardUseReformView(
			TickResult,
			SlotVisualConfig));
	}
	if (TickResult.bCompleted)
	{
		ClearCardUseReformPlayback(true);
	}
	else
	{
		ApplyVisualSlotView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ClearCardUseReformPlayback(bool bSnapToTarget)
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectCardUseReform);
	if (CardUseReformPlayback)
	{
		CardUseReformPlayback->Reset();
	}
	CardUseFlipProgress = 0.0f;
	CardUseImpactProgress = 0.0f;
	CardUseMotionAlpha = 0.0f;
	CardUseOpacityMultiplier = 1.0f;
	if (bSnapToTarget && bHasVisualSlotView)
	{
		VisualSlotView = TargetSlotView;
		VisualSlotView.RenderOpacity = TargetSlotView.RenderOpacity;
	}
	ApplyActiveSurfaceEffectView();
	if (bSnapToTarget)
	{
		ApplyVisualSlotView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingCardUseReformSound()
{
	if (!CardUseReformPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardUseReformSoundRequest> PendingRequest =
		CardUseReformPlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}

	const FWacomFirstPersonCardUseReformSoundRequest& Request = PendingRequest.GetValue();
#if WITH_AUTOMATION_TESTS
	++CardUseEffectSoundRequestCountForTest;
	++CardUseReformSoundRequestCountForTest;
	LastCardUseEffectSoundPitchMultiplierForTest = Request.PitchMultiplier;
#endif
	if (USoundBase* Sound = Request.Sound.Get(); Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsCardUseReformPlaybackActive() const
{
	return CardUseReformPlayback && CardUseReformPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsCardUseReformPlaybackBlockingStage() const
{
	return CardUseReformPlayback && CardUseReformPlayback->IsBlockingStage();
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
	GestureElapsedSeconds = FMath::Max(
		GestureElapsedSeconds,
		CardDragConfig.CardInspectHoldDelaySeconds);
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
		if (HandTargetImpactPlayback && HandTargetImpactPlayback->IsActive())
		{
			View.HandTargetImpactView = BuildHandTargetImpactView(
				HandTargetImpactPlayback->BuildView(),
				SlotVisualConfig,
				CurrentSlotView.Entry.CardInstanceId).HandTargetImpact;
		}
		if (DataRewritePlayback && DataRewritePlayback->IsActive())
		{
			View.DataRewriteView = BuildDataRewriteView(
				DataRewritePlayback->BuildView(),
				SlotVisualConfig);
		}
		if (DrawRevealPlayback && DrawRevealPlayback->IsActive())
		{
			View.DrawRevealView = BuildDrawRevealView(
				DrawRevealPlayback->BuildView(),
				SlotVisualConfig).DrawReveal;
		}
		if (GainRevealPlayback && GainRevealPlayback->IsActive())
		{
			View.GainRevealView = BuildGainRevealView(
				GainRevealPlayback->BuildView(),
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
		&& SurfaceDepartureTransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played;
	View.bPlayedDissolvePlaybackActive = IsSurfaceDeparturePlaybackActive()
		&& SurfaceDepartureTransitionKind == EWacomFirstPersonCardSlotTransitionKind::Exhausted;
	View.bCardUseReformPlaybackActive = IsCardUseReformPlaybackActive();
	View.bCardUseReformUsingTargetSlot = IsCardUseReformPlaybackActive()
		&& CardUseReformPlayback->BuildView().bUseTargetSlotPosition;
	View.bHandTargetImpactCommitActive = IsHandTargetImpactCommitPlaybackActive();
	View.bDataRewritePlaybackActive = IsCardDataRewritePlaybackActive();
	View.bDataRewritePendingHandoff = bPendingDataRewriteHandoff;
	View.bEffectBadgeFeedbackPlaybackActive = IsEffectBadgeFeedbackPlaybackActive();
	if (EffectBadgeFeedbackPlayback && EffectBadgeFeedbackPlayback->IsActive())
	{
		View.EffectBadgeFeedbackView = EffectBadgeFeedbackPlayback->BuildView();
	}
	View.bDrawRevealPlaybackActive = IsDrawRevealPlaybackActive();
	if (DrawRevealPlayback && DrawRevealPlayback->IsActive())
	{
		const FWacomFirstPersonCardDrawRevealPlaybackView RevealView =
			DrawRevealPlayback->BuildView();
		View.bDrawRevealWaiting = RevealView.Phase
			== EWacomFirstPersonCardDrawRevealPhase::Waiting;
		View.DrawRevealProgress = RevealView.Progress;
		View.DrawRevealHorizontalScale = RevealView.HorizontalScale;
		View.DrawRevealLandingScale = RevealView.LandingScale;
		View.DrawRevealLandingTranslationYPixels =
			RevealView.LandingTranslationYPixels;
	}
	View.bGainRevealPlaybackActive = IsGainRevealPlaybackActive();
	if (GainRevealPlayback && GainRevealPlayback->IsActive())
	{
		const FWacomFirstPersonCardGainRevealPlaybackView RevealView =
			GainRevealPlayback->BuildView();
		View.bGainRevealWaiting = RevealView.Phase
			== EWacomFirstPersonCardGainRevealPhase::Waiting;
		View.GainRevealProgress = RevealView.Progress;
	}
	View.bHandTargetDeparturePending = bHandTargetImpactDeparturePending;
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
	View.GestureSource = GestureSource;
	View.bPressed = bIsPressedForFirstPersonLayer;
	View.bDenyFeedbackActive = DenyFeedbackElapsedSeconds < SlotFeedbackConfig.DenyDuration;
	View.bConfirmFeedbackActive = ConfirmFeedbackElapsedSeconds < SlotFeedbackConfig.ConfirmDuration;
	View.bCommitFeedbackActive = CommitFeedbackElapsedSeconds < SlotFeedbackConfig.PlayCommitDuration;
	View.bRetainedFeedbackActive = IsRetainedFeedbackActive();
	if (RetainSealPlayback && RetainSealPlayback->IsActive())
	{
		const FWacomFirstPersonCardRetainSealPlaybackView RetainView =
			RetainSealPlayback->BuildView();
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
		? TransitionPlayback->GetElapsedSeconds()
		: 0.0f;
	View.EnterTransitionStartDelaySeconds = IsEnterTransitionPlaybackActive()
		? TransitionPlayback->GetStartDelaySeconds()
		: 0.0f;
	View.EnterTransitionDurationSeconds = IsEnterTransitionPlaybackActive()
		? TransitionPlayback->GetDurationSeconds()
		: 0.0f;
	View.bExitTransitionPlaybackActive = IsExitTransitionPlaybackActive();
	View.ExitTransitionElapsedSeconds = IsExitTransitionPlaybackActive()
		? TransitionPlayback->GetElapsedSeconds()
		: 0.0f;
	View.ExitTransitionStartDelaySeconds = IsExitTransitionPlaybackActive()
		? TransitionPlayback->GetStartDelaySeconds()
		: 0.0f;
	View.ExitTransitionDurationSeconds = IsExitTransitionPlaybackActive()
		? TransitionPlayback->GetDurationSeconds()
		: 0.0f;
	View.EnterTransitionSoundRequestCount = EnterTransitionSoundRequestCountForTest;
	View.LastEnterTransitionSoundKind = LastEnterTransitionSoundKindForTest;
	View.SlotMotionConfig = SlotMotionConfig;
	View.CardDragConfig = CardDragConfig;
	View.SlotVisualConfig = SlotVisualConfig;
	View.SlotMotionConfigApplyCount = SlotMotionConfigApplyCountForTest;
	View.SlotVisualConfigApplyCount = SlotVisualConfigApplyCountForTest;
	View.SlotFeedbackConfigApplyCount = SlotFeedbackConfigApplyCountForTest;
	View.CardDragConfigApplyCount = CardDragConfigApplyCountForTest;
	View.SurfaceReadinessState = SurfaceReadinessGate
		? static_cast<int32>(SurfaceReadinessGate->GetState())
		: 0;
	View.CostDigitReadinessState = CostDigitReadinessGate
		? static_cast<int32>(CostDigitReadinessGate->GetState())
		: 0;
	View.EffectBadgeReadinessState = EffectBadgeReadinessGate
		? static_cast<int32>(EffectBadgeReadinessGate->GetState())
		: 0;
	View.SurfaceReadinessGeneration = SurfaceReadinessGate
		? SurfaceReadinessGate->GetGeneration()
		: 0;
	View.CostDigitReadinessGeneration = CostDigitReadinessGate
		? CostDigitReadinessGate->GetGeneration()
		: 0;
	View.EffectBadgeReadinessGeneration = EffectBadgeReadinessGate
		? EffectBadgeReadinessGate->GetGeneration()
		: 0;
	View.bPlaybackFrozenForReadiness = bPlaybackFrozenForReadiness;
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
	if (GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled)
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
	int32 SequenceCount,
	bool bRetainUntilExplicitRelease)
{
	if (!SlotFeedbackConfig.bEnabled
		|| !SlotFeedbackConfig.bEnableRetainedFeedback
		|| !SlotVisualConfig.RetainSeal.bEnabled
		|| SlotFeedbackConfig.RetainedFeedbackDuration <= 0.0f)
	{
		return;
	}
	const int32 SafeSequenceIndex = FMath::Clamp(
		SequenceIndex,
		0,
		FMath::Max(0, SequenceCount - 1));
	if (!RetainSealPlayback)
	{
		RetainSealPlayback.Reset(new FWacomFirstPersonCardRetainSealPlayback());
	}
	FWacomFirstPersonCardRetainSealPlaybackConfig Config;
	Config.bEnabled = true;
	Config.bReducedMotion = SlotVisualConfig.RetainSeal.bReducedMotion;
	Config.bRetainUntilExplicitRelease = bRetainUntilExplicitRelease;
	Config.StartDelaySeconds =
		static_cast<float>(SafeSequenceIndex) * SlotFeedbackConfig.RetainedFeedbackStaggerSeconds;
	Config.SealingDurationSeconds = SlotFeedbackConfig.RetainedFeedbackDuration;
	Config.ReleaseDurationSeconds = SlotFeedbackConfig.RetainedFeedbackReleaseDuration;
	Config.PeakLiftPixels = SlotFeedbackConfig.RetainedFeedbackLiftPixels;
	Config.PeakScale = SlotFeedbackConfig.RetainedFeedbackScale;
	Config.HeldLiftPixels = SlotFeedbackConfig.RetainedFeedbackHeldLiftPixels;
	Config.HeldScale = SlotFeedbackConfig.RetainedFeedbackHeldScale;
	Config.Seed = static_cast<float>(GetTypeHash(CurrentSlotView.Entry.CardInstanceId) & 0xFFFFu)
		/ 65535.0f;
	Config.Style = SlotVisualConfig.RetainSeal.Style;
	RetainSealPlayback->Begin(Config);
	bSuppressRetainSealSurfaceForReadinessFailure = false;
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectRetainSeal);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerRetainedReleaseFeedback()
{
	if (!RetainSealPlayback || !RetainSealPlayback->IsActive())
	{
		return;
	}
	RetainSealPlayback->Release();
	bSuppressRetainSealSurfaceForReadinessFailure = false;
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectRetainSeal);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearInteractionFeedback()
{
	bIsPressedForFirstPersonLayer = false;
	ResetDragPickupFeedback();
	ClearCardDragTargetFeedback();
	bHasFeedbackTargetScreenPosition = false;
	FeedbackTargetScreenPosition = FVector2D::ZeroVector;
	ConfirmFeedbackElapsedSeconds = SlotFeedbackConfig.ConfirmDuration;
	DenyFeedbackElapsedSeconds = SlotFeedbackConfig.DenyDuration;
	CommitFeedbackElapsedSeconds = SlotFeedbackConfig.PlayCommitDuration;
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
	return IsRetainSealPlaybackActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::HasActivePresentationPlayback() const
{
	const bool bBlockingPresentationReadiness =
		(SurfaceReadinessGate && SurfaceReadinessGate->IsPending()
			&& bSurfaceReadinessBlocksPresentationPhase)
		|| (CostDigitReadinessGate && CostDigitReadinessGate->IsPending())
		|| (EffectBadgeReadinessGate && EffectBadgeReadinessGate->IsPending());
	return bBlockingPresentationReadiness
		|| IsEnterTransitionPlaybackActive()
		|| IsExitingForFirstPersonLayer()
		|| IsCardUseReformPlaybackBlockingStage()
		|| IsHandTargetImpactCommitPlaybackActive()
		|| bHandTargetImpactDeparturePending
		|| IsRetainSealPlaybackBlockingPresentation()
		|| (bDataRewriteBlocksPresentationPhase
			&& (IsCardDataRewritePlaybackActive() || bPendingDataRewriteHandoff))
		|| (bEffectBadgeFeedbackBlocksPresentationPhase
			&& IsEffectBadgeFeedbackPlaybackActive());
}

void UWacomFirstPersonCardLayerSlotWidget::ForceCompletePresentationPlayback()
{
	CancelAllPresentationReadiness();
	ClearEnterTransitionPlayback();
	if (IsCardUseReformPlaybackActive())
	{
		ClearCardUseReformPlayback(true);
	}
	if (IsHandTargetImpactPlaybackActive() || bHandTargetImpactDeparturePending)
	{
		ClearHandTargetImpactPlayback();
	}
	if (IsCardDataRewritePlaybackActive() || bPendingDataRewriteHandoff)
	{
		ClearCardDataRewritePlayback();
	}
	if (IsEffectBadgeFeedbackPlaybackActive())
	{
		ClearEffectBadgeFeedbackPlayback();
	}
	if (bIsExitingForFirstPersonLayer)
	{
		VisualSlotView = TargetSlotView;
		VisualSlotView.bProjected = false;
		ApplyVisualSlotView();
		ClearExitTransitionPlayback();
		ClearSurfaceDeparturePlayback();
		bUsesFixedExitTransitionPlayback = false;
		bUsesSurfaceDepartureExit = false;
		SurfaceDepartureTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
		ExitMotionElapsedSeconds = FMath::Max(0.0f, SlotMotionConfig.ExitDuration);
	}
	else if (bHasVisualSlotView)
	{
		VisualSlotView = GetEffectiveTargetSlotView();
		ApplyVisualSlotView();
	}
	ClearRetainSealPlayback();
	ApplyFeedbackOverlay();
	UpdateWantsTick();
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
		const bool bSourceInteractionFeedbackActive =
			bIsPressedForFirstPersonLayer
			|| ConfirmAlpha > 0.0f
			|| CommitAlpha > 0.0f;
		if (!bSourceInteractionFeedbackActive && CanApplyPlayableHoverFeedback())
		{
			OverlayColor = SlotFeedbackConfig.PlayableHoverColor;
			OverlayOpacity = SlotFeedbackConfig.PlayableHoverOpacity;
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
	const bool bPressedFeedbackActive =
		SlotFeedbackConfig.bEnabled && bIsPressedForFirstPersonLayer;

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
		|| IsRetainSealPlaybackBlockingPresentation();
	const FWacomFirstPersonCardLayerSlotView& EffectiveTargetSlotView = GetEffectiveTargetSlotView();
	const bool bGestureActive =
		GestureState != EWacomFirstPersonCardGestureState::Idle
		&& GestureState != EWacomFirstPersonCardGestureState::Cancelled;
	bWantsSlotMotionTick = IsEnterTransitionPlaybackActive()
		|| bPlaybackFrozenForReadiness
		|| bIsExitingForFirstPersonLayer
		|| IsCardUseReformPlaybackBlockingStage()
		|| IsHandTargetImpactPlaybackActive()
		|| IsCardDataRewritePlaybackActive()
		|| IsEffectBadgeFeedbackPlaybackActive()
		|| bPendingDataRewriteHandoff
		|| bHandTargetImpactDeparturePending
		|| IsDrawRevealPlaybackActive()
		|| IsGainRevealPlaybackActive()
		|| IsRetainSealPlaybackBlockingPresentation()
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

void UWacomFirstPersonCardLayerSlotWidget::StartEnterTransitionPlayback(
	const FWacomFirstPersonCardLayerSlotView& StartSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& EnterProfile)
{
	if (!TransitionPlayback)
	{
		TransitionPlayback.Reset(new FWacomFirstPersonCardTransitionPlayback());
	}
	EnterTransitionStartWidgetPosition = StartSlotView.WidgetPosition;
	TransitionPlayback->BeginEnter(StartSlotView, EnterProfile);
	if (TransitionPlayback->IsEnterActive())
	{
		PrepareDrawRevealPlayback(EnterProfile.TransitionKind);
		PrepareGainRevealPlayback(EnterProfile.TransitionKind);
	}
	else
	{
		ClearDrawRevealPlayback();
		ClearGainRevealPlayback();
	}
	if (!IsDrawRevealPlaybackActive() && !IsGainRevealPlaybackActive())
	{
		BroadcastPendingEnterTransitionStarted();
		PlayPendingTransitionStartSound();
	}
}

void FWacomFirstPersonCardHandTargetImpactPlaybackDeleter::operator()(
	FWacomFirstPersonCardHandTargetImpactPlayback* Playback) const
{
	delete Playback;
}

void FWacomFirstPersonCardDrawRevealPlaybackDeleter::operator()(
	FWacomFirstPersonCardDrawRevealPlayback* Playback) const
{
	delete Playback;
}

void FWacomFirstPersonCardGainRevealPlaybackDeleter::operator()(
	FWacomFirstPersonCardGainRevealPlayback* Playback) const
{
	delete Playback;
}

void FWacomFirstPersonCardRetainSealPlaybackDeleter::operator()(
	FWacomFirstPersonCardRetainSealPlayback* Playback) const
{
	delete Playback;
}

void FWacomFirstPersonCardDataRewritePlaybackDeleter::operator()(
	FWacomFirstPersonCardDataRewritePlayback* Playback) const
{
	delete Playback;
}

void FWacomFirstPersonCardEffectBadgeFeedbackPlaybackDeleter::operator()(
	FWacomFirstPersonCardEffectBadgeFeedbackPlayback* Playback) const
{
	delete Playback;
}

void UWacomFirstPersonCardLayerSlotWidget::ClearEnterTransitionPlayback()
{
	if (TransitionPlayback)
	{
		TransitionPlayback->ResetIfMode(EWacomFirstPersonCardTransitionPlaybackMode::Enter);
	}
	ClearDrawRevealPlayback();
	ClearGainRevealPlayback();
	EnterTransitionStartWidgetPosition = FVector2D::ZeroVector;
}

bool UWacomFirstPersonCardLayerSlotWidget::TickEnterTransitionPlayback(float DeltaTime)
{
	if (!IsEnterTransitionPlaybackActive())
	{
		return true;
	}
	float PlaybackDeltaTime = DeltaTime;
	if ((IsDrawRevealPlaybackActive() || IsGainRevealPlaybackActive())
		&& !ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return false;
	}
	const FWacomFirstPersonCardTransitionTickResult Result =
		TransitionPlayback->Tick(PlaybackDeltaTime, GetEffectiveTargetSlotView());
	BroadcastPendingEnterTransitionStarted();
	PlayPendingTransitionStartSound();
	if (Result.bHasPlaybackProgress)
	{
		UpdateDrawRevealPlayback(Result.NormalizedPlaybackProgress);
		UpdateGainRevealPlayback(Result.NormalizedPlaybackProgress);
	}
	if (Result.bHasVisualSlotView)
	{
		VisualSlotView = Result.VisualSlotView;
		ApplyVisualSlotView();
	}
	if (Result.bCompleted)
	{
		ClearDrawRevealPlayback();
		ClearGainRevealPlayback();
	}
	return Result.bCompleted;
}

void UWacomFirstPersonCardLayerSlotWidget::BroadcastPendingEnterTransitionStarted()
{
	if (!TransitionPlayback)
	{
		return;
	}
	const TOptional<EWacomFirstPersonCardSlotTransitionKind> PendingRequest =
		TransitionPlayback->ConsumePendingStartRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}
	if (PendingRequest.GetValue() == EWacomFirstPersonCardSlotTransitionKind::Drawn)
	{
		StartDrawRevealPlayback();
	}
	else if (PendingRequest.GetValue() == EWacomFirstPersonCardSlotTransitionKind::Gained)
	{
		StartGainRevealPlayback();
	}

	FWacomFirstPersonCardEnterTransitionStartedView View;
	View.CardInstanceId = CurrentSlotView.Entry.CardInstanceId;
	View.TransitionKind = PendingRequest.GetValue();
	View.StartWidgetPosition = EnterTransitionStartWidgetPosition;
	View.TargetWidgetPosition = GetEffectiveTargetSlotView().WidgetPosition;
	OnEnterTransitionStartedNative.Broadcast(View);
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingTransitionStartSound()
{
	if (!TransitionPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardTransitionSoundRequest> PendingRequest =
		TransitionPlayback->ConsumePendingSoundRequest();
	if (!PendingRequest.IsSet())
	{
		return;
	}
	const FWacomFirstPersonCardTransitionSoundRequest& Request = PendingRequest.GetValue();
#if WITH_AUTOMATION_TESTS
	++EnterTransitionSoundRequestCountForTest;
	LastEnterTransitionSoundKindForTest = Request.TransitionKind;
#endif
	USoundBase* Sound = Request.Sound.Get();
	if (!Sound)
	{
		Sound = Request.Sound.LoadSynchronous();
	}
	if (Sound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(
			GetWorld(),
			Sound,
			Request.VolumeMultiplier,
			Request.PitchMultiplier);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsEnterTransitionPlaybackActive() const
{
	return TransitionPlayback && TransitionPlayback->IsEnterActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsEnterTransitionBlockingInteraction() const
{
	return TransitionPlayback && TransitionPlayback->BlocksInteraction();
}

void UWacomFirstPersonCardLayerSlotWidget::StartExitTransitionPlayback(
	const FWacomFirstPersonCardLayerSlotView& StartSlotView,
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& ExitProfile)
{
	if (!TransitionPlayback)
	{
		TransitionPlayback.Reset(new FWacomFirstPersonCardTransitionPlayback());
	}
	TransitionPlayback->BeginExit(StartSlotView, InTargetSlotView, ExitProfile);
	if (!TransitionPlayback->IsExitActive())
	{
		VisualSlotView = InTargetSlotView;
		ApplyVisualSlotView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ClearExitTransitionPlayback()
{
	if (TransitionPlayback)
	{
		TransitionPlayback->ResetIfMode(EWacomFirstPersonCardTransitionPlaybackMode::Exit);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::TickExitTransitionPlayback(float DeltaTime)
{
	if (!IsExitTransitionPlaybackActive())
	{
		return true;
	}
	const FWacomFirstPersonCardTransitionTickResult Result =
		TransitionPlayback->Tick(DeltaTime, GetEffectiveTargetSlotView());
	ExitMotionElapsedSeconds = TransitionPlayback->GetElapsedSeconds();
	if (Result.bHasVisualSlotView)
	{
		VisualSlotView = Result.VisualSlotView;
		ApplyVisualSlotView();
	}
	return Result.bCompleted;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsExitTransitionPlaybackActive() const
{
	return TransitionPlayback && TransitionPlayback->IsExitActive();
}
