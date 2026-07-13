// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class USoundBase;

enum class EWacomFirstPersonCardTransitionPlaybackMode : uint8
{
	None,
	Enter,
	Exit
};

struct FWacomFirstPersonCardTransitionSoundRequest
{
	TSoftObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
	EWacomFirstPersonCardSlotTransitionKind TransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
};

struct FWacomFirstPersonCardTransitionTickResult
{
	FWacomFirstPersonCardLayerSlotView VisualSlotView;
	bool bHasVisualSlotView = false;
	bool bCompleted = false;
};

/** Mutually-exclusive fixed-duration semantic enter/exit playback. */
class FWacomFirstPersonCardTransitionPlayback
{
public:
	void BeginEnter(
		const FWacomFirstPersonCardLayerSlotView& StartSlotView,
		const FWacomFirstPersonCardTransitionMotionProfile& Profile);
	void BeginExit(
		const FWacomFirstPersonCardLayerSlotView& StartSlotView,
		const FWacomFirstPersonCardLayerSlotView& TargetSlotView,
		const FWacomFirstPersonCardTransitionMotionProfile& Profile);
	void Reset();
	void ResetIfMode(EWacomFirstPersonCardTransitionPlaybackMode ExpectedMode);

	FWacomFirstPersonCardTransitionTickResult Tick(
		float DeltaTime,
		const FWacomFirstPersonCardLayerSlotView& DynamicEnterTargetSlotView);
	TOptional<EWacomFirstPersonCardSlotTransitionKind> ConsumePendingStartRequest();
	TOptional<FWacomFirstPersonCardTransitionSoundRequest> ConsumePendingSoundRequest();

	bool IsActive() const { return Mode != EWacomFirstPersonCardTransitionPlaybackMode::None; }
	bool IsEnterActive() const { return Mode == EWacomFirstPersonCardTransitionPlaybackMode::Enter; }
	bool IsExitActive() const { return Mode == EWacomFirstPersonCardTransitionPlaybackMode::Exit; }
	bool BlocksInteraction() const { return IsEnterActive() && bBlockInteractionDuringPlayback; }
	float GetElapsedSeconds() const { return ElapsedSeconds; }
	float GetStartDelaySeconds() const { return StartDelaySeconds; }
	float GetDurationSeconds() const { return DurationSeconds; }

private:
	void QueueStartRequests();
	void QueueStartSoundRequest();
	void ClearActiveStatePreservingPendingRequests();

	EWacomFirstPersonCardTransitionPlaybackMode Mode =
		EWacomFirstPersonCardTransitionPlaybackMode::None;
	FWacomFirstPersonCardLayerSlotView StartSlotView;
	FWacomFirstPersonCardLayerSlotView ExitTargetSlotView;
	float ElapsedSeconds = 0.0f;
	float StartDelaySeconds = 0.0f;
	float DurationSeconds = 0.0f;
	float ArcLiftPixels = 0.0f;
	float EasePower = 1.0f;
	bool bBlockInteractionDuringPlayback = true;
	TSoftObjectPtr<USoundBase> StartSound;
	float StartSoundVolumeMultiplier = 1.0f;
	float StartSoundPitchMultiplier = 1.0f;
	EWacomFirstPersonCardSlotTransitionKind TransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	bool bStartRequested = false;
	TOptional<EWacomFirstPersonCardSlotTransitionKind> PendingStartRequest;
	TOptional<FWacomFirstPersonCardTransitionSoundRequest> PendingSoundRequest;
};
