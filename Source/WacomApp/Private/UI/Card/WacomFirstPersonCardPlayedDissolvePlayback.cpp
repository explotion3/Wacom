// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPlayedDissolvePlayback.h"

#include "Sound/SoundBase.h"

void FWacomFirstPersonCardPlayedDissolvePlayback::Begin(
	const FWacomFirstPersonCardPlayedDissolveConfig& Config,
	float InSeed)
{
	Reset();
	if (!Config.bEnabled
		|| !Config.Style.SurfaceEffectMaterial
		|| !Config.Style.NoiseTexture)
	{
		return;
	}

	Style = Config.Style;
	bReducedMotion = Config.bReducedMotion;
	DurationSeconds = bReducedMotion
		? 0.12f
		: FMath::Max(KINDA_SMALL_NUMBER, Style.DurationSeconds);
	ConfirmHoldSeconds = bReducedMotion
		? 0.0f
		: FMath::Clamp(Style.ConfirmHoldSeconds, 0.0f, DurationSeconds);
	Seed = FMath::Frac(FMath::Abs(InSeed));
	bPlaying = true;

	if (Style.StartSound)
	{
		const float PitchVariation = FMath::Clamp(Style.StartSoundPitchVariation, 0.0f, 0.99f);
		PendingSoundRequest.Sound = Style.StartSound;
		PendingSoundRequest.VolumeMultiplier = FMath::Max(0.0f, Style.StartSoundVolumeMultiplier);
		PendingSoundRequest.PitchMultiplier = FMath::Max(
			0.01f,
			Style.StartSoundPitchMultiplier
				* FMath::FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation));
		bSoundRequestPending = true;
	}
}

FWacomFirstPersonCardPlayedDissolveTickResult
FWacomFirstPersonCardPlayedDissolvePlayback::Tick(float DeltaTime)
{
	Advance(DeltaTime);
	FWacomFirstPersonCardPlayedDissolveTickResult Result;
	Result.View = BuildView();
	Result.bCompleted = IsComplete();
	return Result;
}

void FWacomFirstPersonCardPlayedDissolvePlayback::Advance(float DeltaTime)
{
	if (!bPlaying)
	{
		return;
	}
	ElapsedSeconds = FMath::Min(
		DurationSeconds,
		ElapsedSeconds + FMath::Max(0.0f, DeltaTime));
}

void FWacomFirstPersonCardPlayedDissolvePlayback::Reset()
{
	Style = FWacomFirstPersonCardPlayedDissolveStyleData();
	ElapsedSeconds = 0.0f;
	DurationSeconds = 0.0f;
	ConfirmHoldSeconds = 0.0f;
	Seed = 0.0f;
	bReducedMotion = false;
	bPlaying = false;
	bSoundRequestPending = false;
	PendingSoundRequest = FWacomFirstPersonCardPlayedDissolveSoundRequest();
}

FWacomFirstPersonCardPlayedDissolveView
FWacomFirstPersonCardPlayedDissolvePlayback::BuildView() const
{
	FWacomFirstPersonCardPlayedDissolveView View;
	if (!bPlaying)
	{
		return View;
	}

	View.bActive = true;
	View.bReducedMotion = bReducedMotion;
	View.TimeSeconds = ElapsedSeconds;
	View.Seed = Seed;
	View.Style = Style;
	const float DissolveDuration = FMath::Max(KINDA_SMALL_NUMBER, DurationSeconds - ConfirmHoldSeconds);
	View.Amount = FMath::Clamp(
		(ElapsedSeconds - ConfirmHoldSeconds) / DissolveDuration,
		0.0f,
		1.0f);
	return View;
}

TOptional<FWacomFirstPersonCardPlayedDissolveSoundRequest>
FWacomFirstPersonCardPlayedDissolvePlayback::ConsumePendingSoundRequest()
{
	if (!bSoundRequestPending)
	{
		return TOptional<FWacomFirstPersonCardPlayedDissolveSoundRequest>();
	}
	bSoundRequestPending = false;
	return PendingSoundRequest;
}
