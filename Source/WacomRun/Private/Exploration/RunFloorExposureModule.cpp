// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunFloorExposureModule.h"

#include "Exploration/RunExplorationTypes.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunState.h"

FWacomStatus FRunFloorExposureModule::ApplyDailyDecay(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	const UWacomJourneyDefinition* Journey = State.ExplorationState.JourneyDefinition;
	if (!Journey)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("MissingJourneyDefinition"));
	}

	const int32 JourneyDay = State.TimeState.CurrentDayNumber;
	if (JourneyDay <= 0 || State.ExplorationState.FloorEnteredDayNumber <= 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InvalidFloorExposureDay"));
	}
	if (JourneyDay <= State.ExplorationState.LastDailyDecayAppliedDayNumber)
	{
		return FWacomStatus::Ok();
	}

	const int32 FloorDay = FMath::Max(
		1,
		JourneyDay - State.ExplorationState.FloorEnteredDayNumber + 1);
	const int32 DailyDecay =
		Journey->EvaluateBaseDecay(JourneyDay) + Journey->EvaluateOverstayDecay(FloorDay);
	if (DailyDecay > 0)
	{
		State.Pressure.Add(EWacomPressureType::Decay, DailyDecay);
		FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::PressureChanged;
		Event.Detail = TEXT("DailyDecay");
	}
	State.ExplorationState.LastDailyDecayAppliedDayNumber = JourneyDay;
	return FWacomStatus::Ok();
}
