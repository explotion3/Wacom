// Copyright Wacom. All Rights Reserved.

#include "Core/BattleRules.h"
#include "Cards/BattleCardRuntimeStateModule.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
#include "Cards/CardDefinition.h"
#include "Passives/PassiveDispatcher.h"

int32 FBattleRules::ComputeRuntimeCost(
	const FBattleState& State,
	const FRuntimeCardInstance& Card)
{
	return FBattleCardRuntimeStateModule::EvaluateCost(State, Card).EffectiveCost;
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
	return FBattleCardRuntimeStateModule::IsCostLegal(State, Card);
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

FRuntimeEnemyPart* FBattleRules::FindEnemyPartByKey(FBattleState& State, const FBattleEnemyPartKey& PartKey)
{
	if (!PartKey.IsValidKey())
	{
		return nullptr;
	}

	if (const int32* Idx = State.Enemy.PartIndexByKey.Find(PartKey))
	{
		return State.Enemy.Parts.IsValidIndex(*Idx) ? &State.Enemy.Parts[*Idx] : nullptr;
	}
	return nullptr;
}

const FRuntimeEnemyPart* FBattleRules::FindEnemyPartByKey(
	const FBattleState& State,
	const FBattleEnemyPartKey& PartKey)
{
	if (!PartKey.IsValidKey())
	{
		return nullptr;
	}

	if (const int32* Idx = State.Enemy.PartIndexByKey.Find(PartKey))
	{
		return State.Enemy.Parts.IsValidIndex(*Idx) ? &State.Enemy.Parts[*Idx] : nullptr;
	}
	return nullptr;
}

FBattleEnemyPartKey FBattleRules::FindEnemyPartKey(const FBattleState& State, const FGuid& PartInstanceId)
{
	const FRuntimeEnemyPart* Part = FindEnemyPart(State, PartInstanceId);
	return Part ? Part->Identity.ToEnemyPartKey() : FBattleEnemyPartKey();
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

	// 部位破坏时弹三选一面板，处理完才能判终局。
	// 如果还有未处理的击倒事件，**不**设 BattleEnd——等 KnockdownChoiceResolver
	// 处理完队列后会再次调用本函数。
	// 玩家死亡（bPlayerDead）路径例外：失败优先级高于击倒事件，立刻判 Defeat
	// （本场最后一击不会再让玩家选择援助/破坏/撤离）。
	if (bEnemyAllDestroyed && !bPlayerDead && State.PendingKnockdownEvents.Num() > 0)
	{
		return false;
	}

	// 部位击倒终局判定：
	//   敌方全死 + 玩家 HP=0 = 同归于尽：判 Victory，置 bMutualDestruction，战外 +10% 伤口
	//   敌方全死 + 玩家 HP>0 = 普通胜利：判 Victory
	//   敌方未全死 + 玩家 HP=0 = 失败：判 Defeat
	if (bEnemyAllDestroyed)
	{
		State.Outcome = EBattleOutcome::Victory;
		if (bPlayerDead)
		{
			State.bMutualDestruction = true;
		}
	}
	else
	{
		State.Outcome = EBattleOutcome::Defeat;
	}
	State.Phase = EBattlePhase::BattleEnd;
	if (State.Outcome == EBattleOutcome::Victory)
	{
		FPassiveDispatcher::RunOnBattleSettlement(State, Events);
	}

	FBattleEvent Ev;
	Ev.Type  = EBattleEventType::BattleEnded;
	Ev.Count = (State.Outcome == EBattleOutcome::Victory) ? 1 : 0;
	Events.Emit(Ev);

	return true;
}
