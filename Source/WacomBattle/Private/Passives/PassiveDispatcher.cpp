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
#include "Cards/CardZoneAggregate.h"
#include "Cards/BattleCardPlacementFacts.h"
#include "Statuses/BattleStatusSemanticsModule.h"

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

	void RunTriggerForCard(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardId,
		const FGameplayTag& Trigger,
		IBattleOperationAdapter* OperationAdapter)
	{
		FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
		if (!Card || !Card->Definition)
		{
			return;
		}
		FCardEffectChain Chain = FBattleEffectSemanticsModule::BeginCardChain(
			State,
			Events,
			FCardEffectChainBindings{
				FBattleRules::ComputeRuntimeCost(State, *Card),
				CardId,
				FGuid(),
				FGuid() },
			OperationAdapter);
		for (const FCardPassive& Passive :
			Card->Definition->ResolvePassives(Card->UpgradeTier))
		{
			if (Passive.Trigger != Trigger
				|| !FConditionResolver::Evaluate(
					State,
					Passive.Condition,
					CardId,
					FGuid()))
			{
				continue;
			}
			FBattleEvent Event;
			Event.Type = EBattleEventType::PassiveTriggered;
			Event.CardInstanceId = CardId;
			Event.Tag = Trigger;
			Events.Emit(MoveTemp(Event));
			Chain.Execute(Passive.Effects);
		}
	}

	bool AutoPlayCardFromExhaust(
		FBattleState& State,
		FBattleEventBus& Events,
		const FGuid& CardId,
		IBattleOperationAdapter* OperationAdapter)
	{
		FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId);
		if (!Card
			|| !Card->Definition
			|| Card->Location != ECardLocation::Exhaust)
		{
			return false;
		}

		FBattleEvent Played;
		Played.Type = EBattleEventType::CardPlayed;
		Played.CardInstanceId = CardId;
		Played.Amount = 0;
		Events.Emit(MoveTemp(Played));

		const UCardDefinition* Definition = Card->Definition;
		const EWacomCardUpgradeTier Tier = Card->UpgradeTier;
		const bool bIsCompanion = Definition->Keywords.HasTagExact(
			WacomTags::Card_Keyword_Companion);
		FCardEffectChain Chain = FBattleEffectSemanticsModule::BeginCardChain(
			State,
			Events,
			FCardEffectChainBindings{ 0, CardId, FGuid(), FGuid() },
			OperationAdapter);
		Chain.Execute(Definition->ResolveEffects(Tier));
		FCardZoneAggregate::MoveCardFrom(
			State,
			CardId,
			ECardLocation::Exhaust,
			ECardLocation::Discard);

		FBattleEvent Destination;
		Destination.Type = EBattleEventType::CardPlayDestinationResolved;
		Destination.CardInstanceId = CardId;
		Destination.CardDestination = ECardLocation::Discard;
		Events.Emit(MoveTemp(Destination));

		if (bIsCompanion)
		{
			++State.Player.CompanionPlayedCount;
			FPassiveDispatcher::RunOnCompanionPlayed(
				State,
				Events,
				CardId,
				FBattleCardPlacementFacts{},
				false,
				OperationAdapter);
		}
		if (const FRuntimeCardInstance* CardAfter =
			FBattleRules::FindCard(State, CardId))
		{
			FPassiveDispatcher::RunAfterPlayed(
				State,
				Events,
				*CardAfter,
				0,
				OperationAdapter);
		}
		if (bIsCompanion)
		{
			FPassiveDispatcher::RunOnCompanionCount(
				State,
				Events,
				OperationAdapter);
		}
		FBattleStatusSemanticsModule::ResolveAfterPlayerCard(
			State,
			Events,
			CardId,
			FBattleCardPlacementFacts{});
		return true;
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

	for (const FCardPassive& Passive : Card.Definition->ResolvePassives(Card.UpgradeTier))
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
		for (const FCardPassive& Passive : C.Definition->ResolvePassives(C.UpgradeTier))
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

	for (const FCardPassive& Passive : Card->Definition->ResolvePassives(Card->UpgradeTier))
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

void FPassiveDispatcher::RunOnDraw(
	FBattleState& State,
	FBattleEventBus& Events,
	TConstArrayView<FGuid> DrawnCardIds,
	IBattleOperationAdapter* OperationAdapter)
{
	for (const FGuid& DrawnCardId : DrawnCardIds)
	{
		FRuntimeCardInstance* Card = FBattleRules::FindCard(State, DrawnCardId);
		if (!Card || !Card->Definition || Card->Location != ECardLocation::Hand)
		{
			continue;
		}
		const int32 RuntimeCost =
			FBattleRules::ComputeRuntimeCost(State, *Card);
		FCardEffectChain Chain = FBattleEffectSemanticsModule::BeginCardChain(
			State,
			Events,
			FCardEffectChainBindings{
				RuntimeCost,
				Card->InstanceId,
				FGuid(),
				FGuid() },
			OperationAdapter);
		for (const FCardPassive& Passive :
			Card->Definition->ResolvePassives(Card->UpgradeTier))
		{
			if (Passive.Trigger != WacomTags::Passive_Trigger_OnDraw
				|| !FConditionResolver::Evaluate(
					State,
					Passive.Condition,
					Card->InstanceId,
					FGuid()))
			{
				continue;
			}
			FBattleEvent Triggered;
			Triggered.Type = EBattleEventType::PassiveTriggered;
			Triggered.CardInstanceId = Card->InstanceId;
			Triggered.Tag = Passive.Trigger;
			Events.Emit(MoveTemp(Triggered));
			Chain.Execute(Passive.Effects);
		}
	}
}

void FPassiveDispatcher::RunOnCompanionPlayed(
	FBattleState& State,
	FBattleEventBus& Events,
	const FGuid& PlayedCardId,
	const FBattleCardPlacementFacts& PrePlayPlacement,
	const bool bAllowAdjacent,
	IBattleOperationAdapter* OperationAdapter)
{
	const FRuntimeCardInstance* PlayedCard =
		FBattleRules::FindCard(State, PlayedCardId);
	if (!PlayedCard
		|| !PlayedCard->Definition
		|| !PlayedCard->Definition->Keywords.HasTagExact(
			WacomTags::Card_Keyword_Companion))
	{
		return;
	}

	if (bAllowAdjacent)
	{
		for (const FGuid& NeighborId : {
			PrePlayPlacement.PreviousCardInstanceId,
			PrePlayPlacement.NextCardInstanceId })
		{
			if (NeighborId.IsValid())
			{
				RunTriggerForCard(
					State,
					Events,
					NeighborId,
					WacomTags::Passive_Trigger_OnAdjacentCompanionPlayed,
					OperationAdapter);
			}
		}
	}

	const TArray<FGuid> StableHand = State.Cards.Hand;
	for (const FGuid& CandidateId : StableHand)
	{
		if (CandidateId != PlayedCardId)
		{
			RunTriggerForCard(
				State,
				Events,
				CandidateId,
				WacomTags::Passive_Trigger_OnOtherCompanionPlayed,
				OperationAdapter);
		}
	}
}

void FPassiveDispatcher::RunOnTurnEnd(
	FBattleState& State,
	FBattleEventBus& Events,
	IBattleOperationAdapter* OperationAdapter)
{
	TArray<FGuid> Candidates;
	for (const FRuntimeCardInstance& Card : State.Cards.AllCards)
	{
		if (!Card.Definition)
		{
			continue;
		}
		for (const FCardPassive& Passive :
			Card.Definition->ResolvePassives(Card.UpgradeTier))
		{
			if (Passive.Trigger == WacomTags::Passive_Trigger_OnTurnEnd)
			{
				Candidates.Add(Card.InstanceId);
				break;
			}
		}
	}

	for (const FGuid& CandidateId : Candidates)
	{
		const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CandidateId);
		if (!Card || !Card->Definition)
		{
			continue;
		}
		const UCardDefinition* Definition = Card->Definition;
		const EWacomCardUpgradeTier Tier = Card->UpgradeTier;
		const TArray<FCardPassive>& Passives =
			Definition->ResolvePassives(Tier);
		for (const FCardPassive& Passive : Passives)
		{
			if (Passive.Trigger != WacomTags::Passive_Trigger_OnTurnEnd
				|| !FConditionResolver::Evaluate(
					State,
					Passive.Condition,
					CandidateId,
					FGuid()))
			{
				continue;
			}
			FBattleEvent Triggered;
			Triggered.Type = EBattleEventType::PassiveTriggered;
			Triggered.CardInstanceId = CandidateId;
			Triggered.Tag = Passive.Trigger;
			Events.Emit(MoveTemp(Triggered));

			for (const FCardEffect& Effect : Passive.Effects)
			{
				if (Effect.EffectType == WacomTags::Effect_Card_AutoPlaySelf)
				{
					AutoPlayCardFromExhaust(
						State,
						Events,
						CandidateId,
						OperationAdapter);
					continue;
				}
				Card = FBattleRules::FindCard(State, CandidateId);
				if (!Card || !Card->Definition)
				{
					break;
				}
				FCardEffectChain Chain =
					FBattleEffectSemanticsModule::BeginCardChain(
						State,
						Events,
						FCardEffectChainBindings{
							FBattleRules::ComputeRuntimeCost(State, *Card),
							CandidateId,
							FGuid(),
							FGuid() },
						OperationAdapter);
				Chain.Execute(TConstArrayView<FCardEffect>(&Effect, 1));
			}
		}
	}
}

void FPassiveDispatcher::RunOnBattleSettlement(
	FBattleState& State,
	FBattleEventBus& Events)
{
	if (State.bSettlementPassivesResolved)
	{
		return;
	}
	State.bSettlementPassivesResolved = true;

	TArray<FGuid> StableCardIds;
	StableCardIds.Reserve(State.Cards.AllCards.Num());
	for (const FRuntimeCardInstance& Card : State.Cards.AllCards)
	{
		if (Card.Definition)
		{
			StableCardIds.Add(Card.InstanceId);
		}
	}
	for (const FGuid& CardId : StableCardIds)
	{
		RunTriggerForCard(
			State,
			Events,
			CardId,
			WacomTags::Passive_Trigger_OnBattleSettlement,
			nullptr);
	}
}
