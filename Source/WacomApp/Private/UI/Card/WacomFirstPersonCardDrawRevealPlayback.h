// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

enum class EWacomFirstPersonCardDrawRevealPhase : uint8
{
	Inactive,
	Waiting,
	Playing
};

struct FWacomFirstPersonCardDrawRevealPlaybackView
{
	EWacomFirstPersonCardDrawRevealPhase Phase = EWacomFirstPersonCardDrawRevealPhase::Inactive;
	float Progress = 0.0f;
	float HorizontalScale = 1.0f;
	FVector2D LandingScale = FVector2D::UnitVector;
	float LandingTranslationYPixels = 0.0f;
	bool bReducedMotion = false;
};

/** Drawn-only reveal driven entirely by the owning Enter Transition normalized progress. */
class FWacomFirstPersonCardDrawRevealPlayback
{
public:
	void Prepare(const FWacomFirstPersonCardDrawRevealConfig& InConfig);
	void Start();
	FWacomFirstPersonCardDrawRevealPlaybackView Update(float NormalizedEnterProgress);
	FWacomFirstPersonCardDrawRevealPlaybackView BuildView() const;
	void Reset();

	bool IsActive() const { return Phase != EWacomFirstPersonCardDrawRevealPhase::Inactive; }
	bool IsWaiting() const { return Phase == EWacomFirstPersonCardDrawRevealPhase::Waiting; }
	bool IsPlaying() const { return Phase == EWacomFirstPersonCardDrawRevealPhase::Playing; }

private:
	EWacomFirstPersonCardDrawRevealPhase Phase = EWacomFirstPersonCardDrawRevealPhase::Inactive;
	FWacomFirstPersonCardDrawRevealConfig Config;
	float Progress = 0.0f;
};
