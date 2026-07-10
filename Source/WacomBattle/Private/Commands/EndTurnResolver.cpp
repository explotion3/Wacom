// Copyright Wacom. All Rights Reserved.

#include "Commands/EndTurnResolver.h"
#include "Turns/BattleTurnLifecycleModule.h"

FWacomStatus FEndTurnResolver::Resolve(
	FBattleState& State,
	FBattleEventBus& Events,
	FBattlePresentationJournal& PresentationJournal,
	const FBattleCommand& /*Command*/)
{
	FBattleTurnLifecycleModule::CompleteCurrentTurn(State, Events, PresentationJournal);
	return FWacomStatus::Ok();
}
