// Copyright Wacom. All Rights Reserved.

#include "Initiative/BattleInitiativeTimelineModule.h"

#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeEnemyPart.h"

namespace
{
	void EmitInitiativeChanged(
		FBattleEventBus& Events,
		const FRuntimeEnemyPart& Part,
		const FGameplayTag& CauseTag,
		const FEnemyInitiativeMutationResult& Result)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EnemyInitiativeChanged;
		Event.ActorInstanceId = Part.InstanceId;
		Event.ActorEnemyPartKey = Part.Identity.ToEnemyPartKey();
		Event.Tag = CauseTag;
		Event.Amount = Result.Delta;
		Event.Count = Result.After;
		Events.Emit(Event);
	}
}

FEnemyInitiativeMutationResult FBattleInitiativeTimelineModule::SetCurrent(
	FRuntimeEnemyPart& Part,
	int32 NewValue)
{
	FEnemyInitiativeMutationResult Result;
	Result.Before = Part.CurrentInitiative;
	Result.After = NewValue;
	Result.Delta = Result.After - Result.Before;
	Result.bApplied = Result.Delta != 0;
	Part.CurrentInitiative = Result.After;
	return Result;
}

FEnemyInitiativeMutationResult FBattleInitiativeTimelineModule::ModifyCurrent(
	FRuntimeEnemyPart& Part,
	int32 Delta,
	FBattleEventBus* Events,
	const FGameplayTag& CauseTag)
{
	FEnemyInitiativeMutationResult Result = SetCurrent(Part, Part.CurrentInitiative + Delta);
	if (Result.bApplied && Events)
	{
		EmitInitiativeChanged(*Events, Part, CauseTag, Result);
	}
	return Result;
}

void FBattleInitiativeTimelineModule::PushAllLiving(
	FBattleState& State,
	int32 Amount,
	FBattleEventBus* Events,
	const FGameplayTag& CauseTag)
{
	if (Amount <= 0)
	{
		return;
	}
	for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (!Part.bDestroyed)
		{
			ModifyCurrent(Part, -Amount, Events, CauseTag);
		}
	}
}
