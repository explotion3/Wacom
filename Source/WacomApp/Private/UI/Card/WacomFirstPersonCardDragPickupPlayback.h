// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USoundBase;
struct FWacomFirstPersonCardDragPickupConfig;

struct FWacomFirstPersonCardDragPickupSoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

/**
 * App-private one-shot playback for the physical card pickup cue.
 *
 * The slot owns triggering, ticking and audio playback. This value object only
 * owns transient playback time, the shaped pickup alpha and a consumable sound request.
 */
class FWacomFirstPersonCardDragPickupPlayback
{
public:
	void Begin(
		const FWacomFirstPersonCardDragPickupConfig& Config,
		bool bStartVisualImmediately = true);
	void StartVisualPlayback();
	void Tick(float DeltaTime);
	void Reset();

	bool IsActive() const { return bWaitingForVisualStart || bPlaying; }
	bool IsWaitingForVisualStart() const { return bWaitingForVisualStart; }
	float GetAlpha() const;
	TOptional<FWacomFirstPersonCardDragPickupSoundRequest> ConsumePendingSoundRequest();

private:
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 0.0f;
	float RiseSeconds = 0.0f;
	bool bVisualPlaybackEnabled = false;
	bool bWaitingForVisualStart = false;
	bool bPlaying = false;
	bool bSoundRequestPending = false;
	FWacomFirstPersonCardDragPickupSoundRequest PendingSoundRequest;
};
