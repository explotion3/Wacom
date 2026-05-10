// Copyright Wacom. All Rights Reserved.

#include "Commands/EndTurnResolver.h"
#include "Core/BattleState.h"

FWacomStatus FEndTurnResolver::Resolve(FBattleState& /*State*/, FBattleEventBus& /*Events*/, const FBattleCommand& /*Command*/)
{
	// S5/S6 实现：
	// - 切到 TurnEnd 阶段
	// - 结算回合结束时效果
	// - 调用敌方部位行动子流程
	// - 判断战斗结束
	// - 若未结束则回到 TurnStart
	return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotImplemented"));
}
