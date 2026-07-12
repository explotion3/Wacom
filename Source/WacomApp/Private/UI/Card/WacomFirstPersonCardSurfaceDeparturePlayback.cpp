// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardSurfaceDeparturePlayback.h"

#include "Sound/SoundBase.h"

void FWacomFirstPersonCardSurfaceDeparturePlayback::Begin(
	const FWacomFirstPersonCardSurfaceDeparturePlaybackConfig& Config)
{
	Reset();
	if (Config.Kind == EWacomFirstPersonCardSurfaceDepartureKind::None
		|| Config.DurationSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	Kind = Config.Kind;
	bReducedMotion = Config.bReducedMotion;
	DurationSeconds = FMath::Max(KINDA_SMALL_NUMBER, Config.DurationSeconds);
	ConfirmHoldSeconds = bReducedMotion
		? 0.0f
		: FMath::Clamp(Config.ConfirmHoldSeconds, 0.0f, DurationSeconds);
	ImpactSeconds = FMath::Max(0.0f, Config.ImpactSeconds);
	CardUseEffectKind = Config.CardUseEffectKind;
	Seed = FMath::Frac(FMath::Abs(Config.Seed));

	if (USoundBase* Sound = Config.StartSound.Get())
	{
		const float PitchVariation = FMath::Clamp(Config.SoundPitchVariation, 0.0f, 0.99f);
		PendingSoundRequest.Sound = Sound;
		PendingSoundRequest.VolumeMultiplier = FMath::Max(0.0f, Config.SoundVolumeMultiplier);
		PendingSoundRequest.PitchMultiplier = FMath::Max(
			0.01f,
			Config.SoundPitchMultiplier
				* FMath::FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation));
		bSoundRequestPending = true;
	}
}

FWacomFirstPersonCardSurfaceDepartureTickResult
FWacomFirstPersonCardSurfaceDeparturePlayback::Tick(float DeltaTime)
{
	if (IsActive())
	{
		ElapsedSeconds = FMath::Min(
			DurationSeconds,
			ElapsedSeconds + FMath::Max(0.0f, DeltaTime));
	}
	FWacomFirstPersonCardSurfaceDepartureTickResult Result = BuildView();
	Result.bCompleted = IsActive() && ElapsedSeconds >= DurationSeconds;
	return Result;
}

FWacomFirstPersonCardSurfaceDepartureTickResult
FWacomFirstPersonCardSurfaceDeparturePlayback::BuildView() const
{
	FWacomFirstPersonCardSurfaceDepartureTickResult Result;
	if (!IsActive())
	{
		return Result;
	}

	Result.Kind = Kind;
	Result.TimeSeconds = ElapsedSeconds;
	Result.Seed = Seed;
	Result.bReducedMotion = bReducedMotion;
	const float ActiveDuration = FMath::Max(
		KINDA_SMALL_NUMBER,
		DurationSeconds - ConfirmHoldSeconds);
	Result.Amount = FMath::Clamp(
		(ElapsedSeconds - ConfirmHoldSeconds) / ActiveDuration,
		0.0f,
		1.0f);
	if (Kind == EWacomFirstPersonCardSurfaceDepartureKind::CardUse
		&& CardUseEffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip
		&& !bReducedMotion)
	{
		Result.FlipProgress = FMath::SmoothStep(0.0f, 1.0f, Result.Amount);
		const float ImpactElapsed = FMath::Max(0.0f, ElapsedSeconds - ConfirmHoldSeconds);
		const float ImpactPhase = FMath::Clamp(
			ImpactElapsed / FMath::Max(KINDA_SMALL_NUMBER, ImpactSeconds),
			0.0f,
			1.0f);
		Result.ImpactProgress = ImpactSeconds > KINDA_SMALL_NUMBER
			? FMath::Sin(ImpactPhase * UE_PI)
			: 0.0f;
		const float MotionDuration = FMath::Max(
			KINDA_SMALL_NUMBER,
			ConfirmHoldSeconds + ImpactSeconds);
		const float MotionPhase = FMath::Clamp(ElapsedSeconds / MotionDuration, 0.0f, 1.0f);
		Result.MotionAlpha = FMath::Sin(MotionPhase * UE_PI);
	}
	return Result;
}

TOptional<FWacomFirstPersonCardSurfaceDepartureSoundRequest>
FWacomFirstPersonCardSurfaceDeparturePlayback::ConsumePendingSoundRequest()
{
	if (!bSoundRequestPending)
	{
		return TOptional<FWacomFirstPersonCardSurfaceDepartureSoundRequest>();
	}
	bSoundRequestPending = false;
	return PendingSoundRequest;
}

void FWacomFirstPersonCardSurfaceDeparturePlayback::Reset()
{
	Kind = EWacomFirstPersonCardSurfaceDepartureKind::None;
	ElapsedSeconds = 0.0f;
	DurationSeconds = 0.0f;
	ConfirmHoldSeconds = 0.0f;
	ImpactSeconds = 0.0f;
	Seed = 0.0f;
	CardUseEffectKind = EWacomFirstPersonCardUseEffectKind::DiamondWave;
	bReducedMotion = false;
	bSoundRequestPending = false;
	PendingSoundRequest = FWacomFirstPersonCardSurfaceDepartureSoundRequest();
}
