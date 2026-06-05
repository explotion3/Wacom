// Copyright Wacom. All Rights Reserved.

#include "Commands/PlayCardResolver.h"
#include "Commands/BattleCommand.h"

#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Deck/DeckService.h"
#include "Effects/CardEffectDispatcher.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Passives/PassiveDispatcher.h"
#include "Resolution/HandCardTargetEligibility.h"
#include "Resolution/InitiativeResolver.h"
#include "Resolution/ZoneHookResolver.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Runtime/RuntimeEnemyPart.h"
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

	FName MapHandCardEligibilityRejectToStatusDetail(
		EWacomHandCardTargetEligibilityReject RejectReason)
	{
		switch (RejectReason)
		{
		case EWacomHandCardTargetEligibilityReject::NormalHandCardUnsupported:
			return TEXT("TargetNormalHandCardUnsupported");
		case EWacomHandCardTargetEligibilityReject::HandAnchorUnsupported:
			return TEXT("TargetCardAnchorUnsupported");
		case EWacomHandCardTargetEligibilityReject::MissingRequiredTargetKeyword:
			return TEXT("TargetMissingRequiredKeyword");
		case EWacomHandCardTargetEligibilityReject::BlockedTargetKeyword:
			return TEXT("TargetBlockedKeyword");
		case EWacomHandCardTargetEligibilityReject::None:
		default:
			return TEXT("TargetCardFilterUnsupported");
		}
	}
}

FWacomStatus FPlayCardResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command)
{
	// ================ 1. 基础合法性 ================
	if (!Command.CardInstanceId.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoCardInstanceId"));
	}

	FRuntimeCardInstance* Card = FBattleRules::FindCard(State, Command.CardInstanceId);
	if (!Card)
	{
		return FWacomStatus::Fail(EWacomError::NotFound, TEXT("CardInstanceNotFound"));
	}
	if (Card->Location != ECardLocation::Hand)
	{
		return FWacomStatus::Fail(EWacomError::IllegalCardZone, TEXT("CardNotInHand"));
	}
	if (!Card->Definition)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CardHasNoDefinition"));
	}

	// ================ 2. 目标枚举 ================
	const UCardDefinition* Def = Card->Definition;

	FRuntimeEnemyPart* TargetPart = nullptr;
	FRuntimeCardInstance* TargetCard = nullptr;
	if (Def->TargetMode == ECardTargetMode::SingleEnemyPart)
	{
		if (!Command.TargetPartInstanceId.IsValid())
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("MissingTarget"));
		}
		TargetPart = FBattleRules::FindEnemyPart(State, Command.TargetPartInstanceId);
		if (!TargetPart || TargetPart->bDestroyed)
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetInvalid"));
		}
	}
	else if (Def->TargetMode == ECardTargetMode::HandCard)
	{
		if (!Command.TargetCardInstanceId.IsValid())
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("MissingTargetCard"));
		}
		if (Command.TargetCardInstanceId == Command.CardInstanceId)
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("SelfTargetCard"));
		}
		TargetCard = FBattleRules::FindCard(State, Command.TargetCardInstanceId);
		if (!TargetCard || TargetCard->Location != ECardLocation::Hand)
		{
			return FWacomStatus::Fail(EWacomError::IllegalTarget, TEXT("TargetCardInvalid"));
		}
		const FWacomHandCardTargetEligibility Eligibility =
			FHandCardTargetEligibility::Validate(State, *Def, Command.TargetCardInstanceId);
		if (!Eligibility.bCanTarget)
		{
			return FWacomStatus::Fail(
				EWacomError::IllegalTarget,
				MapHandCardEligibilityRejectToStatusDetail(Eligibility.RejectReason));
		}
	}
	// 其他 TargetMode 不要求 Command 带目标字段。

	// ================ 3. 费用判断 ================
	if (!FBattleRules::IsCardCostLegal(State, *Card))
	{
		return FWacomStatus::Fail(EWacomError::NotEnoughInitiative, TEXT("CostExceedsInitiativeSum"));
	}

	const int32 RuntimeCost = FBattleRules::ComputeRuntimeCost(*Card);
	const bool  bAnchor     = (Card->InstanceId == State.Cards.LeftHandInstanceId)
	                       || (Card->InstanceId == State.Cards.RightHandInstanceId);
	const bool  bSwift      = HasKeyword(*Card, WacomTags::Card_Keyword_Swift);
	const bool  bCombo      = HasKeyword(*Card, WacomTags::Card_Keyword_Combo);

	const FGuid CardId            = Card->InstanceId;
	const FGuid SelectedPartId    = TargetPart ? TargetPart->InstanceId : FGuid();
	const FGuid SelectedHandCardId = TargetCard ? TargetCard->InstanceId : FGuid();

	// ================ 4. 打牌事件 ================
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::CardPlayed;
		Ev.CardInstanceId  = CardId;
		Ev.ActorInstanceId = SelectedPartId;
		Ev.Amount          = RuntimeCost;
		Events.Emit(Ev);
	}

	// ================ 5. ZoneHook: OnPlay ================
	// 放在 CardPlayed 之后、"记录出牌前先机"之前。
	FZoneHookResolver::RunOnPlayHooks(State, Events, *Def, RuntimeCost, SelectedPartId, CardId);

	// ================ 6. 先机命中 + 抵抗（先于完美释放）================
	TArray<FInitiativeResolver::FPreCastEntry> PreCastInitiative;
	FInitiativeResolver::SnapshotInitiativeBeforePlay(State, PreCastInitiative);

	TArray<FGuid> HitPartIds;
	FInitiativeResolver::CollectInitiativeHits(PreCastInitiative, RuntimeCost, HitPartIds);

	for (const FGuid& HitId : HitPartIds)
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::InitiativeHit;
		Ev.ActorInstanceId = HitId;
		Ev.CardInstanceId  = CardId;
		Ev.Amount          = RuntimeCost;
		Events.Emit(Ev);
	}

	FInitiativeResolver::ResolveResistance(State, Events, *Def, RuntimeCost, HitPartIds, CardId);

	// ================ 7. 主效果 ================
	// 主效果链独立维护 LastShuffledCardId（与 ZoneHook / PerfectRelease / AfterPlayed 互不串）。
	{
		FGuid MainLastShuffledCardId;
		for (const FCardEffect& Eff : Def->Effects)
		{
			FCardEffectDispatcher::Execute(State, Events, Eff, RuntimeCost,
				SelectedPartId, CardId, MainLastShuffledCardId, SelectedHandCardId);
		}
	}

	// ================ 8. 完美释放 ================
	FInitiativeResolver::ResolvePerfectRelease(State, Events, *Def, RuntimeCost, HitPartIds, CardId, bSwift);

	// ================ 9. 先机推进 ================
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

	// ================ 10. 卡牌去向 ================
	SendCardAfterPlay(State, CardId, bAnchor, bCombo);

	// ================ 11. Companion 计数累加（在 AfterPlayed 之前）================
	if (const FRuntimeCardInstance* PlayedCard = FBattleRules::FindCard(State, CardId))
	{
		if (HasKeyword(*PlayedCard, WacomTags::Card_Keyword_Companion))
		{
			++State.Player.CompanionPlayedCount;
		}
	}

	// ================ 12. AfterPlayed 被动 ================
	// 必须在"卡牌去向"之后触发（Combo 留在手牌，其他的如果作用于本卡会失败但不崩）。
	if (const FRuntimeCardInstance* CardAfter = FBattleRules::FindCard(State, CardId))
	{
		FPassiveDispatcher::RunAfterPlayed(State, Events, *CardAfter, RuntimeCost);
	}

	// ================ 13. OnCompanionCount 被动 ================
	// AfterPlayed 之后触发：此时本次打出的 Companion 已计入 CompanionPlayedCount。
	FPassiveDispatcher::RunOnCompanionCount(State, Events);

	// ================ 14. 中毒结算 ================
	// 敌方部位行动之前：中毒可能破坏部位，从而影响随后的先机归零行动集合。
	FPoisonResolver::ResolvePoisonForAllHosts(State, Events);

	// ================ 15. 敌方部位行动子流程 ================
	if (!bSwift)
	{
		FEnemyPartActionResolver::ResolveInitiativeZeroActions(State, Events);
	}

	// ================ 16. 战斗结束判断 ================
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	++State.StateVersion;
	return FWacomStatus::Ok();
}
