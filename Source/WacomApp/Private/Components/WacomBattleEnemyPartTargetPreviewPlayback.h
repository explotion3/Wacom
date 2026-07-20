// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EWacomBattleEnemyPartTargetPreviewKind : uint8
{
	None,
	Valid,
	Invalid,
	Available,
};

enum class EWacomBattleEnemyPartTargetPreviewPhase : uint8
{
	Inactive,
	Entering,
	Holding,
	Exiting,
};

struct FWacomBattleEnemyPartTargetPreviewPlaybackView
{
	EWacomBattleEnemyPartTargetPreviewKind Kind = EWacomBattleEnemyPartTargetPreviewKind::None;
	EWacomBattleEnemyPartTargetPreviewPhase Phase = EWacomBattleEnemyPartTargetPreviewPhase::Inactive;
	float Amount = 0.0f;
	float Pulse = 0.0f;
	bool bReducedMotion = false;
	bool bActive = false;
};

/** App-private lifecycle for one hovered world-target preview. */
class WACOMAPP_API FWacomBattleEnemyPartTargetPreviewPlayback
{
public:
	bool Begin(
		EWacomBattleEnemyPartTargetPreviewKind Kind,
		float EnterSeconds,
		float ExitSeconds,
		float PulsePeriodSeconds,
		bool bReducedMotion);
	void BeginExit();
	FWacomBattleEnemyPartTargetPreviewPlaybackView Tick(float DeltaSeconds);
	void Reset();

	const FWacomBattleEnemyPartTargetPreviewPlaybackView& GetView() const
	{
		return View;
	}

	static FName KindToName(EWacomBattleEnemyPartTargetPreviewKind Kind);

private:
	FWacomBattleEnemyPartTargetPreviewPlaybackView View;
	float PhaseElapsedSeconds = 0.0f;
	float HoldElapsedSeconds = 0.0f;
	float PhaseStartAmount = 0.0f;
	float EnterDurationSeconds = 0.18f;
	float ExitDurationSeconds = 0.10f;
	float PulsePeriod = 0.95f;
};
