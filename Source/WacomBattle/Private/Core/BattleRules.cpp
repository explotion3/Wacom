// Copyright Wacom. All Rights Reserved.

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Cards/CardDefinition.h"

int32 FBattleRules::ComputeRuntimeCost(const FRuntimeCardInstance& Card)
{
	const int32 Base = Card.Definition ? Card.Definition->BaseCost : 0;
	return FMath::Max(0, Base + Card.RuntimeCostModifier);
}

int32 FBattleRules::ComputeEnemyInitiativeSum(const FBattleState& State)
{
	int32 Sum = 0;
	for (const FRuntimeEnemyPart& Part : State.EnemyParts)
	{
		if (!Part.bDestroyed)
		{
			Sum += Part.CurrentInitiative;
		}
	}
	return Sum;
}

bool FBattleRules::IsCardCostLegal(const FBattleState& State, const FRuntimeCardInstance& Card)
{
	return ComputeRuntimeCost(Card) <= ComputeEnemyInitiativeSum(State);
}

FRuntimeEnemyPart* FBattleRules::FindEnemyPart(FBattleState& State, const FGuid& PartInstanceId)
{
	for (FRuntimeEnemyPart& Part : State.EnemyParts)
	{
		if (Part.InstanceId == PartInstanceId)
		{
			return &Part;
		}
	}
	return nullptr;
}

const FRuntimeEnemyPart* FBattleRules::FindEnemyPart(const FBattleState& State, const FGuid& PartInstanceId)
{
	for (const FRuntimeEnemyPart& Part : State.EnemyParts)
	{
		if (Part.InstanceId == PartInstanceId)
		{
			return &Part;
		}
	}
	return nullptr;
}

FRuntimeCardInstance* FBattleRules::FindCard(FBattleState& State, const FGuid& CardInstanceId)
{
	for (FRuntimeCardInstance& Card : State.AllCards)
	{
		if (Card.InstanceId == CardInstanceId)
		{
			return &Card;
		}
	}
	return nullptr;
}

const FRuntimeCardInstance* FBattleRules::FindCard(const FBattleState& State, const FGuid& CardInstanceId)
{
	for (const FRuntimeCardInstance& Card : State.AllCards)
	{
		if (Card.InstanceId == CardInstanceId)
		{
			return &Card;
		}
	}
	return nullptr;
}

bool FBattleRules::AreAllEnemyPartsDestroyed(const FBattleState& State)
{
	if (State.EnemyParts.IsEmpty())
	{
		return false;
	}
	for (const FRuntimeEnemyPart& Part : State.EnemyParts)
	{
		if (!Part.bDestroyed)
		{
			return false;
		}
	}
	return true;
}

bool FBattleRules::CheckAndApplyBattleEnd(FBattleState& State, FBattleEventBus& Events)
{
	const bool bEnemyAllDestroyed = AreAllEnemyPartsDestroyed(State);
	const bool bPlayerDead        = State.PlayerCurrentHp <= 0;

	if (!bEnemyAllDestroyed && !bPlayerDead)
	{
		return false;
	}

	// Battle_Rules §14：同时满足时优先胜利。
	State.Outcome = bEnemyAllDestroyed ? EBattleOutcome::Victory : EBattleOutcome::Defeat;
	State.Phase   = EBattlePhase::BattleEnd;

	FBattleEvent Ev;
	Ev.Type  = EBattleEventType::BattleEnded;
	Ev.Count = (State.Outcome == EBattleOutcome::Victory) ? 1 : 0;
	Events.Emit(Ev);

	++State.StateVersion;
	return true;
}
