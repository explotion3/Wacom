// Copyright Wacom. All Rights Reserved.

#include "Passives/PassiveDispatcher.h"
#include "Core/BattleOperationAdapter.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Effects/ConditionResolver.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"
#include "Hand/BattleCardZoneTransition.h"
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
		if (Card->Location == ECardLocation::Played
			|| Card->Location == ECardLocation::Unknown)
		{
			return false;
		}

		FHandZoneService::InsertCardsIntoHandAtRandom(State, { CardId });
		return Card->Location == ECardLocation::Hand;
	}
}

void FPassiveDispatcher::RunAfterPlayed(
	FBattleState& State,
	FBattleEventBus& Events,
	const FRuntimeCardInstance& Card,
	int32 RuntimeCost,
	IBattleOperationAdapter* OperationAdapter)
{
	if (!Card.Definition) { return; }

	// AfterPlayed 自成一条 chain；所有匹配 passive 按当前语义共享 scratch。
	FCardEffectChain Chain = FBattleEffectSemanticsModule::BeginCardChain(
		State,
		Events,
		FCardEffectChainBindings{
			RuntimeCost,
			Card.InstanceId,
			FGuid(),
			FGuid() },
		OperationAdapter);

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

		Chain.Execute(Passive.Effects);
	}
}

void FPassiveDispatcher::RunOnCompanionCount(
	FBattleState& State,
	FBattleEventBus& Events,
	IBattleOperationAdapter* OperationAdapter)
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

	if (!Candidates.IsEmpty() && OperationAdapter)
	{
		const FBattleOperationDescriptor Operation{
			EBattleOperationKind::DirectRule,
			EBattleOperationDeterminism::Random,
			FGameplayTag(),
			/*bReportUnresolvedWhenSkipped*/true };
		if (!OperationAdapter->ShouldExecute(Operation))
		{
			return;
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
		FBattleCardZoneTransition::DiscardExcessNormalCardsFromHand(
			State,
			Events,
			FBattleCardZoneTransitionCause::FromHandLimit(
				EHandLimitDiscardSource::PassiveOnCompanionCount,
				OperationAdapter));
		State.Player.CompanionPlayedCount = 0;
	}
}


// ================ OnDiscard ================

void FPassiveDispatcher::RunOnDiscard(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& DiscardedCardId,
	IBattleOperationAdapter* OperationAdapter)
{
	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, DiscardedCardId);
	if (!Card || !Card->Definition) { return; }

	for (const FCardPassive& Passive : Card->Definition->Passives)
	{
		if (Passive.Trigger != WacomTags::Passive_Trigger_OnDiscard) { continue; }
		if (!FConditionResolver::Evaluate(State, Passive.Condition, DiscardedCardId, FGuid())) { continue; }

		FCardEffectChain Chain = FBattleEffectSemanticsModule::BeginCardChain(
			State,
			Events,
			FCardEffectChainBindings{ 0, DiscardedCardId, FGuid(), FGuid() },
			OperationAdapter);
		Chain.Execute(Passive.Effects);
	}
}
