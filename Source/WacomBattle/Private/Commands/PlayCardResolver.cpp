// Copyright Wacom. All Rights Reserved.

#include "Commands/PlayCardResolver.h"
#include "Commands/PlayCardEvaluation.h"

#include "Core/BattleRules.h"
#include "Core/BattleOperationAdapter.h"
#include "Core/BattleState.h"
#include "Effects/Semantics/BattleEffectSemanticsModule.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Hand/BattleCardZoneTransition.h"
#include "Passives/PassiveDispatcher.h"
#include "Resolution/InitiativeResolver.h"
#include "Resolution/ZoneHookResolver.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Statuses/BattleStatusSemanticsModule.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "Cards/CardDefinition.h"

namespace
{
	bool HasResolvedCardKeyword(const FRuntimeCardInstance& Card, const FGameplayTag& Keyword)
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
	IBattleOperationAdapter& OperationAdapter,
	FBattlePresentationJournal* PresentationJournal)
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
	const TArray<FGuid> FrozenPartIdsAtPlayStart =
		FBattleStatusSemanticsModule::CaptureFrozenEnemyPartsForNextCard(State);

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
	bool bSourceExplicitlyMoved = FZoneHookResolver::RunOnPlayHooks(
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
	if (bSwift)
	{
		HitPartIds.Reset();
	}
	else if (RuntimeCost > 0)
	{
		HitPartIds.RemoveAll(
			[&FrozenPartIdsAtPlayStart](const FGuid& PartId)
			{
				return FrozenPartIdsAtPlayStart.Contains(PartId);
			});
	}

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

	FCardCriticalResolutionLedger MainCriticalLedger;
	const FBattleOperationDescriptor CriticalOperation{
		EBattleOperationKind::DirectRule,
		EBattleOperationDeterminism::Random,
		FGameplayTag(),
		/*bReportUnresolvedWhenSkipped*/false };
	MainCriticalLedger.bAllowRolls =
		OperationAdapter.ShouldExecute(CriticalOperation);

	FInitiativeResolver::ResolveResistance(
		State,
		Events,
		*Def,
		RuntimeCost,
		SelectedPartId,
		HitPartIds,
		CardId,
		&MainCriticalLedger);

	// ================ 4. 主效果 ================
	// 主效果自成一条词法 chain（与 ZoneHook / PerfectRelease / AfterPlayed 互不串）。
	{
		FCardEffectChain MainChain = FBattleEffectSemanticsModule::BeginCardChain(
			State,
			Events,
			FCardEffectChainBindings{
				RuntimeCost,
				CardId,
				SelectedPartId,
				SelectedHandCardId,
				&MainCriticalLedger },
			&OperationAdapter);
		MainChain.Execute(Def->ResolveEffects(EvaluatedCard->UpgradeTier));
		bSourceExplicitlyMoved |= MainChain.WasCardShuffled(CardId);
	}

	// ================ 5. 完美释放 ================
	bSourceExplicitlyMoved |= FInitiativeResolver::ResolvePerfectRelease(
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
		FBattleStatusSemanticsModule::ResolveCardInitiativePush(
			State,
			Events,
			RuntimeCost,
			FrozenPartIdsAtPlayStart);

		FBattleEvent Ev;
		Ev.Type   = EBattleEventType::InitiativePushed;
		Ev.Amount = RuntimeCost;
		Events.Emit(Ev);
	}

	// A successful play consumes finite durability only after all primary
	// effects (including full clone creation) have resolved.
	bool bForceExhaustFromDurability = false;
	if (FRuntimeCardInstance* DurabilityCard = FBattleRules::FindCard(State, CardId);
		DurabilityCard
			&& DurabilityCard->bHasFiniteDurability
			&& DurabilityCard->CurrentDurability > 0)
	{
		--DurabilityCard->CurrentDurability;
		if (DurabilityCard->CurrentDurability <= 0)
		{
			DurabilityCard->CurrentDurability = 0;
			bForceExhaustFromDurability = true;
			DurabilityCard->TemporaryKeywords.AddTag(
				WacomTags::Card_Keyword_Exhaust);
		}
	}

	// ================ 7. 卡牌去向 ================
	const ECardLocation ResolvedCardDestination =
		FBattleCardZoneTransition::ResolvePlayedCardDestination(
		State,
		CardId,
		bAnchor,
		bCombo,
		bForceExhaustFromDurability,
		bSourceExplicitlyMoved,
		Prepared.GetPrePlayPlacement());
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::CardPlayDestinationResolved;
		Ev.CardInstanceId = CardId;
		Ev.CardDestination = ResolvedCardDestination;
		Events.Emit(Ev);
	}

	// ================ 8. Companion 计数累加（在 AfterPlayed 之前）================
	if (const FRuntimeCardInstance* PlayedCard = FBattleRules::FindCard(State, CardId))
	{
		if (HasResolvedCardKeyword(*PlayedCard, WacomTags::Card_Keyword_Companion))
		{
			++State.Player.CompanionPlayedCount;
		}
	}
	FPassiveDispatcher::RunOnCompanionPlayed(
		State,
		Events,
		CardId,
		Prepared.GetPrePlayPlacement(),
		true,
		&OperationAdapter);

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
	FBattleStatusSemanticsModule::ResolveAfterPlayerCard(
		State,
		Events,
		CardId,
		Prepared.GetPrePlayPlacement());

	// ================ 12. 敌方部位行动子流程 ================
	if (!bSwift)
	{
		FEnemyPartActionResolver::ResolveInitiativeZeroActions(
			State,
			Events,
			&OperationAdapter,
			PresentationJournal);
	}

	// ================ 13. 战斗结束判断 ================
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	return FWacomStatus::Ok();
}
