// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunNodeActivityAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Exploration/RunNodeActivityModule.h"

FWacomStatus FWacomRunNodeActivityAutomationTestView::Begin(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& Pending,
	const ERunNodeActivityKind Kind,
	const int32 ReservedActionPoints,
	FRunNodeActivityTicket& OutTicket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunNodeActivityModule::Begin(
		State, Pending, Kind, ReservedActionPoints, OutTicket, OutEvents);
}

FWacomStatus FWacomRunNodeActivityAutomationTestView::Complete(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& Pending,
	const FRunNodeActivityTicket& Ticket,
	const bool bCommitReservation,
	const bool bResolveNode,
	TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunNodeActivityModule::Complete(
		State, Pending, Ticket, bCommitReservation, bResolveNode, OutEvents);
}

FWacomStatus FWacomRunNodeActivityAutomationTestView::Cancel(
	FRunState& State,
	TOptional<FRunNodeActivityTicket>& Pending,
	const FRunNodeActivityTicket& Ticket,
	TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunNodeActivityModule::Cancel(State, Pending, Ticket, OutEvents);
}

#endif
