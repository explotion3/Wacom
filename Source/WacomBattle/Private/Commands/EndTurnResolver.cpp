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
	// 对齐 Battle_Rules §12 "结束阶段流程"：
	//   1. 结束阶段开始（Phase 切换 + TurnEnded 事件）
	//   2. 结算"回合结束时"类效果                       ← TODO：第一阶段无此类效果
	//   3. 结算"直到回合结束"类效果终止                  ← TODO：第一阶段无此类效果
	//   4. 判断敌方全部或我方生命值归零（敌方行动前的 early-exit）
	//   5. 执行敌方部位行动子流程
	//   6. 判断玩家或敌人是否被击倒或被消灭
	//   7. 若战斗未结束，回到起始阶段
	//
	// 额外实现：在步骤 4 之前（即"回合结束时"效果结算的时机）插入
	// 非保留普通卡进弃牌的处理。Hand_Zone_Rules §7 规定"回合结束时"
	// 非保留普通卡进弃牌，但未明确时序。Tech_Debt_And_Deferred.md 已标注。

	// ---- 1. 结束阶段开始 ----
	State.Phase = EBattlePhase::TurnEnd;
	{
		FBattleEvent Ev;
		Ev.Type  = EBattleEventType::TurnEnded;
		Ev.Count = State.TurnNumber;
		Events.Emit(Ev);
	}

	// ---- 2. "回合结束时"类效果（占位）----
	// 当前唯一落到此阶段的是 P3.2 的"非保留普通卡进弃牌"。
	TArray<FGuid> DiscardedAtTurnEnd;
	FHandZoneService::DiscardNonRetainedNormalCardsAtTurnEnd(State, DiscardedAtTurnEnd);
	if (!DiscardedAtTurnEnd.IsEmpty())
	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::HandZoneChanged;
		Events.Emit(Ev);
	}

	// ---- 3. "直到回合结束"类效果终止（占位，第一阶段无）----

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
