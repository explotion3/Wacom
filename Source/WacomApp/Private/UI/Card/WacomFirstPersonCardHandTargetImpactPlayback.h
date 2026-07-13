// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

enum class EWacomFirstPersonCardHandTargetImpactPhase : uint8
{
	Inactive,
	PreviewEntering,
	PreviewSustain,
	PreviewExiting,
	Commit
};

struct FWacomFirstPersonCardHandTargetImpactSoundRequest
{
	TObjectPtr<USoundBase> Sound = nullptr;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

struct FWacomFirstPersonCardHandTargetImpactPlaybackView
{
	EWacomFirstPersonCardHandTargetImpactPhase Phase =
		EWacomFirstPersonCardHandTargetImpactPhase::Inactive;
	bool bReducedMotion = false;
	bool bDepartureGateOpen = false;
	bool bCompleted = false;
	float PreviewAmount = 0.0f;
	float CommitProgress = 0.0f;
	float TimeSeconds = 0.0f;
	float ScaleMultiplier = 1.0f;
	float TranslationYPixels = 0.0f;
	int32 ZOrderBoost = 0;
};

/** App-private deterministic playback for preview, commit impact and departure gating. */
class FWacomFirstPersonCardHandTargetImpactPlayback
{
public:
	void BeginPreview(const FWacomFirstPersonCardHandTargetImpactConfig& InConfig);
	void EndPreview();
	void BeginCommit(const FWacomFirstPersonCardHandTargetImpactConfig& InConfig, uint32 Seed);
	FWacomFirstPersonCardHandTargetImpactPlaybackView Tick(float DeltaTime);
	FWacomFirstPersonCardHandTargetImpactPlaybackView BuildView() const;
	TOptional<FWacomFirstPersonCardHandTargetImpactSoundRequest> ConsumePendingSoundRequest();
	void Reset();

	bool IsActive() const { return Phase != EWacomFirstPersonCardHandTargetImpactPhase::Inactive; }
	bool IsCommitActive() const { return Phase == EWacomFirstPersonCardHandTargetImpactPhase::Commit; }
	bool IsDepartureGateOpen() const { return bDepartureGateOpen; }

private:
	FWacomFirstPersonCardHandTargetImpactConfig Config;
	EWacomFirstPersonCardHandTargetImpactPhase Phase =
		EWacomFirstPersonCardHandTargetImpactPhase::Inactive;
	float ElapsedSeconds = 0.0f;
	float PreviewAmountAtExit = 0.0f;
	bool bDepartureGateOpen = false;
	bool bSoundRequested = false;
	uint32 CommitSeed = 0;
	TOptional<FWacomFirstPersonCardHandTargetImpactSoundRequest> PendingSoundRequest;

	void QueueImpactSound();
};
