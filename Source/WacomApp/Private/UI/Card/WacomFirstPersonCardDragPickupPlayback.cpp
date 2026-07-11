// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDragPickupPlayback.h"

#include "Sound/SoundBase.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

void FWacomFirstPersonCardDragPickupPlayback::Begin(
	const FWacomFirstPersonCardSlotFeedbackConfig& Config,
	bool bStartVisualImmediately)
{
	Reset();
	if (!Config.bEnabled || !Config.bEnableDragPickupFeedback)
	{
		return;
	}

	DurationSeconds = FMath::Max(0.0f, Config.DragPickupDurationSeconds);
	RiseSeconds = FMath::Clamp(Config.DragPickupRiseSeconds, 0.0f, DurationSeconds);
	bVisualPlaybackEnabled = !Config.bReduceDragPickupMotion
		&& DurationSeconds > KINDA_SMALL_NUMBER
		&& (Config.DragPickupLiftPixels > KINDA_SMALL_NUMBER
			|| !FMath::IsNearlyEqual(Config.DragPickupScaleMultiplier, 1.0f));
	bWaitingForVisualStart = bVisualPlaybackEnabled && !bStartVisualImmediately;
	bPlaying = bVisualPlaybackEnabled && bStartVisualImmediately;

	if (Config.DragPickupSound)
	{
		const float PitchVariation = FMath::Max(0.0f, Config.DragPickupSoundPitchVariation);
		PendingSoundRequest.Sound = Config.DragPickupSound;
		PendingSoundRequest.VolumeMultiplier =
			FMath::Max(0.0f, Config.DragPickupSoundVolumeMultiplier);
		PendingSoundRequest.PitchMultiplier = FMath::Max(
			0.01f,
			Config.DragPickupSoundPitchMultiplier
				* FMath::FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation));
		bSoundRequestPending = true;
	}
}

void FWacomFirstPersonCardDragPickupPlayback::StartVisualPlayback()
{
	if (!bVisualPlaybackEnabled || !bWaitingForVisualStart)
	{
		return;
	}

	ElapsedSeconds = 0.0f;
	bWaitingForVisualStart = false;
	bPlaying = true;
}

void FWacomFirstPersonCardDragPickupPlayback::Tick(float DeltaTime)
{
	if (!bPlaying)
	{
		return;
	}

	ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (ElapsedSeconds >= DurationSeconds)
	{
		ElapsedSeconds = DurationSeconds;
		bPlaying = false;
	}
}

void FWacomFirstPersonCardDragPickupPlayback::Reset()
{
	ElapsedSeconds = 0.0f;
	DurationSeconds = 0.0f;
	RiseSeconds = 0.0f;
	bVisualPlaybackEnabled = false;
	bWaitingForVisualStart = false;
	bPlaying = false;
	bSoundRequestPending = false;
	PendingSoundRequest = FWacomFirstPersonCardDragPickupSoundRequest();
}

float FWacomFirstPersonCardDragPickupPlayback::GetAlpha() const
{
	if (!bPlaying || DurationSeconds <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	if (RiseSeconds > KINDA_SMALL_NUMBER && ElapsedSeconds < RiseSeconds)
	{
		const float RiseProgress = FMath::Clamp(ElapsedSeconds / RiseSeconds, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - RiseProgress, 4.0f);
	}

	const float SettleSeconds = FMath::Max(KINDA_SMALL_NUMBER, DurationSeconds - RiseSeconds);
	const float SettleProgress = FMath::Clamp(
		(ElapsedSeconds - RiseSeconds) / SettleSeconds,
		0.0f,
		1.0f);
	return FMath::Pow(1.0f - SettleProgress, 3.0f);
}

TOptional<FWacomFirstPersonCardDragPickupSoundRequest>
FWacomFirstPersonCardDragPickupPlayback::ConsumePendingSoundRequest()
{
	if (!bSoundRequestPending)
	{
		return TOptional<FWacomFirstPersonCardDragPickupSoundRequest>();
	}

	bSoundRequestPending = false;
	return PendingSoundRequest;
}
