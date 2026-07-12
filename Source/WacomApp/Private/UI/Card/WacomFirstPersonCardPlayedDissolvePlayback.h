// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class USoundBase;

struct FWacomFirstPersonCardPlayedDissolveSoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

struct FWacomFirstPersonCardPlayedDissolveTickResult
{
	FWacomFirstPersonCardPlayedDissolveView View;
	bool bCompleted = false;
};

/** App-private, fixed-duration playback for the Played surface dissolve. */
class FWacomFirstPersonCardPlayedDissolvePlayback
{
public:
	void Begin(const FWacomFirstPersonCardPlayedDissolveConfig& Config, float Seed);
	FWacomFirstPersonCardPlayedDissolveTickResult Tick(float DeltaTime);
	void Reset();

	bool IsActive() const { return bPlaying; }
	bool IsComplete() const { return bPlaying && ElapsedSeconds >= DurationSeconds; }
	FWacomFirstPersonCardPlayedDissolveView BuildView() const;
	TOptional<FWacomFirstPersonCardPlayedDissolveSoundRequest> ConsumePendingSoundRequest();

private:
	void Advance(float DeltaTime);

	FWacomFirstPersonCardPlayedDissolveStyleData Style;
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 0.0f;
	float ConfirmHoldSeconds = 0.0f;
	float Seed = 0.0f;
	bool bReducedMotion = false;
	bool bPlaying = false;
	bool bSoundRequestPending = false;
	FWacomFirstPersonCardPlayedDissolveSoundRequest PendingSoundRequest;
};
