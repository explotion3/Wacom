// Copyright Wacom. All Rights Reserved.

#include "Commands/EndTurnResolver.h"
#include "Commands/BattleCommand.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Core/BattleTurnFlow.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Hand/HandZoneService.h"

FWacomStatus FEndTurnResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& /*Command*/)
{
	// 回合结束流程：
	//   1. 结束阶段开始（Phase 切换 + TurnEnded 事件）
	//   2. 结算当前已接入的回合结束效果
	//   3. 结算"直到回合结束"类效果终止（当前无）
	//   4. 判断敌方全部或我方生命值归零（敌方行动前的 early-exit）
	//   5. 执行敌方部位行动子流程
	//   6. 判断玩家或敌人是否被击倒或被消灭
	//   7. 若战斗未结束，回到起始阶段
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

	// ---- 2. 回合结束弃牌 ----
	TArray<FGuid> DiscardedAtTurnEnd;
	FHandZoneService::DiscardNonRetainedNormalCardsAtTurnEnd(State, DiscardedAtTurnEnd);
	if (!DiscardedAtTurnEnd.IsEmpty())
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(Ev);
	}

	// ---- 3. "直到回合结束"类效果终止（当前无）----

	// ---- 4. 敌方行动前的战斗结束判断 ----
	// 防御性 early-exit：若"回合结束时"类效果改变了 HP（毒、流血、自伤等），
	// 可能使战斗在此时就应结束，不应再触发敌方行动。
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	// ---- 5. 敌方部位行动子流程 ----
	FEnemyPartActionResolver::ResolveEndTurnActions(State, Events);

	// ---- 6. 敌方行动后的战斗结束判断 ----
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	// ---- 7. 推进到下一回合 ----
	++State.TurnNumber;
	FBattleTurnFlow::BeginPlayerTurn(State, Events, /*bIsFirstTurn=*/false);

	return FWacomStatus::Ok();
}
