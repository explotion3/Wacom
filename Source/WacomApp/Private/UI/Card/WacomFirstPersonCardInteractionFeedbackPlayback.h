// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardMotionMixer.h"

struct FWacomFirstPersonCardInteractionFeedbackPlaybackView
{
	bool bPressed = false;
	float PressedAmount = 0.0f;
	float DenyElapsedSeconds = 0.0f;
	float CommitElapsedSeconds = 0.0f;
	bool bDenyActive = false;
	bool bCommitActive = false;
	float DenyPulseAlpha = 0.0f;
};

/** Owns the local Pressed/Deny/Commit envelopes without knowing about UMG. */
class FWacomFirstPersonCardInteractionFeedbackPlayback
{
public:
	void SetConfig(const FWacomFirstPersonCardInteractionFeedbackConfig& InConfig);
	void SetPressed(bool bInPressed);
	void TriggerDeny();
	void TriggerCommit();
	void Tick(float DeltaTime);
	void Reset();

	bool IsActive() const;
	FWacomFirstPersonCardInteractionFeedbackPlaybackView BuildView() const;
	FWacomFirstPersonCardLocalFeedbackView BuildLocalFeedbackView() const;

private:
	static float ComputePulseAlpha(float ElapsedSeconds, float DurationSeconds);

	FWacomFirstPersonCardInteractionFeedbackConfig Config;
	bool bPressed = false;
	float PressedAmount = 0.0f;
	float DenyElapsedSeconds = TNumericLimits<float>::Max();
	float CommitElapsedSeconds = TNumericLimits<float>::Max();
};
