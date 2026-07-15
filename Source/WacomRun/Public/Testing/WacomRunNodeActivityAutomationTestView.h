// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Exploration/RunExplorationTypes.h"
#include "RunState.h"
#include "Types/WacomResult.h"

/** Narrow automation seam for the private generic NodeActivity module. */
struct WACOMRUN_API FWacomRunNodeActivityAutomationTestView
{
	static FWacomStatus Begin(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& Pending,
		ERunNodeActivityKind Kind,
		int32 ReservedActionPoints,
		FRunNodeActivityTicket& OutTicket,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus Complete(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& Pending,
		const FRunNodeActivityTicket& Ticket,
		bool bCommitReservation,
		bool bResolveNode,
		TArray<FRunExplorationEvent>& OutEvents);
	static FWacomStatus Cancel(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& Pending,
		const FRunNodeActivityTicket& Ticket,
		TArray<FRunExplorationEvent>& OutEvents);
};

#endif
