// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomFirstPersonCardRetainSealPlaybackConfig
{
	bool bEnabled = true;
	bool bReducedMotion = false;
	bool bRetainUntilExplicitRelease = false;
	float StartDelaySeconds = 0.0f;
	float SealingDurationSeconds = 0.32f;
	float ReleaseDurationSeconds = 0.16f;
	float PeakLiftPixels = 12.0f;
	float PeakScale = 1.025f;
	float HeldLiftPixels = 5.0f;
	float HeldScale = 1.01f;
	float Seed = 0.0f;
	FWacomFirstPersonCardRetainSealStyleData Style;
};

struct FWacomFirstPersonCardRetainSealPlaybackView
{
	EWacomFirstPersonCardRetainSealPhase Phase =
		EWacomFirstPersonCardRetainSealPhase::Inactive;
	float PhaseProgress = 0.0f;
	float LiftPixels = 0.0f;
	float ScaleMultiplier = 1.0f;
	bool bReducedMotion = false;
	bool bBlocksPresentation = false;
	float Seed = 0.0f;
	FWacomFirstPersonCardRetainSealStyleData Style;
};

/** Stateful retained-card seal. Held is deliberately visual but non-blocking. */
class FWacomFirstPersonCardRetainSealPlayback
{
public:
	void Begin(const FWacomFirstPersonCardRetainSealPlaybackConfig& InConfig);
	void Release();
	FWacomFirstPersonCardRetainSealPlaybackView Tick(float DeltaTime);
	FWacomFirstPersonCardRetainSealPlaybackView BuildView() const;
	void Reset();

	bool IsActive() const
	{
		return Phase != EWacomFirstPersonCardRetainSealPhase::Inactive;
	}
	bool IsBlockingPresentation() const
	{
		return Phase == EWacomFirstPersonCardRetainSealPhase::Sealing
			|| Phase == EWacomFirstPersonCardRetainSealPhase::Releasing;
	}

private:
	EWacomFirstPersonCardRetainSealPhase Phase =
		EWacomFirstPersonCardRetainSealPhase::Inactive;
	FWacomFirstPersonCardRetainSealPlaybackConfig Config;
	float ElapsedSeconds = 0.0f;
	float ReleaseStartLiftPixels = 0.0f;
	float ReleaseStartScale = 1.0f;

	void EnterHeldOrRelease();
};
