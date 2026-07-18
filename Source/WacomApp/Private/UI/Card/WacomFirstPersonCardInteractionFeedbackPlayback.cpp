// Copyright Wacom. All Rights Reserved.

#include "WacomFirstPersonCardInteractionFeedbackPlayback.h"

#include "Sound/SoundBase.h"

void FWacomFirstPersonCardInteractionFeedbackPlayback::SetConfig(
	const FWacomFirstPersonCardInteractionFeedbackConfig& InConfig)
{
	const float PreviousDenyDuration = Config.DenyDuration;
	const float PreviousCommitDuration = Config.PlayCommitDuration;
	const bool bWasDenyActive = PreviousDenyDuration > 0.0f
		&& DenyElapsedSeconds < PreviousDenyDuration;
	const bool bWasCommitActive = PreviousCommitDuration > 0.0f
		&& CommitElapsedSeconds < PreviousCommitDuration;
	const float PreviousDenyProgress = bWasDenyActive
		? FMath::Clamp(DenyElapsedSeconds / PreviousDenyDuration, 0.0f, 1.0f)
		: 1.0f;
	const float PreviousCommitProgress = bWasCommitActive
		? FMath::Clamp(CommitElapsedSeconds / PreviousCommitDuration, 0.0f, 1.0f)
		: 1.0f;

	Config = InConfig;
	if (!Config.bEnabled)
	{
		Reset();
		return;
	}
	DenyElapsedSeconds = bWasDenyActive && Config.DenyDuration > 0.0f
		? PreviousDenyProgress * Config.DenyDuration
		: Config.DenyDuration;
	if (!Config.bEnableInvalidTargetPreview)
	{
		bInvalidTargetPreviewRequested = false;
	}
	CommitElapsedSeconds = bWasCommitActive && Config.PlayCommitDuration > 0.0f
		? PreviousCommitProgress * Config.PlayCommitDuration
		: Config.PlayCommitDuration;
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::SetPressed(bool bInPressed)
{
	bPressed = Config.bEnabled && bInPressed;
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::SetInvalidTargetPreview(bool bInActive)
{
	bInvalidTargetPreviewRequested = Config.bEnabled
		&& Config.bEnableInvalidTargetPreview
		&& bInActive;
}

void FWacomFirstPersonCardInteractionFeedbackPlayback::TriggerDeny(
	const FVector2D& ReleaseDirection,
	int32 Seed)
{
	if (Config.bEnabled && Config.DenyDuration > 0.0f)
	{
		DenyElapsedSeconds = 0.0f;
		bInvalidTargetPreviewRequested = false;
		DenyDirection = ReleaseDirection.GetSafeNormal();
		if (DenyDirection.IsNearlyZero())
		{
			DenyDirection = FVector2D(0.0f, -1.0f);
		}
		DenySeed = Seed;
		if (Config.DenySound)
		{
			const float PitchVariation = FMath::Max(0.0f, Config.DenySoundPitchVariation);
			FRandomStream PitchRandom(Seed);
			PendingDenySoundRequest.Sound = Config.DenySound;
			PendingDenySoundRequest.VolumeMultiplier =
				FMath::Max(0.0f, Config.DenySoundVolumeMultiplier);
			PendingDenySoundRequest.PitchMultiplier = FMath::Max(
				0.01f,
				Config.DenySoundPitchMultiplier
					* PitchRandom.FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation));
			bDenySoundRequestPending = true;
		}
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

	const float InvalidPreviewTarget = Config.bEnabled
		&& Config.bEnableInvalidTargetPreview
		&& bInvalidTargetPreviewRequested
		&& DenyElapsedSeconds >= Config.DenyDuration
		? 1.0f
		: 0.0f;
	if (!FMath::IsNearlyEqual(
		InvalidTargetPreviewAmount,
		InvalidPreviewTarget,
		KINDA_SMALL_NUMBER))
	{
		const float Duration = InvalidPreviewTarget > InvalidTargetPreviewAmount
			? Config.InvalidTargetPreviewEnterDuration
			: Config.InvalidTargetPreviewExitDuration;
		InvalidTargetPreviewAmount = Duration <= KINDA_SMALL_NUMBER
			? InvalidPreviewTarget
			: FMath::FInterpConstantTo(
				InvalidTargetPreviewAmount,
				InvalidPreviewTarget,
				SafeDeltaTime,
				1.0f / Duration);
	}
	else
	{
		InvalidTargetPreviewAmount = InvalidPreviewTarget;
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
	bInvalidTargetPreviewRequested = false;
	InvalidTargetPreviewAmount = 0.0f;
	DenyElapsedSeconds = Config.DenyDuration;
	CommitElapsedSeconds = Config.PlayCommitDuration;
	DenyDirection = FVector2D(0.0f, -1.0f);
	DenySeed = 0;
	bDenySoundRequestPending = false;
	PendingDenySoundRequest = FWacomFirstPersonCardDenySoundRequest();
}

bool FWacomFirstPersonCardInteractionFeedbackPlayback::IsActive() const
{
	return (Config.bEnabled && (bPressed || PressedAmount > KINDA_SMALL_NUMBER))
		|| bInvalidTargetPreviewRequested
		|| InvalidTargetPreviewAmount > KINDA_SMALL_NUMBER
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
	View.bInvalidTargetPreviewActive = Config.bEnabled
		&& Config.bEnableInvalidTargetPreview
		&& InvalidTargetPreviewAmount > KINDA_SMALL_NUMBER;
	View.InvalidTargetPreviewAmount = FMath::Clamp(
		InvalidTargetPreviewAmount, 0.0f, 1.0f);
	View.DenyElapsedSeconds = DenyElapsedSeconds;
	View.CommitElapsedSeconds = CommitElapsedSeconds;
	View.bDenyActive = Config.bEnabled
		&& DenyElapsedSeconds < Config.DenyDuration;
	View.bCommitActive = Config.bEnabled
		&& Config.bEnablePlayCommitFeedback
		&& CommitElapsedSeconds < Config.PlayCommitDuration;
	View.DenyProgress = View.bDenyActive
		? FMath::Clamp(
			DenyElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, Config.DenyDuration),
			0.0f,
			1.0f)
		: 0.0f;
	if (View.bDenyActive)
	{
		const float FadeAlpha = 1.0f - FMath::SmoothStep(0.55f, 1.0f, View.DenyProgress);
		View.DenyPulseAlpha = FadeAlpha;
	}
	View.DenyDirection = DenyDirection;
	View.DenySeed = DenySeed;
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
		const float Progress = PlaybackView.DenyProgress;
		constexpr float CompressEnd = 0.175f;
		constexpr float RecoilPeak = 0.50f;
		if (Progress < CompressEnd)
		{
			const float Alpha = FMath::SmoothStep(0.0f, CompressEnd, Progress);
			View.DenyScaleMultiplier = FMath::Lerp(1.0f, Config.DenyCompressScale, Alpha);
		}
		else if (Progress < RecoilPeak)
		{
			const float Alpha = FMath::SmoothStep(CompressEnd, RecoilPeak, Progress);
			View.DenyScaleMultiplier = FMath::Lerp(Config.DenyCompressScale, 1.01f, Alpha);
			View.DenyTranslationPixels = -DenyDirection * Config.DenyShakePixels * Alpha;
		}
		else
		{
			const float SettleAlpha = FMath::Clamp(
				(Progress - RecoilPeak) / FMath::Max(KINDA_SMALL_NUMBER, 1.0f - RecoilPeak),
				0.0f,
				1.0f);
			const float Response = FMath::Square(1.0f - SettleAlpha)
				* FMath::Cos(SettleAlpha * PI * 1.5f);
			View.DenyScaleMultiplier = 1.0f + 0.01f * Response;
			View.DenyTranslationPixels = -DenyDirection * Config.DenyShakePixels * Response;
		}
	}
	return View;
}

TOptional<FWacomFirstPersonCardDenySoundRequest>
FWacomFirstPersonCardInteractionFeedbackPlayback::ConsumePendingDenySoundRequest()
{
	if (!bDenySoundRequestPending)
	{
		return TOptional<FWacomFirstPersonCardDenySoundRequest>();
	}
	bDenySoundRequestPending = false;
	return PendingDenySoundRequest;
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
