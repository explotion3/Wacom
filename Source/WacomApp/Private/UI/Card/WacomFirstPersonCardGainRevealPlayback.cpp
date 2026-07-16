// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardGainRevealPlayback.h"

void FWacomFirstPersonCardGainRevealPlayback::Prepare(
	const FWacomFirstPersonCardGainRevealConfig& InConfig)
{
	Reset();
	if (!InConfig.bEnabled || !InConfig.Style.SurfaceEffectMaterialInstance)
	{
		return;
	}
	Config = InConfig;
	Phase = EWacomFirstPersonCardGainRevealPhase::Waiting;
}

void FWacomFirstPersonCardGainRevealPlayback::Start()
{
	if (Phase == EWacomFirstPersonCardGainRevealPhase::Waiting)
	{
		Phase = EWacomFirstPersonCardGainRevealPhase::Playing;
		Progress = 0.0f;
	}
}

FWacomFirstPersonCardGainRevealPlaybackView
FWacomFirstPersonCardGainRevealPlayback::Update(float NormalizedEnterProgress)
{
	if (Phase == EWacomFirstPersonCardGainRevealPhase::Playing)
	{
		Progress = FMath::Clamp(NormalizedEnterProgress, 0.0f, 1.0f);
	}
	return BuildView();
}

FWacomFirstPersonCardGainRevealPlaybackView
FWacomFirstPersonCardGainRevealPlayback::BuildView() const
{
	FWacomFirstPersonCardGainRevealPlaybackView View;
	View.Phase = Phase;
	View.Progress = Progress;
	View.bReducedMotion = Config.bReducedMotion;
	return View;
}

void FWacomFirstPersonCardGainRevealPlayback::Reset()
{
	Phase = EWacomFirstPersonCardGainRevealPhase::Inactive;
	Config = FWacomFirstPersonCardGainRevealConfig();
	Progress = 0.0f;
}
