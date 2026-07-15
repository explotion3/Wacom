// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartTargetPreviewPlayback.h"

namespace
{
	float EaseOutCubic(float Alpha)
	{
		const float OneMinus = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - OneMinus * OneMinus * OneMinus;
	}
}

bool FWacomBattleEnemyPartTargetPreviewPlayback::Begin(
	EWacomBattleEnemyPartTargetPreviewKind Kind,
	float EnterSeconds,
	float ExitSeconds,
	float PulsePeriodSeconds,
	bool bReducedMotion)
{
	if (Kind == EWacomBattleEnemyPartTargetPreviewKind::None)
	{
		BeginExit();
		return false;
	}

	const bool bSameActiveKind = View.bActive
		&& View.Kind == Kind
		&& View.Phase != EWacomBattleEnemyPartTargetPreviewPhase::Exiting;
	EnterDurationSeconds = FMath::Max(KINDA_SMALL_NUMBER, EnterSeconds);
	ExitDurationSeconds = FMath::Max(KINDA_SMALL_NUMBER, ExitSeconds);
	PulsePeriod = FMath::Max(KINDA_SMALL_NUMBER, PulsePeriodSeconds);
	View.bReducedMotion = bReducedMotion;
	if (bSameActiveKind)
	{
		if (bReducedMotion)
		{
			View.Amount = 1.0f;
			View.Pulse = 0.0f;
			View.Phase = EWacomBattleEnemyPartTargetPreviewPhase::Holding;
			PhaseElapsedSeconds = 0.0f;
		}
		return false;
	}

	View.Kind = Kind;
	View.bActive = true;
	PhaseElapsedSeconds = 0.0f;
	HoldElapsedSeconds = 0.0f;
	PhaseStartAmount = bReducedMotion ? 1.0f : View.Amount;
	View.Amount = PhaseStartAmount;
	View.Pulse = 0.0f;
	View.Phase = bReducedMotion
		? EWacomBattleEnemyPartTargetPreviewPhase::Holding
		: EWacomBattleEnemyPartTargetPreviewPhase::Entering;
	return true;
}

void FWacomBattleEnemyPartTargetPreviewPlayback::BeginExit()
{
	if (!View.bActive || View.Phase == EWacomBattleEnemyPartTargetPreviewPhase::Exiting)
	{
		return;
	}
	PhaseStartAmount = View.Amount;
	PhaseElapsedSeconds = 0.0f;
	View.Pulse = 0.0f;
	View.Phase = EWacomBattleEnemyPartTargetPreviewPhase::Exiting;
}

FWacomBattleEnemyPartTargetPreviewPlaybackView
FWacomBattleEnemyPartTargetPreviewPlayback::Tick(float DeltaSeconds)
{
	if (!View.bActive)
	{
		return View;
	}

	const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);
	PhaseElapsedSeconds += SafeDelta;
	switch (View.Phase)
	{
	case EWacomBattleEnemyPartTargetPreviewPhase::Entering:
	{
		const float Alpha = FMath::Clamp(PhaseElapsedSeconds / EnterDurationSeconds, 0.0f, 1.0f);
		View.Amount = FMath::Lerp(PhaseStartAmount, 1.0f, EaseOutCubic(Alpha));
		if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
		{
			View.Amount = 1.0f;
			View.Phase = EWacomBattleEnemyPartTargetPreviewPhase::Holding;
			PhaseElapsedSeconds = 0.0f;
		}
		break;
	}
	case EWacomBattleEnemyPartTargetPreviewPhase::Holding:
		View.Amount = 1.0f;
		HoldElapsedSeconds += SafeDelta;
		View.Pulse = View.bReducedMotion || View.Kind != EWacomBattleEnemyPartTargetPreviewKind::Valid
			? 0.0f
			: 0.5f + 0.5f * FMath::Sin(2.0f * PI * HoldElapsedSeconds / PulsePeriod);
		break;
	case EWacomBattleEnemyPartTargetPreviewPhase::Exiting:
	{
		const float Alpha = FMath::Clamp(PhaseElapsedSeconds / ExitDurationSeconds, 0.0f, 1.0f);
		View.Amount = FMath::Lerp(PhaseStartAmount, 0.0f, Alpha * Alpha);
		if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
		{
			Reset();
		}
		break;
	}
	case EWacomBattleEnemyPartTargetPreviewPhase::Inactive:
	default:
		break;
	}
	return View;
}

void FWacomBattleEnemyPartTargetPreviewPlayback::Reset()
{
	View = FWacomBattleEnemyPartTargetPreviewPlaybackView();
	PhaseElapsedSeconds = 0.0f;
	HoldElapsedSeconds = 0.0f;
	PhaseStartAmount = 0.0f;
}
