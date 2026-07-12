// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardUseReformPlayback.h"

#include "Sound/SoundBase.h"

void FWacomFirstPersonCardUseReformPlayback::Begin(
	const FWacomFirstPersonCardUseReformPlaybackConfig& Config)
{
	Reset();
	DissolveOutSeconds = Config.bReducedMotion
		? 0.10f
		: FMath::Max(0.0f, Config.DissolveOutSeconds);
	HiddenHoldSeconds = Config.bReducedMotion
		? 0.04f
		: FMath::Max(0.0f, Config.HiddenHoldSeconds);
	ReformSeconds = Config.bReducedMotion
		? 0.10f
		: FMath::Max(0.0f, Config.ReformSeconds);
	SettleSeconds = Config.bReducedMotion ? 0.0f : FMath::Max(0.0f, Config.SettleSeconds);
	ImpactSeconds = FMath::Max(0.0f, Config.ImpactSeconds);
	EffectKind = Config.EffectKind;
	if (DissolveOutSeconds <= KINDA_SMALL_NUMBER
		|| ReformSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	Phase = EWacomFirstPersonCardUseReformPhase::DissolvingOut;
	bReducedMotion = Config.bReducedMotion;
	ShadowFadeSeconds = FMath::Max(KINDA_SMALL_NUMBER, Config.ShadowFadeSeconds);

	if (USoundBase* Sound = Config.StartSound.Get())
	{
		const float PitchVariation = FMath::Clamp(Config.SoundPitchVariation, 0.0f, 0.99f);
		PendingSoundRequest.Sound = Sound;
		PendingSoundRequest.VolumeMultiplier = FMath::Max(0.0f, Config.SoundVolumeMultiplier);
		PendingSoundRequest.PitchMultiplier = FMath::Max(
			0.01f,
			Config.SoundPitchMultiplier
				* FMath::FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation));
		bSoundRequestPending = true;
	}
}

FWacomFirstPersonCardUseReformTickResult
FWacomFirstPersonCardUseReformPlayback::Tick(float DeltaTime)
{
	if (!IsActive())
	{
		return FWacomFirstPersonCardUseReformTickResult();
	}

	const float TotalDuration = DissolveOutSeconds + HiddenHoldSeconds + ReformSeconds + SettleSeconds;
	ElapsedSeconds = FMath::Min(
		TotalDuration,
		ElapsedSeconds + FMath::Max(0.0f, DeltaTime));
	if (ElapsedSeconds < DissolveOutSeconds)
	{
		Phase = EWacomFirstPersonCardUseReformPhase::DissolvingOut;
	}
	else if (ElapsedSeconds < DissolveOutSeconds + HiddenHoldSeconds)
	{
		Phase = EWacomFirstPersonCardUseReformPhase::HiddenHold;
	}
	else if (ElapsedSeconds < DissolveOutSeconds + HiddenHoldSeconds + ReformSeconds)
	{
		Phase = EWacomFirstPersonCardUseReformPhase::Reforming;
	}
	else
	{
		Phase = EWacomFirstPersonCardUseReformPhase::Settling;
	}

	FWacomFirstPersonCardUseReformTickResult Result = BuildView();
	Result.bCompleted = ElapsedSeconds >= TotalDuration;
	return Result;
}

FWacomFirstPersonCardUseReformTickResult
FWacomFirstPersonCardUseReformPlayback::BuildView() const
{
	FWacomFirstPersonCardUseReformTickResult Result;
	if (!IsActive())
	{
		return Result;
	}

	Result.Phase = Phase;
	Result.bReducedMotion = bReducedMotion;
	Result.bUseTargetSlotPosition =
		Phase == EWacomFirstPersonCardUseReformPhase::HiddenHold
		|| Phase == EWacomFirstPersonCardUseReformPhase::Reforming
		|| Phase == EWacomFirstPersonCardUseReformPhase::Settling;
	if (Phase == EWacomFirstPersonCardUseReformPhase::DissolvingOut)
	{
		Result.Amount = FMath::Clamp(
			ElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, DissolveOutSeconds),
			0.0f,
			1.0f);
		Result.TimeSeconds = ElapsedSeconds;
		if (EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip && !bReducedMotion)
		{
			Result.FlipProgress = FMath::SmoothStep(0.0f, 1.0f, Result.Amount);
			const float ImpactPhase = FMath::Clamp(
				ElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, ImpactSeconds), 0.0f, 1.0f);
			Result.ImpactProgress = ImpactSeconds > KINDA_SMALL_NUMBER
				? FMath::Sin(ImpactPhase * UE_PI) : 0.0f;
			Result.MotionAlpha = FMath::Sin(
				FMath::Clamp(ElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, ImpactSeconds * 2.0f), 0.0f, 1.0f)
				* UE_PI);
		}
	}
	else if (Phase == EWacomFirstPersonCardUseReformPhase::HiddenHold)
	{
		Result.Amount = 1.0f;
		Result.FlipProgress = EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip ? 1.0f : 0.0f;
		Result.OpacityMultiplier = 0.0f;
		Result.TimeSeconds = ShadowFadeSeconds;
	}
	else if (Phase == EWacomFirstPersonCardUseReformPhase::Reforming)
	{
		const float ReformElapsed = FMath::Max(
			0.0f,
			ElapsedSeconds - DissolveOutSeconds - HiddenHoldSeconds);
		const float ReformProgress = FMath::Clamp(
			ReformElapsed / FMath::Max(KINDA_SMALL_NUMBER, ReformSeconds),
			0.0f,
			1.0f);
		Result.Amount = 1.0f - ReformProgress;
		if (EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip && !bReducedMotion)
		{
			Result.FlipProgress = 1.0f - FMath::SmoothStep(0.0f, 1.0f, ReformProgress);
			Result.ImpactProgress = FMath::Sin(FMath::Clamp(ReformProgress / 0.35f, 0.0f, 1.0f) * UE_PI) * 0.55f;
			Result.MotionAlpha = FMath::Sin(ReformProgress * UE_PI) * 0.65f;
		}
		// The existing material derives contact-shadow opacity from CardUseTime.
		// Keep it suppressed while hidden, then restore it during the final 0.10 s.
		Result.TimeSeconds = FMath::Min(
			ShadowFadeSeconds,
			FMath::Max(0.0f, ReformSeconds - ReformElapsed));
	}
	else
	{
		Result.Amount = 0.0f;
		Result.TimeSeconds = 0.0f;
	}
	return Result;
}

TOptional<FWacomFirstPersonCardUseReformSoundRequest>
FWacomFirstPersonCardUseReformPlayback::ConsumePendingSoundRequest()
{
	if (!bSoundRequestPending)
	{
		return TOptional<FWacomFirstPersonCardUseReformSoundRequest>();
	}
	bSoundRequestPending = false;
	return PendingSoundRequest;
}

void FWacomFirstPersonCardUseReformPlayback::Reset()
{
	Phase = EWacomFirstPersonCardUseReformPhase::Inactive;
	ElapsedSeconds = 0.0f;
	DissolveOutSeconds = 0.0f;
	HiddenHoldSeconds = 0.0f;
	ReformSeconds = 0.0f;
	ShadowFadeSeconds = 0.10f;
	SettleSeconds = 0.0f;
	ImpactSeconds = 0.0f;
	EffectKind = EWacomFirstPersonCardUseEffectKind::DiamondWave;
	bReducedMotion = false;
	bSoundRequestPending = false;
	PendingSoundRequest = FWacomFirstPersonCardUseReformSoundRequest();
}
