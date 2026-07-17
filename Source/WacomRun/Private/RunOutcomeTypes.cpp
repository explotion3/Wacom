// Copyright Wacom. All Rights Reserved.

#include "RunOutcomeTypes.h"

bool FRunCompletionSummary::IsValid() const
{
	return !JourneyId.IsNone()
		&& TerminalNode.IsValid()
		&& CompletionDay > 0
		&& EnteredFloorCount > 0
		&& TotalFloorCount > 0
		&& EnteredFloorCount <= TotalFloorCount
		&& ResolvedNodeCount >= 0
		&& TotalNodeCount > 0
		&& ResolvedNodeCount <= TotalNodeCount
		&& FinalPressure >= 0;
}
