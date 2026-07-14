// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWacomBattlePresentationTargetCue;

enum class EWacomBattleEnemyPartCuePlaybackKind : uint8
{
	None,
	TargetConfirmed,
	Damage,
	Destroyed,
};

struct FWacomBattleEnemyPartCuePlaybackView
{
	EWacomBattleEnemyPartCuePlaybackKind Kind = EWacomBattleEnemyPartCuePlaybackKind::None;
	bool bActive = false;
	float ElapsedSeconds = 0.0f;
	float DurationSeconds = 0.0f;
	float Progress = 0.0f;
};

/**
 * 场景敌方部位一次性语义 cue 的纯值 Playback。
 *
 * 本类型只管理互斥、优先级和时间，不直接修改 Actor、材质或 Battle 状态。
 */
class FWacomBattleEnemyPartCuePlayback
{
public:
	bool Begin(const FWacomBattlePresentationTargetCue& Cue, float FallbackDurationSeconds);
	FWacomBattleEnemyPartCuePlaybackView Tick(float DeltaTime);
	void ForceComplete();
	void Reset();

	const FWacomBattleEnemyPartCuePlaybackView& GetView() const { return View; }

	static FName KindToName(EWacomBattleEnemyPartCuePlaybackKind Kind);

private:
	static EWacomBattleEnemyPartCuePlaybackKind ResolveKind(
		const FWacomBattlePresentationTargetCue& Cue);
	static int32 GetPriority(EWacomBattleEnemyPartCuePlaybackKind Kind);

	FWacomBattleEnemyPartCuePlaybackView View;
};
