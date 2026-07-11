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
	SoundTransitionKind = Profile.SoundTransitionKind;
	if (StartDelaySeconds <= 0.0f)
	{
		QueueStartSoundRequest();
	}
	if (StartDelaySeconds <= 0.0f && DurationSeconds <= 0.0f && ArcLiftPixels <= 0.0f)
	{
		ClearActiveStatePreservingPendingSound();
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
		ClearActiveStatePreservingPendingSound();
	}
}

void FWacomFirstPersonCardTransitionPlayback::Reset()
{
	PendingSoundRequest.Reset();
	ClearActiveStatePreservingPendingSound();
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
		QueueStartSoundRequest();
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
		ClearActiveStatePreservingPendingSound();
	}
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
	if (bStartSoundRequested || StartSound.IsNull())
	{
		return;
	}
	bStartSoundRequested = true;
	FWacomFirstPersonCardTransitionSoundRequest Request;
	Request.Sound = StartSound;
	Request.VolumeMultiplier = StartSoundVolumeMultiplier;
	Request.PitchMultiplier = StartSoundPitchMultiplier;
	Request.TransitionKind = SoundTransitionKind;
	PendingSoundRequest = Request;
}

void FWacomFirstPersonCardTransitionPlayback::ClearActiveStatePreservingPendingSound()
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
	SoundTransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
	bStartSoundRequested = false;
}
