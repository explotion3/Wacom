// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunCampActivity.h"
#include "Exploration/RunExplorationTypes.h"
#include "Types/WacomResult.h"

struct FRunState;

/** Camp 地点选择、预留和 Night->Morning 提交的私有深层模块。 */
class FRunCampModule
{
public:
	static bool FindNearestCampNode(const FRunState& State, FWacomMapNodeHandle& OutNode);

	static FWacomStatus Begin(
		FRunState& State,
		TOptional<FRunCampTicket>& PendingCamp,
		FRunCampTicket& OutTicket,
		TArray<FRunExplorationEvent>& OutEvents);

	static FWacomStatus Cancel(
		FRunState& State,
		TOptional<FRunCampTicket>& PendingCamp,
		const FRunCampTicket& Ticket,
		TArray<FRunExplorationEvent>& OutEvents);

	static FWacomStatus Complete(
		FRunState& State,
		TOptional<FRunCampTicket>& PendingCamp,
		const FRunCampTicket& Ticket,
		const IRunCampActivityHandler& Handler,
		TArray<FRunExplorationEvent>& OutEvents);

	static bool Matches(
		const FRunState& State,
		const TOptional<FRunCampTicket>& PendingCamp,
		const FRunCampTicket& Ticket);
};
