// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/Card/WacomFirstPersonCardDataRewritePlayback.h"
#include "UI/Card/WacomFirstPersonCardDrawRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardEffectBadgeFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardGainRevealPlayback.h"
#include "UI/Card/WacomFirstPersonCardHandTargetImpactPlayback.h"
#include "UI/Card/WacomFirstPersonCardInteractionFeedbackPlayback.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardRetainSealPlayback.h"
#include "UI/Card/WacomFirstPersonCardSlotPresentationController.h"
#include "UI/Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"
#include "UI/Card/WacomFirstPersonCardSurfaceEffectViewBuilder.h"
#include "UI/Card/WacomFirstPersonCardTransitionPlayback.h"
#include "UI/Card/WacomFirstPersonCardUseReformPlayback.h"
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
	constexpr EWacomFirstPersonCardPresentationReadinessChannel SurfaceReadinessChannel =
		EWacomFirstPersonCardPresentationReadinessChannel::Surface;
	constexpr EWacomFirstPersonCardPresentationReadinessChannel CostDigitReadinessChannel =
		EWacomFirstPersonCardPresentationReadinessChannel::CostDigit;
	constexpr EWacomFirstPersonCardPresentationReadinessChannel EffectBadgeReadinessChannel =
		EWacomFirstPersonCardPresentationReadinessChannel::EffectBadge;
}
void UWacomFirstPersonCardLayerSlotWidget::ResetCardSurfaceEffectView()
{
	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(FWacomFirstPersonCardSurfaceEffectView());
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
	if (!PresentationController->HandTargetImpactPlayback)
	{
		PresentationController->HandTargetImpactPlayback.Reset(new FWacomFirstPersonCardHandTargetImpactPlayback());
	}
	PresentationController->HandTargetImpactPlayback->BeginPreview(SlotVisualConfig.HandTargetImpact);
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectHandTargetImpact, false, false);
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::EndHandTargetImpactPreview()
{
	if (!PresentationController->HandTargetImpactPlayback || PresentationController->HandTargetImpactPlayback->IsCommitActive())
	{
		return;
	}
	PresentationController->HandTargetImpactPlayback->EndPreview();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerHandTargetImpactFeedback()
{
	if (!CanPlayHandTargetImpact() || !bHasVisualSlotView)
	{
		return;
	}
	if (!PresentationController->HandTargetImpactPlayback)
	{
		PresentationController->HandTargetImpactPlayback.Reset(new FWacomFirstPersonCardHandTargetImpactPlayback());
	}
	ClearCardDragTargetFeedback();
	if (CardView)
	{
		CardView->ResetCostDigitPreviewView();
	}
	const uint32 Seed = GetTypeHash(CurrentSlotView.Entry.CardInstanceId);
	PresentationController->HandTargetImpactPlayback->BeginCommit(SlotVisualConfig.HandTargetImpact, Seed);
	PresentationController->State.bHandTargetImpactDepartureGateReleased = false;
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
		&& !PresentationController->State.bHandTargetImpactDepartureGateReleased)
	{
		PresentationController->State.PendingDataRewriteFieldMask |= FieldMask;
		PresentationController->State.PendingDataRewriteTone = Tone;
		PresentationController->State.PendingDataRewriteSeed = Seed;
		PresentationController->State.PendingDataRewriteSequenceIndex = FMath::Max(0, SequenceIndex);
		PresentationController->State.PendingDataRewriteSequenceCount = FMath::Max(1, SequenceCount);
		PresentationController->State.bPendingDataRewriteHandoff = true;
		PresentationController->State.bDataRewriteBlocksPresentationPhase = bBlocksPresentationPhase;
		UpdateWantsTick();
		return;
	}

	BeginCardDataRewritePlayback(
		FieldMask,
		Tone,
		Seed,
		SequenceIndex,
		true);
	PresentationController->State.bDataRewriteBlocksPresentationPhase = bBlocksPresentationPhase;
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
	if (!PresentationController->EffectBadgeFeedbackPlayback)
	{
		PresentationController->EffectBadgeFeedbackPlayback.Reset(
			new FWacomFirstPersonCardEffectBadgeFeedbackPlayback());
	}
	PresentationController->EffectBadgeFeedbackPlayback->Begin(SlotVisualConfig.EffectBadgeFeedback, Changes);
	PresentationController->State.bEffectBadgeFeedbackBlocksPresentationPhase =
		bBlocksPresentationPhase && PresentationController->EffectBadgeFeedbackPlayback->IsActive();
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
	PresentationController->State.DeferredHandTargetExitSlotView = InExitTargetSlotView;
	PresentationController->State.DeferredHandTargetExitProfile = ExitProfileOverride;
	PresentationController->State.DeferredHandTargetExitTransitionKind = TransitionKind;
	PresentationController->State.bHandTargetImpactDeparturePending = true;
	PresentationController->State.bHandTargetImpactDepartureOwnedByPileTransfer = false;
	PresentationController->State.bHandTargetImpactDepartureGateReleased = false;
	TriggerHandTargetImpactFeedback();
	UpdateWantsTick();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsHandTargetImpactDeparturePending() const
{
	return PresentationController
		&& PresentationController->State.bHandTargetImpactDeparturePending;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsHandTargetImpactDepartureGateOpen() const
{
	return PresentationController->State.bHandTargetImpactDeparturePending
		&& (PresentationController->State.bHandTargetImpactDepartureGateReleased
			|| (PresentationController->HandTargetImpactPlayback
				&& PresentationController->HandTargetImpactPlayback->IsDepartureGateOpen()));
}

void UWacomFirstPersonCardLayerSlotWidget::SetHandTargetImpactDepartureOwnedByPileTransfer(
	bool bOwned)
{
	PresentationController->State.bHandTargetImpactDepartureOwnedByPileTransfer =
		bOwned && PresentationController->State.bHandTargetImpactDeparturePending;
}

void UWacomFirstPersonCardLayerSlotWidget::ReleaseDeferredHandTargetExitNow()
{
	if (!PresentationController->State.bHandTargetImpactDeparturePending)
	{
		return;
	}
	const FWacomFirstPersonCardLayerSlotView ExitSlotView = PresentationController->State.DeferredHandTargetExitSlotView;
	const TOptional<FWacomFirstPersonCardTransitionMotionProfile> ExitProfile =
		PresentationController->State.DeferredHandTargetExitProfile;
	const EWacomFirstPersonCardSlotTransitionKind TransitionKind =
		PresentationController->State.DeferredHandTargetExitTransitionKind;
	PresentationController->State.bHandTargetImpactDeparturePending = false;
	BeginExitMotionWithProfile(ExitSlotView, ExitProfile, TransitionKind);
}

void UWacomFirstPersonCardLayerSlotWidget::TickHandTargetImpactPlayback(float DeltaTime)
{
	if (!PresentationController->HandTargetImpactPlayback || !PresentationController->HandTargetImpactPlayback->IsActive())
	{
		return;
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardHandTargetImpactPlaybackView View =
		PresentationController->HandTargetImpactPlayback->Tick(PlaybackDeltaTime);
	PresentationController->State.bHandTargetImpactDepartureGateReleased =
		PresentationController->State.bHandTargetImpactDepartureGateReleased || View.bDepartureGateOpen;
	if (!PresentationController->HandTargetImpactPlayback->IsActive())
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
		PresentationController->HandTargetImpactPlayback->Reset();
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
	if (PresentationController->HandTargetImpactPlayback)
	{
		PresentationController->HandTargetImpactPlayback->Reset();
	}
	HandTargetImpactScaleMultiplier = 1.0f;
	HandTargetImpactTranslationYPixels = 0.0f;
	HandTargetImpactZOrderBoost = 0;
	PresentationController->State.bHandTargetImpactDeparturePending = false;
	PresentationController->State.bHandTargetImpactDepartureOwnedByPileTransfer = false;
	PresentationController->State.bHandTargetImpactDepartureGateReleased = false;
	PresentationController->State.DeferredHandTargetExitProfile.Reset();
	PresentationController->State.DeferredHandTargetExitTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	if (CardView)
	{
		CardView->ResetCostDigitPreviewView();
	}
	ApplyActiveSurfaceEffectView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyHandTargetImpactSurfaceView()
{
	if (!CardView || !PresentationController->HandTargetImpactPlayback || !PresentationController->HandTargetImpactPlayback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardHandTargetImpactPlaybackView PlaybackView =
		PresentationController->HandTargetImpactPlayback->BuildView();
	CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildHandTargetImpactView(
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
	if (!PresentationController->DataRewritePlayback)
	{
		PresentationController->DataRewritePlayback.Reset(new FWacomFirstPersonCardDataRewritePlayback());
	}
	PresentationController->DataRewritePlayback->BeginOrRetarget(
		SlotVisualConfig.DataRewrite,
		FieldMask,
		Tone,
		static_cast<uint32>(Seed),
		SequenceIndex,
		bAllowSequenceDelay);
	PresentationController->State.PendingDataRewriteFieldMask = 0;
	PresentationController->State.PendingDataRewriteTone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	PresentationController->State.PendingDataRewriteSeed = 0;
	PresentationController->State.PendingDataRewriteSequenceIndex = 0;
	PresentationController->State.PendingDataRewriteSequenceCount = 1;
	PresentationController->State.bPendingDataRewriteHandoff = false;
	if (!PresentationController->DataRewritePlayback || !PresentationController->DataRewritePlayback->IsActive())
	{
		PresentationController->State.bDataRewriteBlocksPresentationPhase = false;
	}
	ApplyCardDataRewriteView();
	if (PresentationController->DataRewritePlayback->BuildView().bActive)
	{
		BeginCostDigitPresentationReadiness();
	}
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TickCardDataRewritePlayback(float DeltaTime)
{
	if (!PresentationController->DataRewritePlayback || !PresentationController->DataRewritePlayback->IsActive())
	{
		return;
	}
	if (!PresentationController->DataRewritePlayback->BuildView().bActive)
	{
		const FWacomFirstPersonCardDataRewritePlaybackView DelayView =
			PresentationController->DataRewritePlayback->Tick(DeltaTime);
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
		PresentationController->DataRewritePlayback->Tick(PlaybackDeltaTime);
	PlayPendingCardDataRewriteSound();
	if (View.bCompleted || !PresentationController->DataRewritePlayback->IsActive())
	{
		PresentationController->DataRewritePlayback->Reset();
		if (CardView)
		{
			CardView->ResetCardDataRewriteView();
		}
		PresentationController->State.bDataRewriteBlocksPresentationPhase = false;
		UpdateWantsTick();
		return;
	}
	ApplyCardDataRewriteView();
}

void UWacomFirstPersonCardLayerSlotWidget::ClearCardDataRewritePlayback()
{
	CancelCostDigitPresentationReadiness();
	if (PresentationController->DataRewritePlayback)
	{
		PresentationController->DataRewritePlayback->Reset();
	}
	PresentationController->State.PendingDataRewriteFieldMask = 0;
	PresentationController->State.PendingDataRewriteTone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	PresentationController->State.PendingDataRewriteSeed = 0;
	PresentationController->State.PendingDataRewriteSequenceIndex = 0;
	PresentationController->State.PendingDataRewriteSequenceCount = 1;
	PresentationController->State.bPendingDataRewriteHandoff = false;
	PresentationController->State.bDataRewriteBlocksPresentationPhase = false;
	if (CardView)
	{
		CardView->ResetCardDataRewriteView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyCardDataRewriteView()
{
	if (!CardView || !PresentationController->DataRewritePlayback || !PresentationController->DataRewritePlayback->IsActive())
	{
		return;
	}
	CardView->SetCardDataRewriteView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildDataRewriteView(
		PresentationController->DataRewritePlayback->BuildView(),
		SlotVisualConfig));
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingCardDataRewriteSound()
{
	if (!PresentationController->DataRewritePlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardDataRewriteSoundRequest> PendingRequest =
		PresentationController->DataRewritePlayback->ConsumePendingSoundRequest();
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
	return PresentationController->DataRewritePlayback && PresentationController->DataRewritePlayback->IsActive();
}

void UWacomFirstPersonCardLayerSlotWidget::TickEffectBadgeFeedbackPlayback(float DeltaTime)
{
	if (!PresentationController->EffectBadgeFeedbackPlayback || !PresentationController->EffectBadgeFeedbackPlayback->IsActive())
	{
		return;
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveEffectBadgePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardEffectBadgeFeedbackView View =
		PresentationController->EffectBadgeFeedbackPlayback->Tick(PlaybackDeltaTime);
	PlayPendingEffectBadgeFeedbackSound();
	if (View.bCompleted || !PresentationController->EffectBadgeFeedbackPlayback->IsActive())
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
	if (PresentationController->EffectBadgeFeedbackPlayback)
	{
		PresentationController->EffectBadgeFeedbackPlayback->Reset();
	}
	PresentationController->State.bEffectBadgeFeedbackBlocksPresentationPhase = false;
	if (CardView)
	{
		CardView->ResetEffectBadgeFeedbackView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyEffectBadgeFeedbackView()
{
	if (!CardView || !PresentationController->EffectBadgeFeedbackPlayback || !PresentationController->EffectBadgeFeedbackPlayback->IsActive())
	{
		return;
	}
	CardView->SetEffectBadgeFeedbackConfig(SlotVisualConfig.EffectBadgeFeedback);
	CardView->SetEffectBadgeFeedbackView(PresentationController->EffectBadgeFeedbackPlayback->BuildView());
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingEffectBadgeFeedbackSound()
{
	if (!PresentationController->EffectBadgeFeedbackPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest> PendingRequest =
		PresentationController->EffectBadgeFeedbackPlayback->ConsumePendingSoundRequest();
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
	return PresentationController->EffectBadgeFeedbackPlayback && PresentationController->EffectBadgeFeedbackPlayback->IsActive();
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
	if (!PresentationController->DrawRevealPlayback)
	{
		PresentationController->DrawRevealPlayback.Reset(new FWacomFirstPersonCardDrawRevealPlayback());
	}
	PresentationController->DrawRevealPlayback->Prepare(SlotVisualConfig.DrawReveal);
	ApplyDrawRevealSurfaceView();
	BeginSurfacePresentationReadiness(SurfaceEffectDrawReveal);
}

void UWacomFirstPersonCardLayerSlotWidget::StartDrawRevealPlayback()
{
	if (!PresentationController->DrawRevealPlayback)
	{
		return;
	}
	PresentationController->DrawRevealPlayback->Start();
	ApplyDrawRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateDrawRevealPlayback(
	float NormalizedEnterProgress)
{
	if (!PresentationController->DrawRevealPlayback || !PresentationController->DrawRevealPlayback->IsActive())
	{
		return;
	}
	PresentationController->DrawRevealPlayback->Update(NormalizedEnterProgress);
	ApplyDrawRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyDrawRevealSurfaceView()
{
	if (!CardView || !PresentationController->DrawRevealPlayback || !PresentationController->DrawRevealPlayback->IsActive())
	{
		return;
	}
	CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildDrawRevealView(
		PresentationController->DrawRevealPlayback->BuildView(),
		SlotVisualConfig));
}

void UWacomFirstPersonCardLayerSlotWidget::ClearDrawRevealPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectDrawReveal);
	if (PresentationController->DrawRevealPlayback)
	{
		PresentationController->DrawRevealPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsDrawRevealPlaybackActive() const
{
	return PresentationController->DrawRevealPlayback && PresentationController->DrawRevealPlayback->IsActive();
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
	if (!PresentationController->GainRevealPlayback)
	{
		PresentationController->GainRevealPlayback.Reset(new FWacomFirstPersonCardGainRevealPlayback());
	}
	PresentationController->GainRevealPlayback->Prepare(SlotVisualConfig.GainReveal);
	ApplyGainRevealSurfaceView();
	BeginSurfacePresentationReadiness(SurfaceEffectGainReveal);
}

void UWacomFirstPersonCardLayerSlotWidget::StartGainRevealPlayback()
{
	if (!PresentationController->GainRevealPlayback)
	{
		return;
	}
	PresentationController->GainRevealPlayback->Start();
	ApplyGainRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::UpdateGainRevealPlayback(
	float NormalizedEnterProgress)
{
	if (!PresentationController->GainRevealPlayback || !PresentationController->GainRevealPlayback->IsActive())
	{
		return;
	}
	PresentationController->GainRevealPlayback->Update(NormalizedEnterProgress);
	ApplyGainRevealSurfaceView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyGainRevealSurfaceView()
{
	if (!CardView || !PresentationController->GainRevealPlayback || !PresentationController->GainRevealPlayback->IsActive())
	{
		return;
	}
	CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildGainRevealView(
		PresentationController->GainRevealPlayback->BuildView(),
		SlotVisualConfig,
		CurrentSlotView));
}

void UWacomFirstPersonCardLayerSlotWidget::ClearGainRevealPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectGainReveal);
	if (PresentationController->GainRevealPlayback)
	{
		PresentationController->GainRevealPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsGainRevealPlaybackActive() const
{
	return PresentationController->GainRevealPlayback && PresentationController->GainRevealPlayback->IsActive();
}

void UWacomFirstPersonCardLayerSlotWidget::TickRetainSealPlayback(float DeltaTime)
{
	if (!PresentationController->RetainSealPlayback || !PresentationController->RetainSealPlayback->IsActive())
	{
		return;
	}
	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	PresentationController->RetainSealPlayback->Tick(PlaybackDeltaTime);
	if (!PresentationController->RetainSealPlayback->IsActive())
	{
		CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectRetainSeal);
	}
	ApplyActiveSurfaceEffectView();
	ApplyVisualSlotView();
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyRetainSealSurfaceView()
{
	if (!CardView || !PresentationController->RetainSealPlayback || !PresentationController->RetainSealPlayback->IsActive())
	{
		return;
	}
	if (PresentationController->State.bSuppressRetainSealSurfaceForReadinessFailure)
	{
		ResetCardSurfaceEffectView();
		return;
	}
	CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildRetainSealView(
		PresentationController->RetainSealPlayback->BuildView()));
}

void UWacomFirstPersonCardLayerSlotWidget::ClearRetainSealPlayback()
{
	CancelSurfacePresentationReadinessIfOwnedBy(SurfaceEffectRetainSeal);
	PresentationController->State.bSuppressRetainSealSurfaceForReadinessFailure = false;
	if (PresentationController->RetainSealPlayback)
	{
		PresentationController->RetainSealPlayback->Reset();
	}
	ApplyActiveSurfaceEffectView();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsRetainSealPlaybackActive() const
{
	return PresentationController->RetainSealPlayback && PresentationController->RetainSealPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsRetainSealPlaybackBlockingPresentation() const
{
	return PresentationController->RetainSealPlayback && PresentationController->RetainSealPlayback->IsBlockingPresentation();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingHandTargetImpactSound()
{
	if (!PresentationController->HandTargetImpactPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardHandTargetImpactSoundRequest> PendingRequest =
		PresentationController->HandTargetImpactPlayback->ConsumePendingSoundRequest();
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
	return PresentationController->HandTargetImpactPlayback && PresentationController->HandTargetImpactPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsHandTargetImpactCommitPlaybackActive() const
{
	return PresentationController->HandTargetImpactPlayback && PresentationController->HandTargetImpactPlayback->IsCommitActive();
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
	if (!PresentationController->SurfaceDeparturePlayback)
	{
		PresentationController->SurfaceDeparturePlayback.Reset(new FWacomFirstPersonCardSurfaceDeparturePlayback());
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

	PresentationController->SurfaceDeparturePlayback->Begin(PlaybackConfig);
	if (!PresentationController->SurfaceDeparturePlayback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardSurfaceDepartureTickResult InitialView =
		PresentationController->SurfaceDeparturePlayback->BuildView();
	CardUseFlipProgress = InitialView.FlipProgress;
	CardUseImpactProgress = InitialView.ImpactProgress;
	CardUseMotionAlpha = InitialView.MotionAlpha;
	CardUseOpacityMultiplier = 1.0f;

	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildSurfaceDepartureView(
			PresentationController->SurfaceDeparturePlayback->BuildView(),
			SlotVisualConfig));
	}
	BeginSurfacePresentationReadiness(SurfaceEffectDeparture);
}

void UWacomFirstPersonCardLayerSlotWidget::TickSurfaceDeparturePlayback(float DeltaTime)
{
	if (!PresentationController->SurfaceDeparturePlayback || !PresentationController->SurfaceDeparturePlayback->IsActive())
	{
		return;
	}

	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardSurfaceDepartureTickResult TickResult =
		PresentationController->SurfaceDeparturePlayback->Tick(PlaybackDeltaTime);
	PlayPendingSurfaceDepartureSound();
	CardUseFlipProgress = TickResult.FlipProgress;
	CardUseImpactProgress = TickResult.ImpactProgress;
	CardUseMotionAlpha = TickResult.MotionAlpha;
	CardUseOpacityMultiplier = 1.0f;
	if (CardView)
	{
		CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildSurfaceDepartureView(TickResult, SlotVisualConfig));
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
	if (PresentationController->SurfaceDeparturePlayback)
	{
		PresentationController->SurfaceDeparturePlayback->Reset();
	}
	PresentationController->State.SurfaceDepartureTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	CardUseFlipProgress = 0.0f;
	CardUseImpactProgress = 0.0f;
	CardUseMotionAlpha = 0.0f;
	CardUseOpacityMultiplier = 1.0f;
	ApplyActiveSurfaceEffectView();
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingSurfaceDepartureSound()
{
	if (!PresentationController->SurfaceDeparturePlayback)
	{
		return;
	}
	const EWacomFirstPersonCardSurfaceDepartureKind Kind = PresentationController->SurfaceDeparturePlayback->GetKind();
	const TOptional<FWacomFirstPersonCardSurfaceDepartureSoundRequest> PendingRequest =
		PresentationController->SurfaceDeparturePlayback->ConsumePendingSoundRequest();
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
	return PresentationController->SurfaceDeparturePlayback && PresentationController->SurfaceDeparturePlayback->IsActive();
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
	if (!PresentationController->CardUseReformPlayback)
	{
		PresentationController->CardUseReformPlayback.Reset(new FWacomFirstPersonCardUseReformPlayback());
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
		PresentationController->CardUseReformPlayback->BeginOutbound(PlaybackConfig);
	}
	else if (bInboundOnly)
	{
		PresentationController->CardUseReformPlayback->BeginInbound(PlaybackConfig);
	}
	else
	{
		PresentationController->CardUseReformPlayback->Begin(PlaybackConfig);
	}
	if (!PresentationController->CardUseReformPlayback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardUseReformTickResult InitialView =
		PresentationController->CardUseReformPlayback->BuildView();
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
		CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildCardUseReformView(
			PresentationController->CardUseReformPlayback->BuildView(),
			SlotVisualConfig));
	}
	BeginSurfacePresentationReadiness(SurfaceEffectCardUseReform);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TickCardUseReformPlayback(float DeltaTime)
{
	if (!PresentationController->CardUseReformPlayback || !PresentationController->CardUseReformPlayback->IsActive())
	{
		return;
	}

	float PlaybackDeltaTime = 0.0f;
	if (!ResolveSurfacePresentationReadiness(DeltaTime, PlaybackDeltaTime))
	{
		return;
	}
	const FWacomFirstPersonCardUseReformTickResult TickResult =
		PresentationController->CardUseReformPlayback->Tick(PlaybackDeltaTime);
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
		CardView->SetCardSurfaceEffectView(WacomFirstPersonCardSurfaceEffectViewBuilder::BuildCardUseReformView(
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
	if (PresentationController->CardUseReformPlayback)
	{
		PresentationController->CardUseReformPlayback->Reset();
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
	if (!PresentationController->CardUseReformPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardUseReformSoundRequest> PendingRequest =
		PresentationController->CardUseReformPlayback->ConsumePendingSoundRequest();
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
	return PresentationController->CardUseReformPlayback && PresentationController->CardUseReformPlayback->IsActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsCardUseReformPlaybackBlockingStage() const
{
	return PresentationController->CardUseReformPlayback && PresentationController->CardUseReformPlayback->IsBlockingStage();
}

void UWacomFirstPersonCardLayerSlotWidget::BeginSurfacePresentationReadiness(
	FName EffectName,
	bool bReuseReadyGeneration,
	bool bBlocksPresentationPhase)
{
	if (!CardView || !PresentationController)
	{
		RecordPresentationReadinessFailure(TEXT("Surface"), EffectName, false);
		HandleSurfacePresentationReadinessFailure();
		return;
	}
	const uint32 Generation =
		CardView->BeginSurfacePresentationPreparation(bReuseReadyGeneration);
	if (Generation == 0)
	{
		PresentationController->Readiness.Begin(
			SurfaceReadinessChannel,
			Generation,
			EffectName,
			bBlocksPresentationPhase,
			false);
		RecordPresentationReadinessFailure(TEXT("Surface"), EffectName, false);
		HandleSurfacePresentationReadinessFailure();
		return;
	}
	PresentationController->Readiness.Begin(
		SurfaceReadinessChannel,
		Generation,
		EffectName,
		bBlocksPresentationPhase,
		CardView->IsSurfacePresentationMaterialReady(Generation)
			&& CardView->IsSurfacePresentationPainted(Generation));
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::BeginCostDigitPresentationReadiness()
{
	if (!CardView || !PresentationController)
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
	PresentationController->Readiness.Begin(
		CostDigitReadinessChannel,
		Generation,
		TEXT("DataRewrite"),
		true,
		CardView->IsCostDigitPresentationMaterialReady(Generation)
			&& CardView->IsCostDigitPresentationPainted(Generation));
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::BeginEffectBadgePresentationReadiness()
{
	if (!CardView || !PresentationController)
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
	PresentationController->Readiness.Begin(
		EffectBadgeReadinessChannel,
		Generation,
		TEXT("EffectBadgeRewrite"),
		true,
		CardView->IsEffectBadgePresentationMaterialReady(Generation)
			&& CardView->IsEffectBadgePresentationPainted(Generation));
	RefreshPresentationReadinessFrozenFlag();
}

bool UWacomFirstPersonCardLayerSlotWidget::ResolveSurfacePresentationReadiness(
	float DeltaTime,
	float& OutPlaybackDeltaTime)
{
	OutPlaybackDeltaTime = 0.0f;
	if (!PresentationController
		|| !PresentationController->Readiness.IsActive(SurfaceReadinessChannel))
	{
		OutPlaybackDeltaTime = DeltaTime;
		return true;
	}
	if (CardView)
	{
		CardView->RefreshSurfacePresentationPreparation(
			PresentationController->Readiness.GetGeneration(SurfaceReadinessChannel));
	}
	const uint32 Generation =
		PresentationController->Readiness.GetGeneration(SurfaceReadinessChannel);
	const EWacomFirstPersonCardPresentationReadinessPollResult Result =
		PresentationController->Readiness.Poll(
			SurfaceReadinessChannel,
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
		RecordPresentationReadinessFailure(
			TEXT("Surface"),
			PresentationController->Readiness.GetEffectName(SurfaceReadinessChannel),
			true);
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
	if (!PresentationController
		|| !PresentationController->Readiness.IsActive(CostDigitReadinessChannel))
	{
		OutPlaybackDeltaTime = DeltaTime;
		return true;
	}
	if (CardView)
	{
		CardView->RefreshCostDigitPresentationPreparation(
			PresentationController->Readiness.GetGeneration(CostDigitReadinessChannel));
	}
	const uint32 Generation =
		PresentationController->Readiness.GetGeneration(CostDigitReadinessChannel);
	const EWacomFirstPersonCardPresentationReadinessPollResult Result =
		PresentationController->Readiness.Poll(
			CostDigitReadinessChannel,
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
	if (!PresentationController
		|| !PresentationController->Readiness.IsActive(EffectBadgeReadinessChannel))
	{
		OutPlaybackDeltaTime = DeltaTime;
		return true;
	}
	if (CardView)
	{
		CardView->RefreshEffectBadgePresentationPreparation(
			PresentationController->Readiness.GetGeneration(EffectBadgeReadinessChannel));
	}
	const uint32 Generation =
		PresentationController->Readiness.GetGeneration(EffectBadgeReadinessChannel);
	const EWacomFirstPersonCardPresentationReadinessPollResult Result =
		PresentationController->Readiness.Poll(
			EffectBadgeReadinessChannel,
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
	if (PresentationController)
	{
		PresentationController->Readiness.Reset(SurfaceReadinessChannel);
	}
	if (CardView)
	{
		CardView->CancelSurfacePresentationPreparation();
	}
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::CancelSurfacePresentationReadinessIfOwnedBy(
	FName EffectName)
{
	if (PresentationController
		&& PresentationController->Readiness.IsOwnedBy(SurfaceReadinessChannel, EffectName))
	{
		CancelSurfacePresentationReadiness();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::CancelCostDigitPresentationReadiness()
{
	if (PresentationController)
	{
		PresentationController->Readiness.Reset(CostDigitReadinessChannel);
	}
	if (CardView)
	{
		CardView->CancelCostDigitPresentationPreparation();
	}
	RefreshPresentationReadinessFrozenFlag();
}

void UWacomFirstPersonCardLayerSlotWidget::CancelEffectBadgePresentationReadiness()
{
	if (PresentationController)
	{
		PresentationController->Readiness.Reset(EffectBadgeReadinessChannel);
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
	if (PresentationController)
	{
		PresentationController->Readiness.ResetAll();
	}
	if (CardView)
	{
		CardView->CancelAllPresentationPreparations();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::RefreshPresentationReadinessFrozenFlag()
{
	PresentationController->State.bPlaybackFrozenForReadiness = PresentationController
		&& PresentationController->Readiness.IsAnyPending();
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
	const FName FailedEffect = PresentationController
		? PresentationController->Readiness.GetEffectName(SurfaceReadinessChannel)
		: NAME_None;
	CancelSurfacePresentationReadiness();
	if (FailedEffect == SurfaceEffectDrawReveal)
	{
		ClearDrawRevealPlayback();
		if (PresentationController->TransitionPlayback)
		{
			PresentationController->TransitionPlayback->ConsumePendingSoundRequest();
		}
	}
	else if (FailedEffect == SurfaceEffectGainReveal)
	{
		ClearGainRevealPlayback();
		if (PresentationController->TransitionPlayback)
		{
			PresentationController->TransitionPlayback->ConsumePendingSoundRequest();
		}
	}
	else if (FailedEffect == SurfaceEffectHandTargetImpact)
	{
		if (PresentationController->HandTargetImpactPlayback)
		{
			PresentationController->HandTargetImpactPlayback->Reset();
		}
		HandTargetImpactScaleMultiplier = 1.0f;
		HandTargetImpactTranslationYPixels = 0.0f;
		HandTargetImpactZOrderBoost = 0;
		PresentationController->State.bHandTargetImpactDepartureGateReleased = true;
		ApplyActiveSurfaceEffectView();
		ApplyVisualSlotView();
	}
	else if (FailedEffect == SurfaceEffectDeparture)
	{
		ClearSurfaceDeparturePlayback();
		PresentationController->State.bUsesSurfaceDepartureExit = false;
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
		PresentationController->State.bSuppressRetainSealSurfaceForReadinessFailure = true;
		ResetCardSurfaceEffectView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ApplyActiveSurfaceEffectView()
{
	if (!CardView || !PresentationController)
	{
		return;
	}

	FWacomFirstPersonCardSurfaceEffectClaims Claims;
	Claims.bDeparture = IsSurfaceDeparturePlaybackActive();
	Claims.bCardUseReform = IsCardUseReformPlaybackActive();
	Claims.bHandTargetImpact = IsHandTargetImpactPlaybackActive();
	Claims.bDrawReveal = IsDrawRevealPlaybackActive();
	Claims.bGainReveal = IsGainRevealPlaybackActive();
	Claims.bRetainSeal = IsRetainSealPlaybackActive();

	switch (PresentationController->SurfaceArbiter.Resolve(Claims))
	{
	case EWacomFirstPersonCardSurfaceEffectOwner::Departure:
	case EWacomFirstPersonCardSurfaceEffectOwner::CardUseReform:
		// These playbacks write their view while ticking; the arbiter prevents
		// a lower-priority owner from replacing it between ticks.
		break;
	case EWacomFirstPersonCardSurfaceEffectOwner::HandTargetImpact:
		ApplyHandTargetImpactSurfaceView();
		break;
	case EWacomFirstPersonCardSurfaceEffectOwner::DrawReveal:
		ApplyDrawRevealSurfaceView();
		break;
	case EWacomFirstPersonCardSurfaceEffectOwner::GainReveal:
		ApplyGainRevealSurfaceView();
		break;
	case EWacomFirstPersonCardSurfaceEffectOwner::RetainSeal:
		ApplyRetainSealSurfaceView();
		break;
	case EWacomFirstPersonCardSurfaceEffectOwner::Base:
	default:
		CardView->SetEffectBadgeFeedbackConfig(SlotVisualConfig.EffectBadgeFeedback);
		ResetCardSurfaceEffectView();
		break;
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::IsDenyFeedbackActive() const
{
	return InteractionFeedbackPlayback
		&& InteractionFeedbackPlayback->BuildView().bDenyActive;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsRetainedFeedbackActive() const
{
	return IsRetainSealPlaybackActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::HasActivePresentationPlayback() const
{
	if (!PresentationController)
	{
		return false;
	}
	FWacomFirstPersonCardSlotPresentationActivityInput Input;
	Input.bSlotExiting = bIsExitingForFirstPersonLayer;
	return PresentationController->BuildActivityView(Input).bBlocksPresentation;
}

void UWacomFirstPersonCardLayerSlotWidget::ForceCompletePresentationPlayback()
{
	CancelAllPresentationReadiness();
	ClearEnterTransitionPlayback();
	if (IsCardUseReformPlaybackActive())
	{
		ClearCardUseReformPlayback(true);
	}
	if (IsHandTargetImpactPlaybackActive() || PresentationController->State.bHandTargetImpactDeparturePending)
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
	if (bIsExitingForFirstPersonLayer)
	{
		VisualSlotView = TargetSlotView;
		VisualSlotView.bProjected = false;
		ApplyVisualSlotView();
		ClearExitTransitionPlayback();
		ClearSurfaceDeparturePlayback();
		bUsesFixedExitTransitionPlayback = false;
		PresentationController->State.bUsesSurfaceDepartureExit = false;
		PresentationController->State.SurfaceDepartureTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
		ExitMotionElapsedSeconds = FMath::Max(0.0f, SlotMotionConfig.ExitDuration);
	}
	else if (bHasVisualSlotView)
	{
		VisualSlotView = GetEffectiveTargetSlotView();
		ApplyVisualSlotView();
	}
	ClearRetainSealPlayback();
	if (PresentationController)
	{
		PresentationController->Readiness.ResetAll();
		PresentationController->SurfaceArbiter.Reset();
	}
	ApplyInteractionCue();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::StartEnterTransitionPlayback(
	const FWacomFirstPersonCardLayerSlotView& StartSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& EnterProfile)
{
	if (!PresentationController->TransitionPlayback)
	{
		PresentationController->TransitionPlayback.Reset(new FWacomFirstPersonCardTransitionPlayback());
	}
	PresentationController->State.EnterTransitionStartWidgetPosition = StartSlotView.WidgetPosition;
	PresentationController->TransitionPlayback->BeginEnter(StartSlotView, EnterProfile);
	if (PresentationController->TransitionPlayback->IsEnterActive())
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

void UWacomFirstPersonCardLayerSlotWidget::ClearEnterTransitionPlayback()
{
	if (PresentationController->TransitionPlayback)
	{
		PresentationController->TransitionPlayback->ResetIfMode(EWacomFirstPersonCardTransitionPlaybackMode::Enter);
	}
	ClearDrawRevealPlayback();
	ClearGainRevealPlayback();
	PresentationController->State.EnterTransitionStartWidgetPosition = FVector2D::ZeroVector;
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
		PresentationController->TransitionPlayback->Tick(PlaybackDeltaTime, GetEffectiveTargetSlotView());
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
	if (!PresentationController->TransitionPlayback)
	{
		return;
	}
	const TOptional<EWacomFirstPersonCardSlotTransitionKind> PendingRequest =
		PresentationController->TransitionPlayback->ConsumePendingStartRequest();
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
	View.StartWidgetPosition = PresentationController->State.EnterTransitionStartWidgetPosition;
	View.TargetWidgetPosition = GetEffectiveTargetSlotView().WidgetPosition;
	OnEnterTransitionStartedNative.Broadcast(View);
}

void UWacomFirstPersonCardLayerSlotWidget::PlayPendingTransitionStartSound()
{
	if (!PresentationController->TransitionPlayback)
	{
		return;
	}
	const TOptional<FWacomFirstPersonCardTransitionSoundRequest> PendingRequest =
		PresentationController->TransitionPlayback->ConsumePendingSoundRequest();
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
	return PresentationController->TransitionPlayback && PresentationController->TransitionPlayback->IsEnterActive();
}

bool UWacomFirstPersonCardLayerSlotWidget::IsEnterTransitionBlockingInteraction() const
{
	return PresentationController->TransitionPlayback && PresentationController->TransitionPlayback->BlocksInteraction();
}

void UWacomFirstPersonCardLayerSlotWidget::StartExitTransitionPlayback(
	const FWacomFirstPersonCardLayerSlotView& StartSlotView,
	const FWacomFirstPersonCardLayerSlotView& InTargetSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& ExitProfile)
{
	if (!PresentationController->TransitionPlayback)
	{
		PresentationController->TransitionPlayback.Reset(new FWacomFirstPersonCardTransitionPlayback());
	}
	PresentationController->TransitionPlayback->BeginExit(StartSlotView, InTargetSlotView, ExitProfile);
	if (!PresentationController->TransitionPlayback->IsExitActive())
	{
		VisualSlotView = InTargetSlotView;
		ApplyVisualSlotView();
	}
}

void UWacomFirstPersonCardLayerSlotWidget::ClearExitTransitionPlayback()
{
	if (PresentationController->TransitionPlayback)
	{
		PresentationController->TransitionPlayback->ResetIfMode(EWacomFirstPersonCardTransitionPlaybackMode::Exit);
	}
}

bool UWacomFirstPersonCardLayerSlotWidget::TickExitTransitionPlayback(float DeltaTime)
{
	if (!IsExitTransitionPlaybackActive())
	{
		return true;
	}
	const FWacomFirstPersonCardTransitionTickResult Result =
		PresentationController->TransitionPlayback->Tick(DeltaTime, GetEffectiveTargetSlotView());
	ExitMotionElapsedSeconds = PresentationController->TransitionPlayback->GetElapsedSeconds();
	if (Result.bHasVisualSlotView)
	{
		VisualSlotView = Result.VisualSlotView;
		ApplyVisualSlotView();
	}
	return Result.bCompleted;
}

bool UWacomFirstPersonCardLayerSlotWidget::IsExitTransitionPlaybackActive() const
{
	return PresentationController->TransitionPlayback && PresentationController->TransitionPlayback->IsExitActive();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerRetainedFeedback(
	int32 SequenceIndex,
	int32 SequenceCount,
	bool bRetainUntilExplicitRelease)
{
	if (!SlotVisualConfig.RetainSeal.bEnabled
		|| SlotVisualConfig.RetainSeal.SealingDurationSeconds <= 0.0f)
	{
		return;
	}
	const int32 SafeSequenceIndex = FMath::Clamp(
		SequenceIndex,
		0,
		FMath::Max(0, SequenceCount - 1));
	if (!PresentationController->RetainSealPlayback)
	{
		PresentationController->RetainSealPlayback.Reset(new FWacomFirstPersonCardRetainSealPlayback());
	}
	FWacomFirstPersonCardRetainSealPlaybackConfig Config;
	Config.bEnabled = true;
	Config.bReducedMotion = SlotVisualConfig.RetainSeal.bReducedMotion;
	Config.bRetainUntilExplicitRelease = bRetainUntilExplicitRelease;
	Config.StartDelaySeconds =
		static_cast<float>(SafeSequenceIndex) * SlotVisualConfig.RetainSeal.SequenceStaggerSeconds;
	Config.SealingDurationSeconds = SlotVisualConfig.RetainSeal.SealingDurationSeconds;
	Config.ReleaseDurationSeconds = SlotVisualConfig.RetainSeal.ReleaseDurationSeconds;
	Config.PeakLiftPixels = SlotVisualConfig.RetainSeal.PeakLiftPixels;
	Config.PeakScale = SlotVisualConfig.RetainSeal.PeakScale;
	Config.HeldLiftPixels = SlotVisualConfig.RetainSeal.HeldLiftPixels;
	Config.HeldScale = SlotVisualConfig.RetainSeal.HeldScale;
	Config.Seed = static_cast<float>(GetTypeHash(CurrentSlotView.Entry.CardInstanceId) & 0xFFFFu)
		/ 65535.0f;
	Config.Style = SlotVisualConfig.RetainSeal.Style;
	PresentationController->RetainSealPlayback->Begin(Config);
	PresentationController->State.bSuppressRetainSealSurfaceForReadinessFailure = false;
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectRetainSeal);
	ApplyVisualSlotView();
	UpdateWantsTick();
}

void UWacomFirstPersonCardLayerSlotWidget::TriggerRetainedReleaseFeedback()
{
	if (!PresentationController->RetainSealPlayback || !PresentationController->RetainSealPlayback->IsActive())
	{
		return;
	}
	PresentationController->RetainSealPlayback->Release();
	PresentationController->State.bSuppressRetainSealSurfaceForReadinessFailure = false;
	ApplyActiveSurfaceEffectView();
	BeginSurfacePresentationReadiness(SurfaceEffectRetainSeal);
	ApplyVisualSlotView();
	UpdateWantsTick();
}
