// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class USoundBase;

struct FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

/** App-private deterministic playback for local EffectBadge changes. */
class FWacomFirstPersonCardEffectBadgeFeedbackPlayback
{
public:
	void Begin(
		const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig,
		const TArray<FWacomFirstPersonCardEffectBadgeChange>& InChanges);
	FWacomFirstPersonCardEffectBadgeFeedbackView Tick(float DeltaTime);
	FWacomFirstPersonCardEffectBadgeFeedbackView BuildView() const;
	TOptional<FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest> ConsumePendingSoundRequest();
	void Reset();

	bool IsActive() const { return bActive; }

private:
	struct FItem
	{
		FWacomFirstPersonCardEffectBadgeChange Change;
		float StartSeconds = 0.0f;
		float DurationSeconds = 0.0f;
	};

	FWacomFirstPersonCardEffectBadgeFeedbackConfig Config;
	TArray<FItem> Items;
	float ElapsedSeconds = 0.0f;
	float TotalDurationSeconds = 0.0f;
	float ReflowStartSeconds = 0.0f;
	float ReflowEndSeconds = 0.0f;
	bool bActive = false;
	bool bSoundRequested = false;
	TOptional<FWacomFirstPersonCardEffectBadgeFeedbackSoundRequest> PendingSoundRequest;

	void QueueSoundIfNeeded();
	float ResolveDuration(EWacomFirstPersonCardEffectBadgeChangeKind ChangeKind) const;
};
