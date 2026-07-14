// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartCuePlayback.h"

#include "UI/Battle/WacomBattlePresentationTargetCue.h"

bool FWacomBattleEnemyPartCuePlayback::Begin(
	const FWacomBattlePresentationTargetCue& Cue,
	float FallbackDurationSeconds)
{
	const EWacomBattleEnemyPartCuePlaybackKind NewKind = ResolveKind(Cue);
	if (NewKind == EWacomBattleEnemyPartCuePlaybackKind::None)
	{
		return false;
	}

	if (View.bActive && GetPriority(NewKind) < GetPriority(View.Kind))
	{
		return false;
	}

	View.Kind = NewKind;
	View.bActive = true;
	View.ElapsedSeconds = 0.0f;
	View.DurationSeconds = FMath::Max(
		KINDA_SMALL_NUMBER,
		Cue.Duration > 0.0f ? Cue.Duration : FallbackDurationSeconds);
	View.Progress = 0.0f;
	return true;
}

FWacomBattleEnemyPartCuePlaybackView FWacomBattleEnemyPartCuePlayback::Tick(float DeltaTime)
{
	if (!View.bActive)
	{
		return View;
	}

	View.ElapsedSeconds = FMath::Min(
		View.DurationSeconds,
		View.ElapsedSeconds + FMath::Max(0.0f, DeltaTime));
	View.Progress = FMath::Clamp(
		View.ElapsedSeconds / FMath::Max(KINDA_SMALL_NUMBER, View.DurationSeconds),
		0.0f,
		1.0f);
	if (View.Progress >= 1.0f)
	{
		View.bActive = false;
	}
	return View;
}

void FWacomBattleEnemyPartCuePlayback::ForceComplete()
{
	if (View.Kind == EWacomBattleEnemyPartCuePlaybackKind::None)
	{
		return;
	}

	View.ElapsedSeconds = View.DurationSeconds;
	View.Progress = 1.0f;
	View.bActive = false;
}

void FWacomBattleEnemyPartCuePlayback::Reset()
{
	View = FWacomBattleEnemyPartCuePlaybackView();
}

FName FWacomBattleEnemyPartCuePlayback::KindToName(
	EWacomBattleEnemyPartCuePlaybackKind Kind)
{
	switch (Kind)
	{
	case EWacomBattleEnemyPartCuePlaybackKind::TargetConfirmed:
		return TEXT("TargetConfirmed");
	case EWacomBattleEnemyPartCuePlaybackKind::Damage:
		return TEXT("Damage");
	case EWacomBattleEnemyPartCuePlaybackKind::Destroyed:
		return TEXT("Destroyed");
	case EWacomBattleEnemyPartCuePlaybackKind::None:
	default:
		return TEXT("None");
	}
}

EWacomBattleEnemyPartCuePlaybackKind FWacomBattleEnemyPartCuePlayback::ResolveKind(
	const FWacomBattlePresentationTargetCue& Cue)
{
	// Source event must win over the legacy CueKind default. Event queue cues created
	// before explicit kind assignment still default to DamageDealt.
	if (Cue.SourceEventType == EBattleEventType::EnemyPartHpEmptied)
	{
		return EWacomBattleEnemyPartCuePlaybackKind::Destroyed;
	}
	if (Cue.SourceEventType == EBattleEventType::DamageDealt)
	{
		return EWacomBattleEnemyPartCuePlaybackKind::Damage;
	}

	switch (Cue.CueKind)
	{
	case EWacomBattlePresentationTargetCueKind::TargetConfirmed:
		return EWacomBattleEnemyPartCuePlaybackKind::TargetConfirmed;
	case EWacomBattlePresentationTargetCueKind::EnemyPartHpEmptied:
		return EWacomBattleEnemyPartCuePlaybackKind::Destroyed;
	case EWacomBattlePresentationTargetCueKind::DamageDealt:
		return EWacomBattleEnemyPartCuePlaybackKind::Damage;
	case EWacomBattlePresentationTargetCueKind::BattleEvent:
	default:
		return EWacomBattleEnemyPartCuePlaybackKind::None;
	}
}

int32 FWacomBattleEnemyPartCuePlayback::GetPriority(
	EWacomBattleEnemyPartCuePlaybackKind Kind)
{
	switch (Kind)
	{
	case EWacomBattleEnemyPartCuePlaybackKind::Destroyed:
		return 3;
	case EWacomBattleEnemyPartCuePlaybackKind::Damage:
		return 2;
	case EWacomBattleEnemyPartCuePlaybackKind::TargetConfirmed:
		return 1;
	case EWacomBattleEnemyPartCuePlaybackKind::None:
	default:
		return 0;
	}
}
