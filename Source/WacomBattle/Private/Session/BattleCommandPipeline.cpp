// Copyright Wacom. All Rights Reserved.

#include "Session/BattleCommandPipeline.h"

#include "Commands/BattleCommand.h"
#include "Commands/KnockdownFlowService.h"
#include "Core/BattleResolver.h"
#include "Core/BattleState.h"

FWacomStatus FBattleCommandPipeline::Submit(
	FBattleState& State,
	FBattleEventBus& Events,
	FBattlePresentationJournal& PresentationJournal,
	const FBattleCommand& Command)
{
	if (State.Phase == EBattlePhase::BattleEnd)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("BattleEnded"));
	}

	const FWacomStatus Status = FBattleResolver::Resolve(State, Events, PresentationJournal, Command);

	if (Status.IsOk()
		&& State.Phase != EBattlePhase::BattleEnd
		&& State.Phase != EBattlePhase::PendingKnockdownChoice
		&& State.PendingKnockdownEvents.Num() > 0)
	{
		State.Phase = EBattlePhase::PendingKnockdownChoice;
		FKnockdownFlowService::RequestCurrentChoiceIfPending(State, Events);
	}

	return Status;
}
