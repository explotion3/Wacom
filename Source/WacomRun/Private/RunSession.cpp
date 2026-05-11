// Copyright Wacom. All Rights Reserved.

#include "RunSession.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Session/BattleSession.h"

bool URunSession::Initialize(UCharacterDefinition* InCharacter)
{
	RunState = FRunState{};
	RunState.Character   = InCharacter;
	RunState.BattleSeed  = 0;
	RunState.DefeatedEnemies.Reset();
	RunState.bRunActive  = true;

	if (!InCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] Initialize: Character 为空"));
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[RunSession] Initialized with Character=%s"),
		*GetNameSafe(InCharacter));
	return true;
}

bool URunSession::BuildInitParamsForBattle(UEnemyDefinition* EnemyDef, FBattleInitParams& OutParams) const
{
	if (!RunState.Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: RunState.Character 为空"));
		return false;
	}
	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunSession] BuildInitParamsForBattle: EnemyDef 为空"));
		return false;
	}

	OutParams.Character  = RunState.Character;
	OutParams.Enemy      = EnemyDef;
	OutParams.RandomSeed = RunState.BattleSeed;
	return true;
}

void URunSession::OnBattleFinished(EBattleOutcome Outcome, UEnemyDefinition* EnemyDef)
{
	switch (Outcome)
	{
	case EBattleOutcome::Victory:
		if (EnemyDef)
		{
			RunState.DefeatedEnemies.AddUnique(EnemyDef);
		}
		UE_LOG(LogTemp, Display,
			TEXT("[RunSession] Battle victory against %s (%d total defeated)"),
			*GetNameSafe(EnemyDef),
			RunState.DefeatedEnemies.Num());
		break;

	case EBattleOutcome::Defeat:
		RunState.bRunActive = false;
		UE_LOG(LogTemp, Display, TEXT("[RunSession] Battle defeat, run ended"));
		break;

	case EBattleOutcome::Undetermined:
	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[RunSession] OnBattleFinished with Undetermined outcome, ignored"));
		break;
	}
}
