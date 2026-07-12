// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class USoundBase;

enum class EWacomFirstPersonCardSurfaceDepartureKind : uint8
{
	None,
	CardUse,
	ExhaustDissolve
};

struct FWacomFirstPersonCardSurfaceDeparturePlaybackConfig
{
	EWacomFirstPersonCardSurfaceDepartureKind Kind =
		EWacomFirstPersonCardSurfaceDepartureKind::None;
	float DurationSeconds = 0.0f;
	float ConfirmHoldSeconds = 0.0f;
	float ImpactSeconds = 0.0f;
	float Seed = 0.0f;
	EWacomFirstPersonCardUseEffectKind CardUseEffectKind =
		EWacomFirstPersonCardUseEffectKind::DiamondWave;
	bool bReducedMotion = false;
	TWeakObjectPtr<USoundBase> StartSound;
	float SoundVolumeMultiplier = 1.0f;
	float SoundPitchMultiplier = 1.0f;
	float SoundPitchVariation = 0.0f;
};

struct FWacomFirstPersonCardSurfaceDepartureSoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

struct FWacomFirstPersonCardSurfaceDepartureTickResult
{
	EWacomFirstPersonCardSurfaceDepartureKind Kind =
		EWacomFirstPersonCardSurfaceDepartureKind::None;
	float Amount = 0.0f;
	float FlipProgress = 0.0f;
	float ImpactProgress = 0.0f;
	float MotionAlpha = 0.0f;
	float TimeSeconds = 0.0f;
	float Seed = 0.0f;
	bool bReducedMotion = false;
	bool bCompleted = false;
};

/** App-private fixed-duration playback shared by Played use and Exhaust dissolve surfaces. */
class FWacomFirstPersonCardSurfaceDeparturePlayback
{
public:
	void Begin(const FWacomFirstPersonCardSurfaceDeparturePlaybackConfig& Config);
	FWacomFirstPersonCardSurfaceDepartureTickResult Tick(float DeltaTime);
	FWacomFirstPersonCardSurfaceDepartureTickResult BuildView() const;
	TOptional<FWacomFirstPersonCardSurfaceDepartureSoundRequest> ConsumePendingSoundRequest();
	void Reset();

	bool IsActive() const { return Kind != EWacomFirstPersonCardSurfaceDepartureKind::None; }
	EWacomFirstPersonCardSurfaceDepartureKind GetKind() const { return Kind; }

private:
	EWacomFirstPersonCardSurfaceDepartureKind Kind =
		EWacomFirstPersonCardSurfaceDepartureKind::None;
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 0.0f;
	float ConfirmHoldSeconds = 0.0f;
	float ImpactSeconds = 0.0f;
	float Seed = 0.0f;
	EWacomFirstPersonCardUseEffectKind CardUseEffectKind =
		EWacomFirstPersonCardUseEffectKind::DiamondWave;
	bool bReducedMotion = false;
	bool bSoundRequestPending = false;
	FWacomFirstPersonCardSurfaceDepartureSoundRequest PendingSoundRequest;
};
