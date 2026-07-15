// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class USoundBase;

enum class EWacomFirstPersonCardUseReformPhase : uint8
{
	Inactive,
	DissolvingOut,
	HiddenHold,
	Reforming,
	Settling
};

struct FWacomFirstPersonCardUseReformPlaybackConfig
{
	float DissolveOutSeconds = 0.28f;
	float HiddenHoldSeconds = 0.08f;
	float ReformSeconds = 0.24f;
	float ShadowFadeSeconds = 0.10f;
	float SettleSeconds = 0.0f;
	float ImpactSeconds = 0.0f;
	EWacomFirstPersonCardUseEffectKind EffectKind =
		EWacomFirstPersonCardUseEffectKind::DiamondWave;
	bool bReducedMotion = false;
	TWeakObjectPtr<USoundBase> StartSound;
	float SoundVolumeMultiplier = 1.0f;
	float SoundPitchMultiplier = 1.0f;
	float SoundPitchVariation = 0.0f;
};

struct FWacomFirstPersonCardUseReformSoundRequest
{
	TWeakObjectPtr<USoundBase> Sound;
	float VolumeMultiplier = 1.0f;
	float PitchMultiplier = 1.0f;
};

struct FWacomFirstPersonCardUseReformTickResult
{
	EWacomFirstPersonCardUseReformPhase Phase =
		EWacomFirstPersonCardUseReformPhase::Inactive;
	float Amount = 0.0f;
	float FlipProgress = 0.0f;
	float ImpactProgress = 0.0f;
	float MotionAlpha = 0.0f;
	float OpacityMultiplier = 1.0f;
	float TimeSeconds = 0.0f;
	bool bReducedMotion = false;
	bool bUseTargetSlotPosition = false;
	bool bCompleted = false;
};

/**
 * App-private retained-card playback. It dissolves at the submitted position,
 * moves only while fully hidden, then reverses the same surface material at the
 * latest hand-layout target.
 */
class FWacomFirstPersonCardUseReformPlayback
{
public:
	void Begin(const FWacomFirstPersonCardUseReformPlaybackConfig& Config);
	void BeginOutbound(const FWacomFirstPersonCardUseReformPlaybackConfig& Config);
	void BeginInbound(const FWacomFirstPersonCardUseReformPlaybackConfig& Config);
	FWacomFirstPersonCardUseReformTickResult Tick(float DeltaTime);
	FWacomFirstPersonCardUseReformTickResult BuildView() const;
	TOptional<FWacomFirstPersonCardUseReformSoundRequest> ConsumePendingSoundRequest();
	void Reset();

	bool IsActive() const
	{
		return Phase != EWacomFirstPersonCardUseReformPhase::Inactive;
	}

	bool IsHeldHidden() const
	{
		return Phase == EWacomFirstPersonCardUseReformPhase::HiddenHold
			&& bAwaitExplicitInbound;
	}

	bool IsBlockingStage() const
	{
		return IsActive() && !IsHeldHidden();
	}

private:
	void Initialize(
		const FWacomFirstPersonCardUseReformPlaybackConfig& Config,
		bool bInAwaitExplicitInbound,
		bool bRequestSound);

	EWacomFirstPersonCardUseReformPhase Phase =
		EWacomFirstPersonCardUseReformPhase::Inactive;
	float ElapsedSeconds = 0.0f;
	float DissolveOutSeconds = 0.0f;
	float HiddenHoldSeconds = 0.0f;
	float ReformSeconds = 0.0f;
	float ShadowFadeSeconds = 0.10f;
	float SettleSeconds = 0.0f;
	float ImpactSeconds = 0.0f;
	EWacomFirstPersonCardUseEffectKind EffectKind =
		EWacomFirstPersonCardUseEffectKind::DiamondWave;
	bool bReducedMotion = false;
	bool bAwaitExplicitInbound = false;
	bool bSoundRequestPending = false;
	FWacomFirstPersonCardUseReformSoundRequest PendingSoundRequest;
};
