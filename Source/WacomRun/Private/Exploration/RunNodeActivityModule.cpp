// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunNodeActivityModule.h"

#include "Exploration/RunMapModule.h"
#include "Exploration/RunTimeModule.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunState.h"

namespace
{
	bool SameTicket(const FRunNodeActivityTicket& A, const FRunNodeActivityTicket& B)
	{
		return A.Token == B.Token
			&& A.VersionBefore == B.VersionBefore
			&& A.Node == B.Node
			&& A.Kind == B.Kind
			&& A.ReservedActionPoints == B.ReservedActionPoints;
	}
}

FWacomStatus FRunNodeActivityModule::Begin(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& PendingActivity,
	const ERunNodeActivityKind Kind,
	const int32 ReservedActionPoints,
	FRunNodeActivityTicket& OutTicket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	OutTicket = {};
	if (PendingActivity.IsSet()
		|| State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
	}
	if (ReservedActionPoints < 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidActionPointReservation"));
	}
	if (State.TimeState.CurrentTimePhase == ETimePhase::Night
		&& State.TimeState.NightGate != ERunNightGate::ExplorationOpen)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NightChoiceRequired"));
	}
	if (State.TimeState.RemainingActionPoints < ReservedActionPoints)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InsufficientActionPoints"));
	}

	const UWacomFloorMapDefinition* Floor = FRunMapModule::FindCurrentFloor(State);
	const FWacomMapNodeDefinition* Node = Floor
		? Floor->FindNode(State.ExplorationState.CurrentNodeId)
		: nullptr;
	if (!Floor || !Node || Node->NodeType != ExpectedNodeType(Kind))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CurrentNodeActivityTypeMismatch"));
	}

	FRunNodeActivityTicket Ticket;
	Ticket.Token = FRunNodeActivityToken(FGuid::NewGuid());
	Ticket.VersionBefore = State.ExplorationState.ExplorationStateVersion;
	Ticket.Node = { Floor->FloorId, Node->NodeId };
	Ticket.Kind = Kind;
	Ticket.ReservedActionPoints = ReservedActionPoints;
	if (!Ticket.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NodeActivityTicketGenerationFailed"));
	}

	State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::NodeActivity;
	++State.ExplorationState.ExplorationStateVersion;
	PendingActivity = Ticket;
	OutTicket = Ticket;
	FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
	Event.Type = ERunExplorationEventType::NodeActivityStarted;
	Event.Node = Ticket.Node;
	return FWacomStatus::Ok();
}

FWacomStatus FRunNodeActivityModule::Complete(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& PendingActivity,
	const FRunNodeActivityTicket& Ticket,
	const int32 ActionPointCost,
	const bool bResolveNode,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (!Matches(State, PendingActivity, Ticket))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NodeActivityTicketMismatch"));
	}
	if (ActionPointCost < 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidActionPointCost"));
	}

	FRunState WorkingState = State;
	TOptional<FRunNodeActivityTicket> WorkingPending = PendingActivity;
	TArray<FRunExplorationEvent> WorkingEvents;
	WorkingState.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	if (ActionPointCost > 0)
	{
		const FWacomStatus SpendStatus = FRunTimeModule::TrySpendActionPoints(
			WorkingState,
			ActionPointCost,
			WorkingEvents);
		if (!SpendStatus.IsOk())
		{
			return SpendStatus;
		}
	}
	if (bResolveNode
		&& !FRunMapModule::ResolveNode(WorkingState, Ticket.Node, WorkingEvents))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NodeActivityResolveFailed"));
	}

	WorkingPending.Reset();
	++WorkingState.ExplorationState.ExplorationStateVersion;
	FRunExplorationEvent& Event = WorkingEvents.AddDefaulted_GetRef();
	Event.Type = ERunExplorationEventType::NodeActivityCompleted;
	Event.Node = Ticket.Node;
	State = MoveTemp(WorkingState);
	PendingActivity = MoveTemp(WorkingPending);
	OutEvents.Append(MoveTemp(WorkingEvents));
	return FWacomStatus::Ok();
}

FWacomStatus FRunNodeActivityModule::Cancel(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& PendingActivity,
	const FRunNodeActivityTicket& Ticket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (!Matches(State, PendingActivity, Ticket))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NodeActivityTicketMismatch"));
	}
	State.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	++State.ExplorationState.ExplorationStateVersion;
	PendingActivity.Reset();
	FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
	Event.Type = ERunExplorationEventType::NodeActivityCancelled;
	Event.Node = Ticket.Node;
	return FWacomStatus::Ok();
}

FWacomStatus FRunNodeActivityModule::SpendAndContinue(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& PendingActivity,
	const FRunNodeActivityTicket& Ticket,
	const int32 ActionPointCost,
	const bool bResolveNode,
	const bool bDeferPhaseAdvance,
	bool& bOutActivityContinues,
	FRunNodeActivityTicket& OutUpdatedTicket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	bOutActivityContinues = false;
	OutUpdatedTicket = {};
	if (!Matches(State, PendingActivity, Ticket))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NodeActivityTicketMismatch"));
	}
	if (ActionPointCost <= 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidActionPointCost"));
	}

	FRunState WorkingState = State;
	TOptional<FRunNodeActivityTicket> WorkingPending = PendingActivity;
	TArray<FRunExplorationEvent> WorkingEvents;
	const int32 DayBefore = WorkingState.TimeState.CurrentDayNumber;
	const ETimePhase PhaseBefore = WorkingState.TimeState.CurrentTimePhase;
	WorkingState.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	const FWacomStatus SpendStatus = bDeferPhaseAdvance
		? FRunTimeModule::TrySpendActionPointsDeferredAdvance(
			WorkingState,
			ActionPointCost,
			WorkingEvents)
		: FRunTimeModule::TrySpendActionPoints(
			WorkingState,
			ActionPointCost,
			WorkingEvents);
	if (!SpendStatus.IsOk())
	{
		return SpendStatus;
	}
	if (bResolveNode && !FRunMapModule::ResolveNode(WorkingState, Ticket.Node, WorkingEvents))
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NodeActivityResolveFailed"));
	}

	const bool bPhaseAdvanced =
		WorkingState.TimeState.CurrentDayNumber != DayBefore
		|| WorkingState.TimeState.CurrentTimePhase != PhaseBefore;
	++WorkingState.ExplorationState.ExplorationStateVersion;
	if (bPhaseAdvanced)
	{
		WorkingPending.Reset();
		FRunExplorationEvent& CompletionEvent = WorkingEvents.AddDefaulted_GetRef();
		CompletionEvent.Type = ERunExplorationEventType::NodeActivityCompleted;
		CompletionEvent.Node = Ticket.Node;
	}
	else
	{
		WorkingState.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::NodeActivity;
		FRunNodeActivityTicket UpdatedTicket = Ticket;
		UpdatedTicket.VersionBefore = WorkingState.ExplorationState.ExplorationStateVersion - 1;
		WorkingPending = UpdatedTicket;
		OutUpdatedTicket = UpdatedTicket;
		bOutActivityContinues = true;
	}

	State = MoveTemp(WorkingState);
	PendingActivity = MoveTemp(WorkingPending);
	OutEvents.Append(MoveTemp(WorkingEvents));
	return FWacomStatus::Ok();
}

FWacomStatus FRunNodeActivityModule::ResolveImmediate(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& PendingActivity,
	const ERunNodeActivityKind Kind,
	const int32 ActionPointCost,
	const bool bResolveNode,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (ActionPointCost < 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidActionPointCost"));
	}
	const int32 VersionBefore = State.ExplorationState.ExplorationStateVersion;
	FRunState WorkingState = State;
	TOptional<FRunNodeActivityTicket> WorkingPending = PendingActivity;
	TArray<FRunExplorationEvent> WorkingEvents;
	FRunNodeActivityTicket Ticket;
	const FWacomStatus BeginStatus = Begin(
		WorkingState,
		WorkingPending,
		Kind,
		ActionPointCost,
		Ticket,
		WorkingEvents);
	if (!BeginStatus.IsOk())
	{
		return BeginStatus;
	}
	const FWacomStatus CompleteStatus = Complete(
		WorkingState,
		WorkingPending,
		Ticket,
		ActionPointCost,
		bResolveNode,
		WorkingEvents);
	if (!CompleteStatus.IsOk())
	{
		return CompleteStatus;
	}
	WorkingState.ExplorationState.ExplorationStateVersion = VersionBefore + 1;
	State = MoveTemp(WorkingState);
	PendingActivity = MoveTemp(WorkingPending);
	OutEvents.Append(MoveTemp(WorkingEvents));
	return FWacomStatus::Ok();
}

bool FRunNodeActivityModule::Matches(
	const FRunState& State,
	const TOptional<FRunNodeActivityTicket>& PendingActivity,
	const FRunNodeActivityTicket& Ticket)
{
	return Ticket.IsValid()
		&& PendingActivity.IsSet()
		&& State.ExplorationState.ActiveActivityKind == ERunExplorationActivityKind::NodeActivity
		&& State.ExplorationState.ExplorationStateVersion == Ticket.VersionBefore + 1
		&& SameTicket(PendingActivity.GetValue(), Ticket);
}

EWacomMapNodeType FRunNodeActivityModule::ExpectedNodeType(const ERunNodeActivityKind Kind)
{
	switch (Kind)
	{
	case ERunNodeActivityKind::Encounter: return EWacomMapNodeType::Encounter;
	case ERunNodeActivityKind::RunEvent: return EWacomMapNodeType::RunEvent;
	case ERunNodeActivityKind::Shop: return EWacomMapNodeType::Shop;
	case ERunNodeActivityKind::Treasure: return EWacomMapNodeType::Treasure;
	default: return EWacomMapNodeType::Navigation;
	}
}
