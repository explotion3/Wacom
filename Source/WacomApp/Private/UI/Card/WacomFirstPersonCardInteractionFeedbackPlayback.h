// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardMotionMixer.h"

class USoundBase;

struct FWacomFirstPersonCardDenySoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

struct FWacomFirstPersonCardInteractionFeedbackPlaybackView
{
	bool bPressed = false;
	float PressedAmount = 0.0f;
	bool bInvalidTargetPreviewActive = false;
	float InvalidTargetPreviewAmount = 0.0f;
	float DenyElapsedSeconds = 0.0f;
	float CommitElapsedSeconds = 0.0f;
	bool bDenyActive = false;
	bool bCommitActive = false;
	float DenyPulseAlpha = 0.0f;
	float DenyProgress = 0.0f;
	FVector2D DenyDirection = FVector2D(0.0f, -1.0f);
	int32 DenySeed = 0;
};

/** Owns the local Pressed/Deny/Commit envelopes without knowing about UMG. */
class FWacomFirstPersonCardInteractionFeedbackPlayback
{
public:
	void SetConfig(const FWacomFirstPersonCardInteractionFeedbackConfig& InConfig);
	void SetPressed(bool bInPressed);
	void SetInvalidTargetPreview(bool bInActive);
	void TriggerDeny(const FVector2D& ReleaseDirection, int32 Seed);
	void TriggerCommit();
	void Tick(float DeltaTime);
	void Reset();

	bool IsActive() const;
	FWacomFirstPersonCardInteractionFeedbackPlaybackView BuildView() const;
	FWacomFirstPersonCardLocalFeedbackView BuildLocalFeedbackView() const;
	TOptional<FWacomFirstPersonCardDenySoundRequest> ConsumePendingDenySoundRequest();

private:
	static float ComputePulseAlpha(float ElapsedSeconds, float DurationSeconds);

	FWacomFirstPersonCardInteractionFeedbackConfig Config;
	bool bPressed = false;
	float PressedAmount = 0.0f;
	bool bInvalidTargetPreviewRequested = false;
	float InvalidTargetPreviewAmount = 0.0f;
	float DenyElapsedSeconds = TNumericLimits<float>::Max();
	float CommitElapsedSeconds = TNumericLimits<float>::Max();
	FVector2D DenyDirection = FVector2D(0.0f, -1.0f);
	int32 DenySeed = 0;
	bool bDenySoundRequestPending = false;
	FWacomFirstPersonCardDenySoundRequest PendingDenySoundRequest;
};
