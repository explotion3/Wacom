// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardHandTargetImpactPlayback.h"

namespace
{
	float EaseOutCubicHandTargetImpact(float Alpha)
	{
		const float Inverse = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - Inverse * Inverse * Inverse;
	}
}

void FWacomFirstPersonCardHandTargetImpactPlayback::BeginPreview(
	const FWacomFirstPersonCardHandTargetImpactConfig& InConfig)
{
	if (!InConfig.bEnabled || !InConfig.Style.SurfaceEffectMaterialInstance)
	{
		Reset();
		return;
	}
	if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering
		|| Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain)
	{
		return;
	}
	const float ExistingAmount = BuildView().PreviewAmount;
	Config = InConfig;
	Phase = EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering;
	const float FadeIn = FMath::Max(KINDA_SMALL_NUMBER, Config.Style.PreviewFadeInSeconds);
	ElapsedSeconds = ExistingAmount * FadeIn;
	PreviewAmountAtExit = 0.0f;
	bDepartureGateOpen = false;
	bSoundRequested = false;
	PendingSoundRequest.Reset();
}

void FWacomFirstPersonCardHandTargetImpactPlayback::EndPreview()
{
	if (Phase != EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering
		&& Phase != EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain)
	{
		return;
	}
	PreviewAmountAtExit = BuildView().PreviewAmount;
	ElapsedSeconds = 0.0f;
	Phase = EWacomFirstPersonCardHandTargetImpactPhase::PreviewExiting;
}

void FWacomFirstPersonCardHandTargetImpactPlayback::BeginCommit(
	const FWacomFirstPersonCardHandTargetImpactConfig& InConfig,
	uint32 Seed)
{
	if (!InConfig.bEnabled || !InConfig.Style.SurfaceEffectMaterialInstance)
	{
		Reset();
		return;
	}
	Config = InConfig;
	Phase = EWacomFirstPersonCardHandTargetImpactPhase::Commit;
	ElapsedSeconds = 0.0f;
	PreviewAmountAtExit = 0.0f;
	bDepartureGateOpen = false;
	bSoundRequested = false;
	CommitSeed = Seed;
	PendingSoundRequest.Reset();
}

FWacomFirstPersonCardHandTargetImpactPlaybackView
FWacomFirstPersonCardHandTargetImpactPlayback::Tick(float DeltaTime)
{
	if (!IsActive())
	{
		return BuildView();
	}
	ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering
		&& ElapsedSeconds >= FMath::Max(0.0f, Config.Style.PreviewFadeInSeconds))
	{
		Phase = EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain;
		ElapsedSeconds = 0.0f;
	}
	else if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewExiting
		&& ElapsedSeconds >= 0.08f)
	{
		Reset();
		return BuildView();
	}
	else if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::Commit)
	{
		const float GateSeconds = Config.bReducedMotion
			? FMath::Min(0.04f, FMath::Max(0.0f, Config.Style.CommitDurationSeconds))
			: FMath::Max(0.0f, Config.Style.DepartureGateSeconds);
		if (!bDepartureGateOpen && ElapsedSeconds >= GateSeconds)
		{
			bDepartureGateOpen = true;
			QueueImpactSound();
		}
		const float Duration = Config.bReducedMotion
			? 0.12f
			: FMath::Max(KINDA_SMALL_NUMBER, Config.Style.CommitDurationSeconds);
		if (ElapsedSeconds >= Duration)
		{
			FWacomFirstPersonCardHandTargetImpactPlaybackView Result = BuildView();
			Result.bCompleted = true;
			Reset();
			return Result;
		}
	}
	return BuildView();
}

FWacomFirstPersonCardHandTargetImpactPlaybackView
FWacomFirstPersonCardHandTargetImpactPlayback::BuildView() const
{
	FWacomFirstPersonCardHandTargetImpactPlaybackView View;
	View.Phase = Phase;
	View.bReducedMotion = Config.bReducedMotion;
	View.bDepartureGateOpen = bDepartureGateOpen;
	View.TimeSeconds = ElapsedSeconds;
	if (!IsActive())
	{
		return View;
	}

	if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewEntering)
	{
		View.PreviewAmount = Config.bReducedMotion
			? 1.0f
			: FMath::Clamp(
				ElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, Config.Style.PreviewFadeInSeconds),
				0.0f,
				1.0f);
	}
	else if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewSustain)
	{
		const float Period = FMath::Max(0.01f, Config.Style.PreviewPeriodSeconds);
		// Sustain starts at the preview peak so entering the breathing loop does not
		// produce a one-frame brightness drop at the fade-in boundary.
		const float Breath = 0.5f + 0.5f * FMath::Cos((ElapsedSeconds / Period) * 2.0f * UE_PI);
		View.PreviewAmount = Config.bReducedMotion ? 1.0f : FMath::Lerp(0.72f, 1.0f, Breath);
	}
	else if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::PreviewExiting)
	{
		View.PreviewAmount = PreviewAmountAtExit
			* (1.0f - FMath::Clamp(ElapsedSeconds / 0.08f, 0.0f, 1.0f));
	}
	else if (Phase == EWacomFirstPersonCardHandTargetImpactPhase::Commit)
	{
		const float Duration = Config.bReducedMotion
			? 0.12f
			: FMath::Max(KINDA_SMALL_NUMBER, Config.Style.CommitDurationSeconds);
		View.CommitProgress = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
		View.ZOrderBoost = FMath::Max(0, Config.Style.ZOrderBoost);
		if (!Config.bReducedMotion)
		{
			const float Delay = FMath::Max(0.0f, Config.Style.CommitDelaySeconds);
			const float Gate = FMath::Max(Delay + KINDA_SMALL_NUMBER, Config.Style.DepartureGateSeconds);
			const float Rebound = FMath::Max(Gate + KINDA_SMALL_NUMBER, Config.Style.ReboundPeakSeconds);
			if (ElapsedSeconds < Delay)
			{
				View.ScaleMultiplier = 1.0f;
			}
			else if (ElapsedSeconds < Gate)
			{
				const float Alpha = FMath::InterpEaseIn(
					0.0f, 1.0f, (ElapsedSeconds - Delay) / (Gate - Delay), 2.2f);
				View.ScaleMultiplier = FMath::Lerp(1.0f, Config.Style.CompressionScale, Alpha);
				View.TranslationYPixels = Config.Style.CompressionTranslationPixels * Alpha;
			}
			else if (ElapsedSeconds < Rebound)
			{
				const float Alpha = EaseOutCubicHandTargetImpact((ElapsedSeconds - Gate) / (Rebound - Gate));
				View.ScaleMultiplier = FMath::Lerp(
					Config.Style.CompressionScale, Config.Style.ReboundScale, Alpha);
				View.TranslationYPixels = FMath::Lerp(
					Config.Style.CompressionTranslationPixels, -Config.Style.ReboundLiftPixels, Alpha);
			}
			else
			{
				const float Alpha = EaseOutCubicHandTargetImpact(
					(ElapsedSeconds - Rebound) / FMath::Max(KINDA_SMALL_NUMBER, Duration - Rebound));
				View.ScaleMultiplier = FMath::Lerp(Config.Style.ReboundScale, 1.0f, Alpha);
				View.TranslationYPixels = FMath::Lerp(-Config.Style.ReboundLiftPixels, 0.0f, Alpha);
			}
		}
	}
	return View;
}

TOptional<FWacomFirstPersonCardHandTargetImpactSoundRequest>
FWacomFirstPersonCardHandTargetImpactPlayback::ConsumePendingSoundRequest()
{
	TOptional<FWacomFirstPersonCardHandTargetImpactSoundRequest> Result = PendingSoundRequest;
	PendingSoundRequest.Reset();
	return Result;
}

void FWacomFirstPersonCardHandTargetImpactPlayback::Reset()
{
	Config = FWacomFirstPersonCardHandTargetImpactConfig();
	Phase = EWacomFirstPersonCardHandTargetImpactPhase::Inactive;
	ElapsedSeconds = 0.0f;
	PreviewAmountAtExit = 0.0f;
	bDepartureGateOpen = false;
	bSoundRequested = false;
	CommitSeed = 0;
	PendingSoundRequest.Reset();
}

void FWacomFirstPersonCardHandTargetImpactPlayback::QueueImpactSound()
{
	if (bSoundRequested)
	{
		return;
	}
	bSoundRequested = true;
	if (!Config.Style.ImpactSound)
	{
		return;
	}
	const float Variation = FMath::Clamp(Config.Style.ImpactSoundPitchVariation, 0.0f, 0.99f);
	FRandomStream RandomStream(static_cast<int32>(CommitSeed));
	FWacomFirstPersonCardHandTargetImpactSoundRequest Request;
	Request.Sound = Config.Style.ImpactSound;
	Request.VolumeMultiplier = FMath::Max(0.0f, Config.Style.ImpactSoundVolumeMultiplier);
	Request.PitchMultiplier = FMath::Max(
		0.01f,
		Config.Style.ImpactSoundPitchMultiplier * RandomStream.FRandRange(1.0f - Variation, 1.0f + Variation));
	PendingSoundRequest = Request;
}
