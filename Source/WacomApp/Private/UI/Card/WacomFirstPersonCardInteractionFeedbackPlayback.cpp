// Copyright Wacom. All Rights Reserved.

#include "WacomFirstPersonCardInteractionFeedbackPlayback.h"

void FWacomFirstPersonCardInteractionFeedbackPlayback::SetConfig(
	const FWacomFirstPersonCardInteractionFeedbackConfig& InConfig)
{
	Config = InConfig;
	if (!Config.bEnabled)
	{
		Reset();
		return;
	}
	DenyElapsedSeconds = FMath::Min(DenyElapsedSeconds, Config.DenyDuration);
	CommitElapsedSeconds = FMath::Min(
		CommitElapsedSeconds,
		Config.PlayCommitDuration);
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::SetPressed(bool bInPressed)
{
	bPressed = Config.bEnabled && bInPressed;
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::TriggerDeny()
{
	if (Config.bEnabled && Config.DenyDuration > 0.0f)
	{
		DenyElapsedSeconds = 0.0f;
	}
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::TriggerCommit()
{
	if (Config.bEnabled
		&& Config.bEnablePlayCommitFeedback
		&& Config.PlayCommitDuration > 0.0f)
	{
		CommitElapsedSeconds = 0.0f;
	}
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::Tick(float DeltaTime)
{
	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	const float TargetAmount = Config.bEnabled
		&& !Config.bReduceInteractionMotion
		&& bPressed
		? 1.0f
		: 0.0f;
	if (!FMath::IsNearlyEqual(PressedAmount, TargetAmount, KINDA_SMALL_NUMBER))
	{
		const float Duration = TargetAmount > PressedAmount
			? Config.PressedInDurationSeconds
			: Config.PressedOutDurationSeconds;
		PressedAmount = Duration <= KINDA_SMALL_NUMBER
			? TargetAmount
			: FMath::FInterpConstantTo(
				PressedAmount,
				TargetAmount,
				SafeDeltaTime,
				1.0f / Duration);
	}
	else
	{
		PressedAmount = TargetAmount;
	}

	if (DenyElapsedSeconds < Config.DenyDuration)
	{
		DenyElapsedSeconds += SafeDeltaTime;
	}
	if (CommitElapsedSeconds < Config.PlayCommitDuration)
	{
		CommitElapsedSeconds += SafeDeltaTime;
	}
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::Reset()
{
	bPressed = false;
	PressedAmount = 0.0f;
	DenyElapsedSeconds = Config.DenyDuration;
	CommitElapsedSeconds = Config.PlayCommitDuration;
}

bool FWacomFirstPersonCardInteractionFeedbackPlayback::IsActive() const
{
	return (Config.bEnabled && (bPressed || PressedAmount > KINDA_SMALL_NUMBER))
		|| DenyElapsedSeconds < Config.DenyDuration
		|| (Config.bEnablePlayCommitFeedback
			&& CommitElapsedSeconds < Config.PlayCommitDuration);
}

FWacomFirstPersonCardInteractionFeedbackPlaybackView
FWacomFirstPersonCardInteractionFeedbackPlayback::BuildView() const
{
	FWacomFirstPersonCardInteractionFeedbackPlaybackView View;
	View.bPressed = bPressed;
	View.PressedAmount = PressedAmount;
	View.DenyElapsedSeconds = DenyElapsedSeconds;
	View.CommitElapsedSeconds = CommitElapsedSeconds;
	View.bDenyActive = Config.bEnabled
		&& DenyElapsedSeconds < Config.DenyDuration;
	View.bCommitActive = Config.bEnabled
		&& Config.bEnablePlayCommitFeedback
		&& CommitElapsedSeconds < Config.PlayCommitDuration;
	View.DenyPulseAlpha = View.bDenyActive
		? ComputePulseAlpha(DenyElapsedSeconds, Config.DenyDuration)
		: 0.0f;
	return View;
}

FWacomFirstPersonCardLocalFeedbackView
FWacomFirstPersonCardInteractionFeedbackPlayback::BuildLocalFeedbackView() const
{
	FWacomFirstPersonCardLocalFeedbackView View;
	const FWacomFirstPersonCardInteractionFeedbackPlaybackView PlaybackView = BuildView();
	const float PressedAlpha = Config.bEnabled && !Config.bReduceInteractionMotion
		? FMath::Clamp(PlaybackView.PressedAmount, 0.0f, 1.0f)
		: 0.0f;
	View.PressedScaleMultiplier = FMath::Lerp(1.0f, Config.PressedScale, PressedAlpha);
	View.PressedTranslationYPixels = Config.PressedTranslationYPixels * PressedAlpha;
	View.CommitScaleMultiplier = PlaybackView.bCommitActive
		? Config.PlayCommitScale
		: 1.0f;
	if (PlaybackView.bDenyActive && !Config.bReduceInteractionMotion)
	{
		const float Progress = FMath::Clamp(
			DenyElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, Config.DenyDuration),
			0.0f,
			1.0f);
		View.DenyTranslationXPixels = FMath::Sin(Progress * PI * 6.0f)
			* Config.DenyShakePixels
			* (1.0f - Progress);
	}
	return View;
}

float FWacomFirstPersonCardInteractionFeedbackPlayback::ComputePulseAlpha(
	float ElapsedSeconds,
	float DurationSeconds)
{
	if (DurationSeconds <= 0.0f || ElapsedSeconds >= DurationSeconds)
	{
		return 0.0f;
	}
	const float Progress = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	return 1.0f - Progress;
}
