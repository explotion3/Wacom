// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

enum class EWacomFirstPersonCardGainRevealPhase : uint8
{
	Inactive,
	Waiting,
	Playing
};

struct FWacomFirstPersonCardGainRevealPlaybackView
{
	EWacomFirstPersonCardGainRevealPhase Phase =
		EWacomFirstPersonCardGainRevealPhase::Inactive;
	float Progress = 0.0f;
	bool bReducedMotion = false;
};

/** Gained-only crystallization driven entirely by the owning Enter Transition progress. */
class FWacomFirstPersonCardGainRevealPlayback
{
public:
	void Prepare(const FWacomFirstPersonCardGainRevealConfig& InConfig);
	void Start();
	FWacomFirstPersonCardGainRevealPlaybackView Update(float NormalizedEnterProgress);
	FWacomFirstPersonCardGainRevealPlaybackView BuildView() const;
	void Reset();

	bool IsActive() const { return Phase != EWacomFirstPersonCardGainRevealPhase::Inactive; }

private:
	EWacomFirstPersonCardGainRevealPhase Phase =
		EWacomFirstPersonCardGainRevealPhase::Inactive;
	FWacomFirstPersonCardGainRevealConfig Config;
	float Progress = 0.0f;
};
