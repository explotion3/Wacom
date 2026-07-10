// Copyright Wacom. All Rights Reserved.

#include "Commands/WaitResolver.h"
#include "Commands/BattleCommand.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Enemy/EnemyPartActionResolver.h"
#include "Events/BattleEventBus.h"
#include "Initiative/BattleInitiativeTimelineModule.h"

FWacomStatus FWaitResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& /*Command*/)
{
	// 等待流程：
	// 1. 所有敌人部位当前先机减去当前等待值
	// 2. 若有部位先机 <= 0，执行敌方部位行动子流程
	// 3. 当前等待值 +1
	// 4. 返回执行阶段

	const int32 Amount = State.CurrentWaitValue;

	FBattleInitiativeTimelineModule::PushAllLiving(State, Amount);

	{
		FBattleEvent Ev;
		Ev.Type   = EBattleEventType::WaitPerformed;
		Ev.Amount = Amount;
		Events.Emit(Ev);
	}

	// 先机归零 -> 敌方部位行动子流程
	FEnemyPartActionResolver::ResolveInitiativeZeroActions(State, Events);

	// 等待值 +1
	++State.CurrentWaitValue;

	// 战斗是否因玩家受伤或意外结束
	if (FBattleRules::CheckAndApplyBattleEnd(State, Events))
	{
		return FWacomStatus::Ok();
	}

	++State.StateVersion;
	return FWacomStatus::Ok();
}
