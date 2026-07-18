// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWacomBattlePlayerVitalsPlaybackConfig
{
	float DamageTrailHoldSeconds = 0.08f;
	float DamageTrailRecoverySeconds = 0.32f;
	float ImpactDurationSeconds = 0.16f;
	float ShieldCompressionScale = 0.94f;
	float ShieldReboundScale = 1.08f;
	bool bReducedMotion = false;
};

struct FWacomBattlePlayerVitalsPlaybackView
{
	float CurrentHpPercent = 0.0f;
	float DamageTrailPercent = 0.0f;
	float DamagePulseAmount = 0.0f;
	float ShieldPulseAmount = 0.0f;
	float ShieldScale = 1.0f;
	bool bKeepBrokenShieldVisible = false;
	bool bActive = false;
};

class FWacomBattlePlayerVitalsPlayback
{
public:
	void SetAuthoritativeHpPercent(float InHpPercent);
	void BeginImpact(
		int32 PreviousHp,
		int32 PreviousMaxHp,
		int32 CurrentHp,
		int32 CurrentMaxHp,
		int32 PreviousShield,
		int32 CurrentShield,
		const FWacomBattlePlayerVitalsPlaybackConfig& Config);
	FWacomBattlePlayerVitalsPlaybackView Tick(
		float DeltaSeconds,
		const FWacomBattlePlayerVitalsPlaybackConfig& Config);
	FWacomBattlePlayerVitalsPlaybackView View(
		const FWacomBattlePlayerVitalsPlaybackConfig& Config) const;
	void Reset(float InHpPercent = 0.0f);

private:
	FWacomBattlePlayerVitalsPlaybackView Evaluate(
		const FWacomBattlePlayerVitalsPlaybackConfig& Config) const;

	float CurrentHpPercent = 0.0f;
	float DamageTrailStartPercent = 0.0f;
	float ElapsedSeconds = 0.0f;
	bool bDamageActive = false;
	bool bShieldActive = false;
	bool bShieldBreak = false;
};
