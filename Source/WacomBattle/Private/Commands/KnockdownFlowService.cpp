// Copyright Wacom. All Rights Reserved.

#include "Commands/KnockdownFlowService.h"

#include "Commands/KnockdownChoiceAvailability.h"
#include "Core/BattleState.h"
#include "Events/BattleEventBus.h"

bool FKnockdownFlowService::RequestCurrentChoiceIfPending(FBattleState& State, FBattleEventBus& Events)
{
	if (State.PendingKnockdownEvents.Num() <= 0)
	{
		return false;
	}

	Events.Emit(BuildCurrentChoiceRequestedEvent(State));
	return true;
}

FBattleEvent FKnockdownFlowService::BuildCurrentChoiceRequestedEvent(const FBattleState& State)
{
	FBattleEvent Event;
	if (State.PendingKnockdownEvents.Num() <= 0)
	{
		return Event;
	}

	const FBattleState::FPendingKnockdownEvent& Head = State.PendingKnockdownEvents[0];
	Event.Type = EBattleEventType::KnockdownChoiceRequested;
	Event.ActorInstanceId = Head.PartInstanceId;
	Event.ActorEnemyPartKey = Head.Identity.ToEnemyPartKey();
	Event.Count = FKnockdownChoiceAvailability::BuildLegacyEventMask(
		FKnockdownChoiceAvailability::BuildView(State));
	return Event;
}
