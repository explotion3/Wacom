// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattlePlayerVitalsPlayback.h"

namespace
{
	float ResolvePercent(const int32 Current, const int32 Maximum)
	{
		return Maximum > 0
			? FMath::Clamp(static_cast<float>(Current) / static_cast<float>(Maximum), 0.0f, 1.0f)
			: 0.0f;
	}

	float EaseOutCubic(const float Alpha)
	{
		const float Remaining = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - Remaining * Remaining * Remaining;
	}

	float EvaluateImpactPulse(const float Alpha)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return Clamped < 0.20f
			? Clamped / 0.20f
			: 1.0f - EaseOutCubic((Clamped - 0.20f) / 0.80f);
	}

	float EvaluateShieldScale(
		const float Alpha,
		const FWacomBattlePlayerVitalsPlaybackConfig& Config)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
		if (Clamped < 0.25f)
		{
			return FMath::Lerp(1.0f, Config.ShieldCompressionScale, EaseOutCubic(Clamped / 0.25f));
		}
		if (Clamped < 0.60f)
		{
			return FMath::Lerp(
				Config.ShieldCompressionScale,
				Config.ShieldReboundScale,
				EaseOutCubic((Clamped - 0.25f) / 0.35f));
		}
		return FMath::Lerp(
			Config.ShieldReboundScale,
			1.0f,
			EaseOutCubic((Clamped - 0.60f) / 0.40f));
	}
}

void FWacomBattlePlayerVitalsPlayback::SetAuthoritativeHpPercent(const float InHpPercent)
{
	CurrentHpPercent = FMath::Clamp(InHpPercent, 0.0f, 1.0f);
	if (!bDamageActive)
	{
		DamageTrailStartPercent = CurrentHpPercent;
	}
}

void FWacomBattlePlayerVitalsPlayback::BeginImpact(
	const int32 PreviousHp,
	const int32 PreviousMaxHp,
	const int32 CurrentHp,
	const int32 CurrentMaxHp,
	const int32 PreviousShield,
	const int32 CurrentShield,
	const FWacomBattlePlayerVitalsPlaybackConfig& Config)
{
	const float PreviousPercent = ResolvePercent(PreviousHp, PreviousMaxHp);
	CurrentHpPercent = ResolvePercent(CurrentHp, CurrentMaxHp);
	bDamageActive = CurrentHp < PreviousHp && CurrentHpPercent < PreviousPercent;
	bShieldActive = CurrentShield < PreviousShield;
	bShieldBreak = bShieldActive && PreviousShield > 0 && CurrentShield <= 0;
	DamageTrailStartPercent = bDamageActive ? PreviousPercent : CurrentHpPercent;
	ElapsedSeconds = 0.0f;

	if (Config.bReducedMotion)
	{
		bDamageActive = false;
		bShieldActive = false;
		bShieldBreak = false;
		DamageTrailStartPercent = CurrentHpPercent;
	}
}

FWacomBattlePlayerVitalsPlaybackView FWacomBattlePlayerVitalsPlayback::Tick(
	const float DeltaSeconds,
	const FWacomBattlePlayerVitalsPlaybackConfig& Config)
{
	ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	FWacomBattlePlayerVitalsPlaybackView Result = Evaluate(Config);
	if (!Result.bActive)
	{
		bDamageActive = false;
		bShieldActive = false;
		bShieldBreak = false;
		DamageTrailStartPercent = CurrentHpPercent;
	}
	return Result;
}

FWacomBattlePlayerVitalsPlaybackView FWacomBattlePlayerVitalsPlayback::View(
	const FWacomBattlePlayerVitalsPlaybackConfig& Config) const
{
	return Evaluate(Config);
}

void FWacomBattlePlayerVitalsPlayback::Reset(const float InHpPercent)
{
	CurrentHpPercent = FMath::Clamp(InHpPercent, 0.0f, 1.0f);
	DamageTrailStartPercent = CurrentHpPercent;
	ElapsedSeconds = 0.0f;
	bDamageActive = false;
	bShieldActive = false;
	bShieldBreak = false;
}

FWacomBattlePlayerVitalsPlaybackView FWacomBattlePlayerVitalsPlayback::Evaluate(
	const FWacomBattlePlayerVitalsPlaybackConfig& Config) const
{
	FWacomBattlePlayerVitalsPlaybackView Result;
	Result.CurrentHpPercent = CurrentHpPercent;
	Result.DamageTrailPercent = CurrentHpPercent;

	if (bDamageActive)
	{
		const float HoldSeconds = FMath::Max(0.0f, Config.DamageTrailHoldSeconds);
		const float RecoverySeconds = FMath::Max(0.0f, Config.DamageTrailRecoverySeconds);
		const float RecoveryAlpha = RecoverySeconds > KINDA_SMALL_NUMBER
			? FMath::Clamp((ElapsedSeconds - HoldSeconds) / RecoverySeconds, 0.0f, 1.0f)
			: 1.0f;
		Result.DamageTrailPercent = FMath::Lerp(
			DamageTrailStartPercent,
			CurrentHpPercent,
			EaseOutCubic(RecoveryAlpha));
		const float DamageDuration = HoldSeconds + RecoverySeconds;
		const float DamageAlpha = DamageDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(ElapsedSeconds / DamageDuration, 0.0f, 1.0f)
			: 1.0f;
		Result.DamagePulseAmount = EvaluateImpactPulse(DamageAlpha);
		Result.bActive = ElapsedSeconds < DamageDuration;
	}

	if (bShieldActive)
	{
		const float ImpactDuration = FMath::Max(0.0f, Config.ImpactDurationSeconds);
		const float ImpactAlpha = ImpactDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(ElapsedSeconds / ImpactDuration, 0.0f, 1.0f)
			: 1.0f;
		Result.ShieldPulseAmount = EvaluateImpactPulse(ImpactAlpha);
		Result.ShieldScale = Config.bReducedMotion
			? 1.0f
			: EvaluateShieldScale(ImpactAlpha, Config);
		Result.bKeepBrokenShieldVisible = bShieldBreak && ElapsedSeconds < ImpactDuration;
		Result.bActive = Result.bActive || ElapsedSeconds < ImpactDuration;
	}

	return Result;
}
