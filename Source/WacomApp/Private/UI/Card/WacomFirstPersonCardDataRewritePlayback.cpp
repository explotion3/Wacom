// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDataRewritePlayback.h"

#include "Sound/SoundBase.h"

namespace
{
	float EaseOutCubicDataRewrite(float Alpha)
	{
		const float Inverse = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - Inverse * Inverse * Inverse;
	}

	float EaseInCubicDataRewrite(float Alpha)
	{
		const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return SafeAlpha * SafeAlpha * SafeAlpha;
	}
}

void FWacomFirstPersonCardDataRewritePlayback::BeginOrRetarget(
	const FWacomFirstPersonCardDataRewriteConfig& InConfig,
	int32 InFieldMask,
	EWacomFirstPersonCardDataRewriteTone InTone,
	uint32 InSeed,
	int32 SequenceIndex,
	bool bAllowSequenceDelay)
{
	if (!InConfig.bEnabled
		|| !InConfig.Style.DigitRewriteMaterialInstance
		|| InFieldMask == 0)
	{
		Reset();
		return;
	}

	const float ExistingProgress = IsActive() ? BuildView().Progress : 0.0f;
	Config = InConfig;
	FieldMask |= InFieldMask;
	Tone = InTone;
	Seed = InSeed;
	RestartProgress = ExistingProgress;
	ElapsedSeconds = 0.0f;
	DelaySeconds = bAllowSequenceDelay
		? FMath::Min(
			FMath::Max(0, SequenceIndex)
				* FMath::Max(0.0f, Config.Style.SequenceStaggerSeconds),
			FMath::Max(0.0f, Config.Style.MaxSequenceDelaySeconds))
		: 0.0f;
	Phase = DelaySeconds > 0.0f
		? EWacomFirstPersonCardDataRewritePhase::Delay
		: EWacomFirstPersonCardDataRewritePhase::Playing;
	bBatchSoundOwner = SequenceIndex == 0;
	bSoundRequested = false;
	PendingSoundRequest.Reset();
}

FWacomFirstPersonCardDataRewritePlaybackView
FWacomFirstPersonCardDataRewritePlayback::Tick(float DeltaTime)
{
	if (!IsActive())
	{
		return BuildView();
	}

	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	ElapsedSeconds += SafeDeltaTime;
	if (Phase == EWacomFirstPersonCardDataRewritePhase::Delay)
	{
		if (ElapsedSeconds < DelaySeconds)
		{
			return BuildView();
		}
		ElapsedSeconds = FMath::Max(0.0f, ElapsedSeconds - DelaySeconds);
		Phase = EWacomFirstPersonCardDataRewritePhase::Playing;
	}

	const float Duration = ResolveDurationSeconds();
	const float RevealStart = Config.bReducedMotion
		? 0.0f
		: FMath::Clamp(Config.Style.NewRevealStartSeconds, 0.0f, Duration);
	if (ElapsedSeconds >= RevealStart)
	{
		QueueSoundIfNeeded();
	}
	if (ElapsedSeconds >= Duration)
	{
		FWacomFirstPersonCardDataRewritePlaybackView Result = BuildView();
		Result.Progress = 1.0f;
		Result.bCompleted = true;
		Reset();
		return Result;
	}
	return BuildView();
}

FWacomFirstPersonCardDataRewritePlaybackView
FWacomFirstPersonCardDataRewritePlayback::BuildView() const
{
	FWacomFirstPersonCardDataRewritePlaybackView View;
	View.Phase = Phase;
	// A staggered card must stay on its base surface until its real start edge.
	// Treat Delay as an active playback only for ticking/lifecycle, not rendering.
	View.bActive = Phase == EWacomFirstPersonCardDataRewritePhase::Playing;
	View.bReducedMotion = Config.bReducedMotion;
	View.FieldMask = FieldMask;
	View.Tone = Tone;
	View.Seed = static_cast<float>(Seed & 0xFFFFu) / 65535.0f;
	if (Phase == EWacomFirstPersonCardDataRewritePhase::Playing)
	{
		const float Duration = ResolveDurationSeconds();
		float Normalized = FMath::Clamp(ElapsedSeconds / Duration, 0.0f, 1.0f);
		if (Config.bReducedMotion)
		{
			View.OldDissolveAmount = Normalized;
			View.NewRevealAmount = Normalized;
			View.DigitScale = 1.0f;
		}
		else
		{
			const float OldDissolveEnd = FMath::Clamp(
				Config.Style.OldDissolveEndSeconds,
				0.0f,
				Duration);
			const float NewRevealStart = FMath::Clamp(
				Config.Style.NewRevealStartSeconds,
				OldDissolveEnd,
				Duration);
			const float NewRevealEnd = FMath::Clamp(
				Config.Style.NewRevealEndSeconds,
				NewRevealStart,
				Duration);
			const float OvershootPeak = FMath::Clamp(
				Config.Style.OvershootPeakSeconds,
				NewRevealEnd,
				Duration);
			View.OldDissolveAmount = FMath::Clamp(
				ElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, OldDissolveEnd),
				0.0f,
				1.0f);
			View.NewRevealAmount = FMath::Clamp(
				(ElapsedSeconds - NewRevealStart)
					/ FMath::Max(KINDA_SMALL_NUMBER, NewRevealEnd - NewRevealStart),
				0.0f,
				1.0f);
			const float MinimumScale = FMath::Max(0.01f, Config.Style.MinimumScale);
			const float OvershootScale = FMath::Max(0.01f, Config.Style.OvershootScale);
			if (ElapsedSeconds < OldDissolveEnd)
			{
				View.DigitScale = FMath::Lerp(
					1.0f,
					MinimumScale,
					EaseInCubicDataRewrite(View.OldDissolveAmount));
			}
			else if (ElapsedSeconds < NewRevealStart)
			{
				View.DigitScale = MinimumScale;
			}
			else if (ElapsedSeconds < OvershootPeak)
			{
				View.DigitScale = FMath::Lerp(
					MinimumScale,
					OvershootScale,
					EaseOutCubicDataRewrite(
						(ElapsedSeconds - NewRevealStart)
							/ FMath::Max(KINDA_SMALL_NUMBER, OvershootPeak - NewRevealStart)));
			}
			else
			{
				View.DigitScale = FMath::Lerp(
					OvershootScale,
					1.0f,
					EaseOutCubicDataRewrite(
						(ElapsedSeconds - OvershootPeak)
							/ FMath::Max(KINDA_SMALL_NUMBER, Duration - OvershootPeak)));
			}
		}
		// On a rapid re-trigger, rebuild the cover from the current visual state
		// instead of flashing through a zero-progress frame.
		View.Progress = FMath::Lerp(
			FMath::Min(RestartProgress, 0.18f),
			1.0f,
			Normalized);
	}
	return View;
}

TOptional<FWacomFirstPersonCardDataRewriteSoundRequest>
FWacomFirstPersonCardDataRewritePlayback::ConsumePendingSoundRequest()
{
	TOptional<FWacomFirstPersonCardDataRewriteSoundRequest> Result = PendingSoundRequest;
	PendingSoundRequest.Reset();
	return Result;
}

void FWacomFirstPersonCardDataRewritePlayback::Reset()
{
	Config = FWacomFirstPersonCardDataRewriteConfig();
	Phase = EWacomFirstPersonCardDataRewritePhase::Inactive;
	DelaySeconds = 0.0f;
	ElapsedSeconds = 0.0f;
	RestartProgress = 0.0f;
	FieldMask = 0;
	Tone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	Seed = 0;
	bBatchSoundOwner = false;
	bSoundRequested = false;
	PendingSoundRequest.Reset();
}

float FWacomFirstPersonCardDataRewritePlayback::ResolveDurationSeconds() const
{
	return Config.bReducedMotion
		? 0.12f
		: FMath::Max(KINDA_SMALL_NUMBER, Config.Style.DurationSeconds);
}

void FWacomFirstPersonCardDataRewritePlayback::QueueSoundIfNeeded()
{
	if (bSoundRequested)
	{
		return;
	}
	bSoundRequested = true;
	if (!bBatchSoundOwner || !Config.Style.RewriteSound)
	{
		return;
	}

	const float Variation = FMath::Clamp(
		Config.Style.RewriteSoundPitchVariation,
		0.0f,
		0.99f);
	FRandomStream RandomStream(static_cast<int32>(Seed));
	FWacomFirstPersonCardDataRewriteSoundRequest Request;
	Request.Sound = Config.Style.RewriteSound;
	Request.VolumeMultiplier = FMath::Max(
		0.0f,
		Config.Style.RewriteSoundVolumeMultiplier);
	Request.PitchMultiplier = FMath::Max(
		0.01f,
		Config.Style.RewriteSoundPitchMultiplier
			* RandomStream.FRandRange(1.0f - Variation, 1.0f + Variation));
	PendingSoundRequest = Request;
}
