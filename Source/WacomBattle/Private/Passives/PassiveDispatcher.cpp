// Copyright Wacom. All Rights Reserved.

#include "Passives/PassiveDispatcher.h"
#include "Effects/CardEffectDispatcher.h"
#include "Effects/ConditionResolver.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/HandZoneService.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"

namespace
{
	/**
	 * 把一张卡从当前容器随机插入手牌。
	 * 上限检查由 RunOnCompanionCount 在所有候选卡移动完成后统一执行。
	 *
	 * 返回是否发生了容器迁移。若卡已在 Hand 或 Definition 缺失，返回 false。
	 */
	bool MoveCardToHandFromAnywhere(FBattleState& State, const FGuid& CardId)
	{
		FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
		if (!Card) { return false; }
		if (Card->Location == ECardLocation::Hand) { return false; }

		switch (Card->Location)
		{
		case ECardLocation::Draw:    State.Cards.DrawPile.RemoveSingle(CardId);    break;
		case ECardLocation::Discard: State.Cards.DiscardPile.RemoveSingle(CardId); break;
		case ECardLocation::Exhaust: State.Cards.ExhaustPile.RemoveSingle(CardId); break;
		case ECardLocation::Limbo:   State.Cards.Limbo.RemoveSingle(CardId);       break;
		default: break;  // Unknown / Hand 不处理
		}

		FHandZoneService::InsertCardsIntoHandAtRandom(State, { CardId });
		return true;
	}
}

void FPassiveDispatcher::RunAfterPlayed(
	FBattleState& State,
	FBattleEventBus& Events,
	const FRuntimeCardInstance& Card,
	int32 RuntimeCost)
{
	if (!Card.Definition) { return; }

	// AfterPlayed 自成一条效果链，LastShuffledCardId 独立。
	FGuid LocalLastShuffledCardId;

	for (const FCardPassive& Passive : Card.Definition->Passives)
	{
		if (Passive.Trigger != WacomTags::Passive_Trigger_AfterPlayed)
		{
			continue;
		}

		// 被动级门控：未设置则永真。
		// AfterPlayed 没有明确目标，Target 类条件传 Invalid 作为 TargetPartId。
		if (!FConditionResolver::Evaluate(State, Passive.Condition, Card.InstanceId, /*TargetPartId=*/FGuid()))
		{
			continue;
		}

		for (const FCardEffect& Eff : Passive.Effects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, RuntimeCost,
				/*SelectedPartId=*/FGuid(), Card.InstanceId, LocalLastShuffledCardId);
		}
	}
}

void FPassiveDispatcher::RunOnCompanionCount(FBattleState& State, FBattleEventBus& Events)
{
	if (State.Player.CompanionPlayedCount <= 0) { return; }

	// 收集阶段与执行阶段分离：避免在遍历 AllCards 时修改容器（虽然当前只改 Location 安全，
	// 分离让代码意图更清晰）。
	TArray<FGuid> Candidates;
	Candidates.Reserve(State.Cards.AllCards.Num());
	for (const FRuntimeCardInstance& C : State.Cards.AllCards)
	{
		if (!C.Definition) { continue; }
		if (C.Location == ECardLocation::Hand) { continue; }
		for (const FCardPassive& Passive : C.Definition->Passives)
		{
			if (Passive.Trigger != WacomTags::Passive_Trigger_OnCompanionCount) { continue; }
			if (Passive.TriggerThreshold <= 0) { continue; }
			if (State.Player.CompanionPlayedCount < Passive.TriggerThreshold) { continue; }

			// 被动级门控。OnCompanionCount 没有明确目标。
			if (!FConditionResolver::Evaluate(State, Passive.Condition, C.InstanceId, /*TargetPartId=*/FGuid()))
			{
				continue;
			}

			Candidates.Add(C.InstanceId);
			break;  // 一张卡有多个 OnCompanionCount 也只计一次
		}
	}

	bool bAnyTriggered = false;
	for (const FGuid& Id : Candidates)
	{
		if (MoveCardToHandFromAnywhere(State, Id))
		{
			bAnyTriggered = true;
			FBattleEvent Ev;
			Ev.Type           = EBattleEventType::HandZoneChanged;
			Ev.CardInstanceId = Id;
			Events.Emit(Ev);
		}
	}

	if (bAnyTriggered)
	{
		TArray<FGuid> DiscardedByLimit;
		FHandZoneService::EnforceNormalCardLimit(State, DiscardedByLimit);
		WacomBattleEvents::EmitHandLimitDiscardedEvents(
			Events,
			DiscardedByLimit,
			EHandLimitDiscardSource::PassiveOnCompanionCount);
		if (!DiscardedByLimit.IsEmpty())
		{
			FBattleEvent Ev;
			Ev.Type = EBattleEventType::HandZoneChanged;
			Ev.Count = DiscardedByLimit.Num();
			Events.Emit(Ev);
		}
		State.Player.CompanionPlayedCount = 0;
	}
}


// ================ OnTurnStart / OnTurnEnd / OnDraw / OnDiscard ================

void FPassiveDispatcher::RunOnTurnStart(FBattleState& State, FBattleEventBus& Events)
{
	// 遍历所有卡（不限位置），触发拥有 OnTurnStart 被动的卡。
	for (const FRuntimeCardInstance& C : State.Cards.AllCards)
	{
		if (!C.Definition) { continue; }
		for (const FCardPassive& Passive : C.Definition->Passives)
		{
			if (Passive.Trigger != WacomTags::Passive_Trigger_OnTurnStart) { continue; }
			if (!FConditionResolver::Evaluate(State, Passive.Condition, C.InstanceId, FGuid())) { continue; }

			FGuid LocalLastShuffled;
			for (const FCardEffect& Eff : Passive.Effects)
			{
				FCardEffectDispatcher::Execute(State, Events, Eff, /*RuntimeCost=*/0,
					FGuid(), C.InstanceId, LocalLastShuffled);
			}
		}
	}
}

void FPassiveDispatcher::RunOnTurnEnd(FBattleState& State, FBattleEventBus& Events)
{
	for (const FRuntimeCardInstance& C : State.Cards.AllCards)
	{
		if (!C.Definition) { continue; }
		for (const FCardPassive& Passive : C.Definition->Passives)
		{
			if (Passive.Trigger != WacomTags::Passive_Trigger_OnTurnEnd) { continue; }
			if (!FConditionResolver::Evaluate(State, Passive.Condition, C.InstanceId, FGuid())) { continue; }

			FGuid LocalLastShuffled;
			for (const FCardEffect& Eff : Passive.Effects)
			{
				FCardEffectDispatcher::Execute(State, Events, Eff, /*RuntimeCost=*/0,
					FGuid(), C.InstanceId, LocalLastShuffled);
			}
		}
	}
}

void FPassiveDispatcher::RunOnDraw(FBattleState& State, FBattleEventBus& Events, const FGuid& DrawnCardId)
{
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, DrawnCardId);
	if (!Card || !Card->Definition) { return; }

	for (const FCardPassive& Passive : Card->Definition->Passives)
	{
		if (Passive.Trigger != WacomTags::Passive_Trigger_OnDraw) { continue; }
		if (!FConditionResolver::Evaluate(State, Passive.Condition, DrawnCardId, FGuid())) { continue; }

		FGuid LocalLastShuffled;
		for (const FCardEffect& Eff : Passive.Effects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, /*RuntimeCost=*/0,
				FGuid(), DrawnCardId, LocalLastShuffled);
		}
	}
}

void FPassiveDispatcher::RunOnDiscard(FBattleState& State, FBattleEventBus& Events, const FGuid& DiscardedCardId)
{
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, DiscardedCardId);
	if (!Card || !Card->Definition) { return; }

	for (const FCardPassive& Passive : Card->Definition->Passives)
	{
		if (Passive.Trigger != WacomTags::Passive_Trigger_OnDiscard) { continue; }
		if (!FConditionResolver::Evaluate(State, Passive.Condition, DiscardedCardId, FGuid())) { continue; }

		FGuid LocalLastShuffled;
		for (const FCardEffect& Eff : Passive.Effects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, /*RuntimeCost=*/0,
				FGuid(), DiscardedCardId, LocalLastShuffled);
		}
	}
}
