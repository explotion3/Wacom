// Copyright Wacom. All Rights Reserved.

#include "Core/BattleResolver.h"
#include "Core/BattleState.h"
#include "Commands/BattleCommand.h"
#include "Commands/PlayCardResolver.h"
#include "Commands/WaitResolver.h"
#include "Commands/EndTurnResolver.h"
#include "Commands/KnockdownChoiceResolver.h"

FWacomStatus FBattleResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command)
{
	// KnockdownChoice 命令独立 Phase 受理（GDD §6 击倒事件）：仅在 PendingKnockdownChoice 阶段允许。
	if (Command.Type == EBattleCommandType::KnockdownChoice)
	{
		if (State.Phase != EBattlePhase::PendingKnockdownChoice)
		{
			return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotPendingKnockdown"));
		}
		return FKnockdownChoiceResolver::Resolve(State, Events, Command);
	}

	// 其余命令仅在 PlayerAction 阶段。
	// Setup / TurnStart / TurnEnd / PendingKnockdownChoice / BattleEnd 阶段由 Session 内部状态机推进。
	if (State.Phase != EBattlePhase::PlayerAction)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NotPlayerAction"));
	}

	switch (Command.Type)
	{
	case EBattleCommandType::PlayCard:
		return FPlayCardResolver::Resolve(State, Events, Command);

	case EBattleCommandType::Wait:
		return FWaitResolver::Resolve(State, Events, Command);

	case EBattleCommandType::EndTurn:
		return FEndTurnResolver::Resolve(State, Events, Command);

	default:
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("UnknownCommand"));
	}
}
