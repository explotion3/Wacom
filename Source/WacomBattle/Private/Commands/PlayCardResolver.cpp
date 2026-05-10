// Copyright Wacom. All Rights Reserved.

#include "Commands/PlayCardResolver.h"
#include "Commands/BattleCommand.h"
#include "Core/BattleState.h"

FWacomStatus FPlayCardResolver::Resolve(FBattleState& /*State*/, FBattleEventBus& /*Events*/, const FBattleCommand& Command)
{
	if (!Command.CardInstanceId.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoCardInstanceId"));
	}

	// 具体流程在 S5 实现：
	// - 卡牌基础合法性
	// - 特殊条件
	// - 目标枚举与修正
	// - 费用枚举与合法性
	// - 效果结算（S7/S8）
	// - 先机推进（S8）
	// - 敌方部位行动子流程（S6）
	// - 卡牌去向
	return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotImplemented"));
}
