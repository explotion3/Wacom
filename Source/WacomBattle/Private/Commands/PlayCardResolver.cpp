// Copyright Wacom. All Rights Reserved.

#include "Commands/PlayCardResolver.h"
#include "Commands/PlayCardEvaluation.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Effects/CardEffectDispatcher.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Passives/PassiveDispatcher.h"
#include "Resolution/InitiativeResolver.h"
#include "Resolution/ZoneHookResolver.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Status/PoisonResolver.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "Cards/CardDefinition.h"

namespace
{
	/**
	 * 卡牌离开手牌后的去向。
	 *
	 * 当前覆盖：
	 * - 左/右手锚点：进入 Limbo（本回合离开手牌但不入任何区域）
	 * - Combo：回原位置（当前位置即原位置）
	 * - 其它：进入本回合使用牌堆
	 *
	 * 保留不进弃牌是回合结束时的行为，不是打出后去向。
	 */
	void SendCardAfterPlay(FBattleState& State, const FGuid& CardInstanceId, bool bIsAnchor, bool bIsCombo)
	{
		const int32 HandIdx = State.Cards.Hand.IndexOfByKey(CardInstanceId);

		if (bIsCombo && HandIdx != INDEX_NONE)
		{
			// 连击：留在原位置。Location 保持 Hand。
			FBattleRules::SetCardLocation(State, CardInstanceId, ECardLocation::Hand);
			return;
		}

		if (bIsAnchor)
		{
			if (HandIdx != INDEX_NONE)
			{
				State.Cards.Hand.RemoveAt(HandIdx);
			}
			State.Cards.Limbo.Add(CardInstanceId);
			FBattleRules::SetCardLocation(State, CardInstanceId, ECardLocation::Limbo);
			return;
		}

		// ExhaustSelf：如果本卡有临时 Exhaust 关键词，进消耗牌堆而不是弃牌堆。
		if (FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardInstanceId))
		{
			if (Card->TemporaryKeywords.HasTagExact(WacomTags::Card_Keyword_Exhaust))
			{
				if (HandIdx != INDEX_NONE)
				{
					State.Cards.Hand.RemoveAt(HandIdx);
				}
				State.Cards.ExhaustPile.Add(CardInstanceId);
				FBattleRules::SetCardLocation(State, CardInstanceId, ECardLocation::Exhaust);
				return;
			}
		}

		FDeckService::MoveFromHandToPlayedPile(State, CardInstanceId);
	}

	bool HasKeyword(const FRuntimeCardInstance& Card, const FGameplayTag& Keyword)
	{
		if (!Card.Definition) { return false; }
		return Card.Definition->Keywords.HasTag(Keyword)
		    || Card.TemporaryKeywords.HasTag(Keyword);
	}

}

FWacomStatus FPlayCardResolver::ResolvePrepared(
	FBattleState& State,
	FBattleEventBus& Events,
	const FPreparedPlayCard& Prepared,
	IBattleOperationAdapter& OperationAdapter)
{
	if (Prepared.GetEvaluatedStateVersion() != State.StateVersion)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("StalePlayCardEvaluation"));
	}

	const FBattleCommand& Command = Prepared.GetCanonicalCommand();
	const UCardDefinition* Def = Prepared.GetSourceDefinition();
	const FRuntimeCardInstance* EvaluatedCard =
		FBattleRules::FindCard(State, Command.CardInstanceId);
	if (!Def || !EvaluatedCard || EvaluatedCard->Definition != Def)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("StalePlayCardEvaluation"));
	}

	const FPlayCardTargetFacts& Target = Prepared.GetExecutionTarget();
	const int32 RuntimeCost = Prepared.GetRuntimeCost();
	const bool bAnchor = Prepared.IsAnchor();
	const bool bSwift = Prepared.IsSwift();
	const bool bCombo = Prepared.IsCombo();
	const FGuid CardId = Command.CardInstanceId;
	const FGuid SelectedPartId = Target.EnemyPartInstanceId;
	const FBattleEnemyPartKey SelectedPartKey = Target.EnemyPartKey;
	const FGuid SelectedHandCardId = Target.HandCardInstanceId;

	// ================ 1. 打牌事件 ================
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::CardPlayed;
		Ev.CardInstanceId  = CardId;
		Ev.ActorInstanceId = SelectedPartId;
		Ev.ActorEnemyPartKey = SelectedPartKey;
		Ev.Amount          = RuntimeCost;
		Events.Emit(Ev);
	}

	// ================ 2. ZoneHook: OnPlay ================
	// 放在 CardPlayed 之后、"记录出牌前先机"之前。
	FZoneHookResolver::RunOnPlayHooks(
		State,
		Events,
		*Def,
		RuntimeCost,
		SelectedPartId,
		CardId,
		&OperationAdapter);

	// ================ 3. 先机命中 + 抵抗（先于完美释放）================
	TArray<FInitiativeResolver::FPreCastEntry> PreCastInitiative;
	FInitiativeResolver::SnapshotInitiativeBeforePlay(State, PreCastInitiative);

	TArray<FGuid> HitPartIds;
	FInitiativeResolver::CollectInitiativeHits(PreCastInitiative, RuntimeCost, HitPartIds);

	for (const FGuid& HitId : HitPartIds)
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::InitiativeHit;
		Ev.ActorInstanceId = HitId;
		Ev.ActorEnemyPartKey = FBattleRules::FindEnemyPartKey(State, HitId);
		Ev.CardInstanceId  = CardId;
		Ev.Amount          = RuntimeCost;
		Events.Emit(Ev);
	}

	FInitiativeResolver::ResolveResistance(State, Events, *Def, RuntimeCost, HitPartIds, CardId);

	// ================ 4. 主效果 ================
	// 主效果链独立维护 LastShuffledCardId（与 ZoneHook / PerfectRelease / AfterPlayed 互不串）。
	{
		FGuid MainLastShuffledCardId;
		for (const FCardEffect& Eff : Def->Effects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, RuntimeCost,
				SelectedPartId,
				CardId,
				MainLastShuffledCardId,
				SelectedHandCardId,
				&OperationAdapter);
		}
	}

	// ================ 5. 完美释放 ================
	FInitiativeResolver::ResolvePerfectRelease(
		State,
		Events,
		*Def,
		RuntimeCost,
		HitPartIds,
		CardId,
		bSwift,
		&OperationAdapter);

	// ================ 6. 先机推进 ================
	// 非迅捷卡：所有仍拥有先机的敌方部位当前先机 -= RuntimeCost。
	// 左手区 + 先机命中 -> ZoneHook(OnPerfectReleaseHit) 跳过推进。
	const bool bSkipPush = FZoneHookResolver::ShouldSkipInitiativePush(
		State, *Def, CardId, !HitPartIds.IsEmpty());

	if (!bSwift && !bSkipPush)
	{
		FBattleRules::PushEnemyInitiative(State, RuntimeCost);

		FBattleEvent Ev;
		Ev.Type   = EBattleEventType::InitiativePushed;
		Ev.Amount = RuntimeCost;
		Events.Emit(Ev);
	}

	// ================ 7. 卡牌去向 ================
	SendCardAfterPlay(State, CardId, bAnchor, bCombo);

	// ================ 8. Companion 计数累加（在 AfterPlayed 之前）================
	if (const FRuntimeCardInstance* PlayedCard = FBattleRules::FindCard(State, CardId))
	{
		if (HasKeyword(*PlayedCard, WacomTags::Card_Keyword_Companion))
		{
			++State.Player.CompanionPlayedCount;
		}
	}

	// ================ 9. AfterPlayed 被动 ================
	// 必须在"卡牌去向"之后触发（Combo 留在手牌，其他的如果作用于本卡会失败但不崩）。
	if (const FRuntimeCardInstance* CardAfter = FBattleRules::FindCard(State, CardId))
	{
		FPassiveDispatcher::RunAfterPlayed(
			State,
			Events,
			*CardAfter,
			RuntimeCost,
			&OperationAdapter);
	}

	// ================ 10. OnCompanionCount 被动 ================
	// AfterPlayed 之后触发：此时本次打出的 Companion 已计入 CompanionPlayedCount。
	FPassiveDispatcher::RunOnCompanionCount(State, Events, &OperationAdapter);

	// ================ 11. 中毒结算 ================
	// 敌方部位行动之前：中毒可能破坏部位，从而影响随后的先机归零行动集合。
	FPoisonResolver::ResolvePoisonForAllHosts(State, Events);

	// ================ 12. 敌方部位行动子流程 ================
	if (!bSwift)
	{
		FEnemyPartActionResolver::ResolveInitiativeZeroActions(State, Events, &OperationAdapter);
	}

	// ================ 13. 战斗结束判断 ================
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	++State.StateVersion;
	return FWacomStatus::Ok();
}
