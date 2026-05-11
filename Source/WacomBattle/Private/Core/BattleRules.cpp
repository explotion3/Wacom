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
	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
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
	if (const int32* Idx = State.Enemy.PartIndexById.Find(PartInstanceId))
	{
		return State.Enemy.Parts.IsValidIndex(*Idx) ? &State.Enemy.Parts[*Idx] : nullptr;
	}
	return nullptr;
}

const FRuntimeEnemyPart* FBattleRules::FindEnemyPart(const FBattleState& State, const FGuid& PartInstanceId)
{
	if (const int32* Idx = State.Enemy.PartIndexById.Find(PartInstanceId))
	{
		return State.Enemy.Parts.IsValidIndex(*Idx) ? &State.Enemy.Parts[*Idx] : nullptr;
	}
	return nullptr;
}

FRuntimeCardInstance* FBattleRules::FindCard(FBattleState& State, const FGuid& CardInstanceId)
{
	if (const int32* Idx = State.Cards.CardIndexById.Find(CardInstanceId))
	{
		return State.Cards.AllCards.IsValidIndex(*Idx) ? &State.Cards.AllCards[*Idx] : nullptr;
	}
	return nullptr;
}

const FRuntimeCardInstance* FBattleRules::FindCard(const FBattleState& State, const FGuid& CardInstanceId)
{
	if (const int32* Idx = State.Cards.CardIndexById.Find(CardInstanceId))
	{
		return State.Cards.AllCards.IsValidIndex(*Idx) ? &State.Cards.AllCards[*Idx] : nullptr;
	}
	return nullptr;
}

void FBattleRules::SetCardLocation(FBattleState& State, const FGuid& CardInstanceId, ECardLocation NewLocation)
{
	if (FRuntimeCardInstance* Card = FindCard(State, CardInstanceId))
	{
		Card->Location = NewLocation;
	}
}

void FBattleRules::PushEnemyInitiative(FBattleState& State, int32 Amount)
{
	if (Amount == 0) { return; }
	for (FRuntimeEnemyPart& Part : State.Enemy.Parts)
	{
		if (!Part.bDestroyed)
		{
			Part.CurrentInitiative -= Amount;
		}
	}
}

bool FBattleRules::AreAllEnemyPartsDestroyed(const FBattleState& State)
{
	if (State.Enemy.Parts.IsEmpty())
	{
		return false;
	}
	for (const FRuntimeEnemyPart& Part : State.Enemy.Parts)
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
	const bool bPlayerDead        = State.Player.CurrentHp <= 0;

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
