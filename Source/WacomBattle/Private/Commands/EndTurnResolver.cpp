// Copyright Wacom. All Rights Reserved.

#include "Commands/EndTurnResolver.h"
#include "Commands/BattleCommand.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Core/BattleTurnFlow.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"

FWacomStatus FEndTurnResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& /*Command*/)
{
	// Battle_Rules §12：
	// 1. 切到 TurnEnd 阶段
	// 2. TODO(S7)：结算"回合结束时"类效果、"直到回合结束"类效果终止
	// 3. 执行敌方部位行动子流程（所有存活可行动部位按部位顺序行动）
	// 4. 战斗结束判断
	// 5. 若未结束，推进到下一回合（调用 BeginPlayerTurn）

	State.Phase = EBattlePhase::TurnEnd;

	{
		FBattleEvent Ev;
		Ev.Type = EBattleEventType::TurnEnded;
		Ev.Count = State.TurnNumber;
		Events.Emit(Ev);
	}

	// 敌方部位行动（S6 填实现）
	FEnemyPartActionResolver::ResolveEndTurnActions(State, Events);

	// 战斗结束判断
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	// 推进到下一回合
	++State.TurnNumber;
	FBattleTurnFlow::BeginPlayerTurn(State, Events, /*bIsFirstTurn=*/false);

	return FWacomStatus::Ok();
}
