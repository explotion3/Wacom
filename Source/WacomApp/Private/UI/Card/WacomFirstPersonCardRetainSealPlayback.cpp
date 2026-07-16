// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardRetainSealPlayback.h"

namespace
{
	float Smooth(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}
}

void FWacomFirstPersonCardRetainSealPlayback::Begin(
	const FWacomFirstPersonCardRetainSealPlaybackConfig& InConfig)
{
	Reset();
	if (!InConfig.bEnabled || InConfig.SealingDurationSeconds <= 0.0f)
	{
		return;
	}
	Config = InConfig;
	Config.StartDelaySeconds = FMath::Max(0.0f, Config.StartDelaySeconds);
	Config.SealingDurationSeconds = FMath::Max(KINDA_SMALL_NUMBER, Config.SealingDurationSeconds);
	Config.ReleaseDurationSeconds = FMath::Max(0.0f, Config.ReleaseDurationSeconds);
	Config.PeakScale = FMath::Max(0.01f, Config.PeakScale);
	Config.HeldScale = FMath::Max(0.01f, Config.HeldScale);
	Phase = EWacomFirstPersonCardRetainSealPhase::Sealing;
	ElapsedSeconds = 0.0f;
}

void FWacomFirstPersonCardRetainSealPlayback::Release()
{
	if (Phase == EWacomFirstPersonCardRetainSealPhase::Inactive
		|| Phase == EWacomFirstPersonCardRetainSealPhase::Releasing)
	{
		return;
	}
	const FWacomFirstPersonCardRetainSealPlaybackView Current = BuildView();
	ReleaseStartLiftPixels = Current.LiftPixels;
	ReleaseStartScale = Current.ScaleMultiplier;
	ElapsedSeconds = 0.0f;
	if (Config.ReleaseDurationSeconds <= KINDA_SMALL_NUMBER)
	{
		Reset();
		return;
	}
	Phase = EWacomFirstPersonCardRetainSealPhase::Releasing;
}

FWacomFirstPersonCardRetainSealPlaybackView
FWacomFirstPersonCardRetainSealPlayback::Tick(float DeltaTime)
{
	if (!IsActive())
	{
		return BuildView();
	}
	ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (Phase == EWacomFirstPersonCardRetainSealPhase::Sealing)
	{
		const float SealingEndSeconds =
			Config.StartDelaySeconds + Config.SealingDurationSeconds;
		if (ElapsedSeconds >= SealingEndSeconds)
		{
			const float OvershootSeconds = ElapsedSeconds - SealingEndSeconds;
			EnterHeldOrRelease();
			if (Phase == EWacomFirstPersonCardRetainSealPhase::Releasing)
			{
				ElapsedSeconds = OvershootSeconds;
			}
		}
	}
	if (Phase == EWacomFirstPersonCardRetainSealPhase::Releasing
		&& ElapsedSeconds >= Config.ReleaseDurationSeconds)
	{
		Reset();
	}
	return BuildView();
}

FWacomFirstPersonCardRetainSealPlaybackView
FWacomFirstPersonCardRetainSealPlayback::BuildView() const
{
	FWacomFirstPersonCardRetainSealPlaybackView View;
	View.Phase = Phase;
	View.bReducedMotion = Config.bReducedMotion;
	View.bBlocksPresentation = IsBlockingPresentation();
	View.Seed = Config.Seed;
	View.Style = Config.Style;
	if (Phase == EWacomFirstPersonCardRetainSealPhase::Inactive)
	{
		return View;
	}
	if (Phase == EWacomFirstPersonCardRetainSealPhase::Held)
	{
		View.PhaseProgress = 1.0f;
		View.LiftPixels = Config.bReducedMotion ? 0.0f : Config.HeldLiftPixels;
		View.ScaleMultiplier = Config.bReducedMotion ? 1.0f : Config.HeldScale;
		return View;
	}
	if (Phase == EWacomFirstPersonCardRetainSealPhase::Releasing)
	{
		const float Alpha = Smooth(ElapsedSeconds / FMath::Max(Config.ReleaseDurationSeconds, KINDA_SMALL_NUMBER));
		View.PhaseProgress = Alpha;
		View.LiftPixels = Config.bReducedMotion ? 0.0f : FMath::Lerp(ReleaseStartLiftPixels, 0.0f, Alpha);
		View.ScaleMultiplier = Config.bReducedMotion ? 1.0f : FMath::Lerp(ReleaseStartScale, 1.0f, Alpha);
		return View;
	}

	const float PlaybackSeconds = ElapsedSeconds - Config.StartDelaySeconds;
	if (PlaybackSeconds <= 0.0f)
	{
		return View;
	}
	const float Duration = FMath::Max(Config.SealingDurationSeconds, KINDA_SMALL_NUMBER);
	const float Normalized = FMath::Clamp(PlaybackSeconds / Duration, 0.0f, 1.0f);
	View.PhaseProgress = Normalized;
	float MotionAlpha = 0.0f;
	if (Normalized < 0.21875f)
	{
		MotionAlpha = 0.35f * Smooth(Normalized / 0.21875f);
	}
	else if (Normalized < 0.46875f)
	{
		MotionAlpha = FMath::Lerp(0.35f, 1.0f, Smooth((Normalized - 0.21875f) / 0.25f));
	}
	else
	{
		const float SettleAlpha = Smooth((Normalized - 0.46875f) / 0.53125f);
		const float HeldLiftAlpha = Config.PeakLiftPixels > KINDA_SMALL_NUMBER
			? FMath::Clamp(Config.HeldLiftPixels / Config.PeakLiftPixels, 0.0f, 1.0f)
			: 0.0f;
		MotionAlpha = FMath::Lerp(1.0f, HeldLiftAlpha, SettleAlpha);
	}
	if (!Config.bReducedMotion)
	{
		View.LiftPixels = FMath::Lerp(0.0f, Config.PeakLiftPixels, MotionAlpha);
		if (Normalized < 0.46875f)
		{
			View.ScaleMultiplier = FMath::Lerp(1.0f, Config.PeakScale, MotionAlpha);
		}
		else
		{
			const float SettleAlpha = Smooth((Normalized - 0.46875f) / 0.53125f);
			View.ScaleMultiplier = FMath::Lerp(Config.PeakScale, Config.HeldScale, SettleAlpha);
		}
	}
	return View;
}

void FWacomFirstPersonCardRetainSealPlayback::Reset()
{
	Phase = EWacomFirstPersonCardRetainSealPhase::Inactive;
	Config = FWacomFirstPersonCardRetainSealPlaybackConfig();
	ElapsedSeconds = 0.0f;
	ReleaseStartLiftPixels = 0.0f;
	ReleaseStartScale = 1.0f;
}

void FWacomFirstPersonCardRetainSealPlayback::EnterHeldOrRelease()
{
	ElapsedSeconds = 0.0f;
	if (Config.bRetainUntilExplicitRelease)
	{
		Phase = EWacomFirstPersonCardRetainSealPhase::Held;
		return;
	}
	ReleaseStartLiftPixels = Config.bReducedMotion ? 0.0f : Config.HeldLiftPixels;
	ReleaseStartScale = Config.bReducedMotion ? 1.0f : Config.HeldScale;
	if (Config.ReleaseDurationSeconds <= KINDA_SMALL_NUMBER)
	{
		Reset();
		return;
	}
	Phase = EWacomFirstPersonCardRetainSealPhase::Releasing;
}
