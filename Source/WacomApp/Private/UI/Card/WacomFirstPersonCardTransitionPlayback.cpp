// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardTransitionPlayback.h"

#include "UI/Card/WacomFirstPersonCardMotionMixer.h"

void FWacomFirstPersonCardTransitionPlayback::BeginEnter(
	const FWacomFirstPersonCardLayerSlotView& InStartSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& Profile)
{
	Reset();
	Mode = EWacomFirstPersonCardTransitionPlaybackMode::Enter;
	StartSlotView = InStartSlotView;
	StartDelaySeconds = FMath::Max(0.0f, Profile.StartDelaySeconds);
	DurationSeconds = FMath::Max(0.0f, Profile.DurationSeconds);
	ArcLiftPixels = FMath::Max(0.0f, Profile.ArcLiftPixels);
	EasePower = FMath::Max(0.1f, Profile.EasePower);
	bBlockInteractionDuringPlayback = Profile.bBlockInteractionDuringPlayback;
	StartSound = Profile.StartSound;
	StartSoundVolumeMultiplier = FMath::Max(0.0f, Profile.StartSoundVolumeMultiplier);
	StartSoundPitchMultiplier = FMath::Max(0.01f, Profile.StartSoundPitchMultiplier);
	TransitionKind = Profile.TransitionKind;
	if (StartDelaySeconds <= 0.0f)
	{
		QueueStartRequests();
	}
	if (StartDelaySeconds <= 0.0f && DurationSeconds <= 0.0f && ArcLiftPixels <= 0.0f)
	{
		ClearActiveStatePreservingPendingRequests();
	}
}

void FWacomFirstPersonCardTransitionPlayback::BeginExit(
	const FWacomFirstPersonCardLayerSlotView& InStartSlotView,
	const FWacomFirstPersonCardLayerSlotView& TargetSlotView,
	const FWacomFirstPersonCardTransitionMotionProfile& Profile)
{
	Reset();
	Mode = EWacomFirstPersonCardTransitionPlaybackMode::Exit;
	StartSlotView = InStartSlotView;
	ExitTargetSlotView = TargetSlotView;
	StartDelaySeconds = FMath::Max(0.0f, Profile.StartDelaySeconds);
	DurationSeconds = FMath::Max(0.0f, Profile.DurationSeconds);
	ArcLiftPixels = FMath::Max(0.0f, Profile.ArcLiftPixels);
	EasePower = FMath::Max(0.1f, Profile.EasePower);
	if (StartDelaySeconds <= 0.0f && DurationSeconds <= 0.0f)
	{
		ClearActiveStatePreservingPendingRequests();
	}
}

void FWacomFirstPersonCardTransitionPlayback::Reset()
{
	PendingStartRequest.Reset();
	PendingSoundRequest.Reset();
	ClearActiveStatePreservingPendingRequests();
}

void FWacomFirstPersonCardTransitionPlayback::ResetIfMode(
	EWacomFirstPersonCardTransitionPlaybackMode ExpectedMode)
{
	if (Mode == ExpectedMode)
	{
		Reset();
	}
}

FWacomFirstPersonCardTransitionTickResult FWacomFirstPersonCardTransitionPlayback::Tick(
	float DeltaTime,
	const FWacomFirstPersonCardLayerSlotView& DynamicEnterTargetSlotView)
{
	FWacomFirstPersonCardTransitionTickResult Result;
	if (!IsActive())
	{
		Result.bCompleted = true;
		return Result;
	}

	ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float PlaybackSeconds = ElapsedSeconds - StartDelaySeconds;
	if (PlaybackSeconds < 0.0f)
	{
		Result.VisualSlotView = StartSlotView;
		Result.bHasVisualSlotView = true;
		return Result;
	}
	if (IsEnterActive())
	{
		QueueStartRequests();
	}

	const FWacomFirstPersonCardLayerSlotView TargetSlotView =
		IsEnterActive() ? DynamicEnterTargetSlotView : ExitTargetSlotView;
	const float LinearAlpha = DurationSeconds <= 0.0f
		? 1.0f
		: FMath::Clamp(PlaybackSeconds / DurationSeconds, 0.0f, 1.0f);
	const float EasedAlpha =
		FWacomFirstPersonCardMotionMixer::ComputeTransitionEaseAlpha(LinearAlpha, EasePower);
	Result.VisualSlotView = FWacomFirstPersonCardMotionMixer::LerpSlotView(
		StartSlotView,
		TargetSlotView,
		EasedAlpha,
		EasedAlpha);
	Result.bHasVisualSlotView = true;
	if (ArcLiftPixels > 0.0f && LinearAlpha > 0.0f && LinearAlpha < 1.0f)
	{
		Result.VisualSlotView.ScreenPosition.Y -= ArcLiftPixels * FMath::Sin(LinearAlpha * UE_PI);
		Result.VisualSlotView.WidgetPosition = Result.VisualSlotView.ScreenPosition;
		Result.VisualSlotView.SnappedWidgetPosition = Result.VisualSlotView.ScreenPosition;
	}

	if (LinearAlpha >= 1.0f)
	{
		Result.VisualSlotView = TargetSlotView;
		Result.bCompleted = true;
		ClearActiveStatePreservingPendingRequests();
	}
	return Result;
}

TOptional<EWacomFirstPersonCardSlotTransitionKind>
FWacomFirstPersonCardTransitionPlayback::ConsumePendingStartRequest()
{
	TOptional<EWacomFirstPersonCardSlotTransitionKind> Result = MoveTemp(PendingStartRequest);
	PendingStartRequest.Reset();
	return Result;
}

TOptional<FWacomFirstPersonCardTransitionSoundRequest>
FWacomFirstPersonCardTransitionPlayback::ConsumePendingSoundRequest()
{
	TOptional<FWacomFirstPersonCardTransitionSoundRequest> Result = MoveTemp(PendingSoundRequest);
	PendingSoundRequest.Reset();
	return Result;
}

void FWacomFirstPersonCardTransitionPlayback::QueueStartSoundRequest()
{
	if (StartSound.IsNull())
	{
		return;
	}
	FWacomFirstPersonCardTransitionSoundRequest Request;
	Request.Sound = StartSound;
	Request.VolumeMultiplier = StartSoundVolumeMultiplier;
	Request.PitchMultiplier = StartSoundPitchMultiplier;
	Request.TransitionKind = TransitionKind;
	PendingSoundRequest = Request;
}

void FWacomFirstPersonCardTransitionPlayback::QueueStartRequests()
{
	if (bStartRequested)
	{
		return;
	}
	bStartRequested = true;
	PendingStartRequest = TransitionKind;
	QueueStartSoundRequest();
}

void FWacomFirstPersonCardTransitionPlayback::ClearActiveStatePreservingPendingRequests()
{
	Mode = EWacomFirstPersonCardTransitionPlaybackMode::None;
	StartSlotView = FWacomFirstPersonCardLayerSlotView();
	ExitTargetSlotView = FWacomFirstPersonCardLayerSlotView();
	ElapsedSeconds = 0.0f;
	StartDelaySeconds = 0.0f;
	DurationSeconds = 0.0f;
	ArcLiftPixels = 0.0f;
	EasePower = 1.0f;
	bBlockInteractionDuringPlayback = true;
	StartSound.Reset();
	StartSoundVolumeMultiplier = 1.0f;
	StartSoundPitchMultiplier = 1.0f;
	TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	bStartRequested = false;
}
