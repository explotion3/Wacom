// Copyright Wacom. All Rights Reserved.

#include "Core/BattleResolver.h"
#include "Core/BattleState.h"
#include "Commands/BattleCommand.h"
#include "Commands/PlayCardResolver.h"
#include "Commands/WaitResolver.h"
#include "Commands/EndTurnResolver.h"

FWacomStatus FBattleResolver::Resolve(FBattleState& State, FBattleEventBus& Events, const FBattleCommand& Command)
{
	// 只允许在 PlayerAction 阶段提交命令。
	// Setup / TurnStart / TurnEnd / BattleEnd 阶段由 Session 内部状态机推进。
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
