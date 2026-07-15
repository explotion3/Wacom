// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class USoundBase;

enum class EWacomFirstPersonCardDataRewritePhase : uint8
{
	Inactive,
	Delay,
	Playing
};

struct FWacomFirstPersonCardDataRewriteSoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

struct FWacomFirstPersonCardDataRewritePlaybackView
{
	EWacomFirstPersonCardDataRewritePhase Phase =
		EWacomFirstPersonCardDataRewritePhase::Inactive;
	bool bActive = false;
	bool bReducedMotion = false;
	bool bCompleted = false;
	int32 FieldMask = 0;
	EWacomFirstPersonCardDataRewriteTone Tone =
		EWacomFirstPersonCardDataRewriteTone::Neutral;
	float Progress = 0.0f;
	float OldDissolveAmount = 0.0f;
	float NewRevealAmount = 0.0f;
	float DigitScale = 1.0f;
	float Seed = 0.0f;
};

/** App-private, deterministic and non-blocking local card-data rewrite playback. */
class FWacomFirstPersonCardDataRewritePlayback
{
public:
	void BeginOrRetarget(
		const FWacomFirstPersonCardDataRewriteConfig& InConfig,
		int32 InFieldMask,
		EWacomFirstPersonCardDataRewriteTone InTone,
		uint32 InSeed,
		int32 SequenceIndex,
		bool bAllowSequenceDelay = true);
	FWacomFirstPersonCardDataRewritePlaybackView Tick(float DeltaTime);
	FWacomFirstPersonCardDataRewritePlaybackView BuildView() const;
	TOptional<FWacomFirstPersonCardDataRewriteSoundRequest> ConsumePendingSoundRequest();
	void Reset();

	bool IsActive() const
	{
		return Phase != EWacomFirstPersonCardDataRewritePhase::Inactive;
	}

private:
	FWacomFirstPersonCardDataRewriteConfig Config;
	EWacomFirstPersonCardDataRewritePhase Phase =
		EWacomFirstPersonCardDataRewritePhase::Inactive;
	float DelaySeconds = 0.0f;
	float ElapsedSeconds = 0.0f;
	float RestartProgress = 0.0f;
	int32 FieldMask = 0;
	EWacomFirstPersonCardDataRewriteTone Tone =
		EWacomFirstPersonCardDataRewriteTone::Neutral;
	uint32 Seed = 0;
	bool bBatchSoundOwner = false;
	bool bSoundRequested = false;
	TOptional<FWacomFirstPersonCardDataRewriteSoundRequest> PendingSoundRequest;

	float ResolveDurationSeconds() const;
	void QueueSoundIfNeeded();
};
