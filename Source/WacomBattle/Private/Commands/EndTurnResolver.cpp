// Copyright Wacom. All Rights Reserved.

#include "Commands/EndTurnResolver.h"
#include "Commands/BattleCommand.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Core/BattleTurnFlow.h"
#include "Deck/DeckService.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Events/BattleEventHelpers.h"
#include "Hand/HandZoneService.h"
#include "Hand/HandZoneMoveEventService.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Snapshots/BattleSnapshotBuilder.h"

namespace
{
	void RecordEndTurnCheckpoint(
		FBattlePresentationJournal& PresentationJournal,
		EBattlePresentationCheckpointType Type,
		const FBattleState& State,
		const TArray<FGuid>& CardInstanceIds,
		int32 FirstEventSequence,
		int32 LastEventSequence)
	{
		if (CardInstanceIds.IsEmpty())
		{
			return;
		}

		PresentationJournal.AddCheckpoint(
			Type,
			FBattleSnapshotBuilder::Build(State),
			CardInstanceIds,
			FirstEventSequence,
			LastEventSequence);
	}
}

FWacomStatus FEndTurnResolver::Resolve(
	FBattleState& State,
	FBattleEventBus& Events,
	FBattlePresentationJournal& PresentationJournal,
	const FBattleCommand& /*Command*/)
{
	// 回合结束流程：
	//   1. 结束阶段开始（Phase 切换 + TurnEnded 事件）
	//   2. 本回合使用牌堆自然进入弃牌堆
	//   3. 回合结束弃牌
	//   4. 结算"直到回合结束"类效果终止（当前无）
	//   5. 判断敌方全部或我方生命值归零（敌方行动前的 early-exit）
	//   6. 执行敌方部位行动子流程
	//   7. 判断玩家或敌人是否被击倒或被消灭
	//   8. 若战斗未结束，回到起始阶段
	//
	// 非保留普通卡弃牌当前放在敌方行动前；若规则改为敌方行动后，见 Docs/TechDebt.md。

	// ---- 1. 结束阶段开始 ----
	State.Phase = EBattlePhase::TurnEnd;
	{
		FBattleEvent Ev;
		Ev.Type  = EBattleEventType::TurnEnded;
		Ev.Count = State.TurnNumber;
		Events.Emit(Ev);
	}

	// ---- 2. 本回合使用牌堆自然进入弃牌堆 ----
	FDeckService::MovePlayedPileToDiscard(State);

	// ---- 3. 回合结束弃牌 ----
	TArray<FGuid> RetainedAtTurnEnd;
	FHandZoneService::CollectRetainedNormalCardsAtTurnEnd(State, RetainedAtTurnEnd);

	TArray<FGuid> DiscardedAtTurnEnd;
	const int32 DiscardFirstEventSequence = Events.GetNextSequence();
	FHandZoneService::DiscardNonRetainedNormalCardsAtTurnEnd(State, DiscardedAtTurnEnd);
	FHandZoneMoveEventService::ResolveDiscardedFromHand(
		State,
		Events,
		DiscardedAtTurnEnd,
		EHandCardZoneMoveReason::TurnEnd);
	RecordEndTurnCheckpoint(
		PresentationJournal,
		EBattlePresentationCheckpointType::TurnEndDiscardResolved,
		State,
		DiscardedAtTurnEnd,
		DiscardFirstEventSequence,
		Events.GetNextSequence() - 1);

	if (!RetainedAtTurnEnd.IsEmpty())
	{
		const int32 RetainFirstEventSequence = Events.GetNextSequence();
		WacomBattleEvents::EmitCardsRetained(Events, RetainedAtTurnEnd);
		RecordEndTurnCheckpoint(
			PresentationJournal,
			EBattlePresentationCheckpointType::TurnEndRetainResolved,
			State,
			RetainedAtTurnEnd,
			RetainFirstEventSequence,
			Events.GetNextSequence() - 1);
	}

	// ---- 4. "直到回合结束"类效果终止（当前无）----

	// ---- 5. 敌方行动前的战斗结束判断 ----
	// 防御性 early-exit：若"回合结束时"类效果改变了 HP（毒、流血、自伤等），
	// 可能使战斗在此时就应结束，不应再触发敌方行动。
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	// ---- 6. 敌方部位行动子流程 ----
	FEnemyPartActionResolver::ResolveEndTurnActions(State, Events);

	// ---- 7. 敌方行动后的战斗结束判断 ----
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	// ---- 8. 推进到下一回合 ----
	++State.TurnNumber;
	TArray<FGuid> DrawnCardIds;
	const int32 DrawFirstEventSequence = Events.GetNextSequence();
	FBattleTurnFlow::BeginPlayerTurn(State, Events, /*bIsFirstTurn=*/false, &DrawnCardIds);
	RecordEndTurnCheckpoint(
		PresentationJournal,
		EBattlePresentationCheckpointType::TurnStartDrawResolved,
		State,
		DrawnCardIds,
		DrawFirstEventSequence,
		Events.GetNextSequence() - 1);

	return FWacomStatus::Ok();
}
