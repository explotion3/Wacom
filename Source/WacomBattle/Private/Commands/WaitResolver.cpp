// Copyright Wacom. All Rights Reserved.

#include "Commands/WaitResolver.h"
#include "Core/BattleState.h"

FWacomStatus FWaitResolver::Resolve(FBattleState& /*State*/, FBattleEventBus& /*Events*/, const FBattleCommand& /*Command*/)
{
	// S5 实现：
	// - 所有敌方部位当前先机减去当前等待值
	// - 若有部位先机 <= 0，调用敌方部位行动子流程（S6）
	// - 当前等待值 +1
	// - 发射 WaitPerformed 事件
	return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotImplemented"));
}
