// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDrawRevealPlayback.h"

namespace
{
	float SmoothRange(float Value, float Start, float End)
	{
		if (End <= Start + KINDA_SMALL_NUMBER)
		{
			return Value >= End ? 1.0f : 0.0f;
		}
		const float Alpha = FMath::Clamp((Value - Start) / (End - Start), 0.0f, 1.0f);
		return Alpha * Alpha * (3.0f - 2.0f * Alpha);
	}
}

void FWacomFirstPersonCardDrawRevealPlayback::Prepare(
	const FWacomFirstPersonCardDrawRevealConfig& InConfig)
{
	Reset();
	if (!InConfig.bEnabled || !InConfig.Style.SurfaceEffectMaterialInstance)
	{
		return;
	}
	Config = InConfig;
	Phase = EWacomFirstPersonCardDrawRevealPhase::Waiting;
}

void FWacomFirstPersonCardDrawRevealPlayback::Start()
{
	if (Phase == EWacomFirstPersonCardDrawRevealPhase::Waiting)
	{
		Phase = EWacomFirstPersonCardDrawRevealPhase::Playing;
		Progress = 0.0f;
	}
}

FWacomFirstPersonCardDrawRevealPlaybackView FWacomFirstPersonCardDrawRevealPlayback::Update(
	float NormalizedEnterProgress)
{
	if (Phase == EWacomFirstPersonCardDrawRevealPhase::Playing)
	{
		Progress = FMath::Clamp(NormalizedEnterProgress, 0.0f, 1.0f);
	}
	return BuildView();
}

FWacomFirstPersonCardDrawRevealPlaybackView FWacomFirstPersonCardDrawRevealPlayback::BuildView() const
{
	FWacomFirstPersonCardDrawRevealPlaybackView View;
	View.Phase = Phase;
	View.Progress = Progress;
	View.bReducedMotion = Config.bReducedMotion;
	if (!IsActive() || Config.bReducedMotion)
	{
		return View;
	}

	const FWacomFirstPersonCardDrawRevealStyleData& Style = Config.Style;
	const float MinimumScale = FMath::Clamp(Style.MinimumHorizontalScale, 0.01f, 1.0f);
	if (Progress < Style.FaceSwitchProgress)
	{
		const float CompressAlpha = SmoothRange(
			Progress,
			Style.BackHoldEndProgress,
			Style.FaceSwitchProgress);
		View.HorizontalScale = FMath::Lerp(1.0f, MinimumScale, CompressAlpha);
	}
	else
	{
		const float ExpandAlpha = SmoothRange(
			Progress,
			Style.FaceSwitchProgress,
			Style.FaceExpandEndProgress);
		View.HorizontalScale = FMath::Lerp(MinimumScale, 1.0f, ExpandAlpha);
	}

	float LandingAlpha = 0.0f;
	if (Progress < Style.LandingPeakProgress)
	{
		LandingAlpha = SmoothRange(
			Progress,
			Style.LandingStartProgress,
			Style.LandingPeakProgress);
	}
	else
	{
		LandingAlpha = 1.0f - SmoothRange(Progress, Style.LandingPeakProgress, 1.0f);
	}
	View.LandingScale = FMath::Lerp(FVector2D::UnitVector, Style.LandingScale, LandingAlpha);
	View.LandingTranslationYPixels = FMath::Max(0.0f, Style.LandingTranslationYPixels) * LandingAlpha;
	return View;
}

void FWacomFirstPersonCardDrawRevealPlayback::Reset()
{
	Phase = EWacomFirstPersonCardDrawRevealPhase::Inactive;
	Config = FWacomFirstPersonCardDrawRevealConfig();
	Progress = 0.0f;
}
