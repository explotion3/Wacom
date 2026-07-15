// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunTimeModule.h"

#include "Exploration/RunExplorationTypes.h"
#include "Exploration/RunFloorExposureModule.h"
#include "RunState.h"

namespace
{
	int32 BudgetForPhase(const FRunTimeState& Time, const ETimePhase Phase)
	{
		switch (Phase)
		{
		case ETimePhase::Morning: return Time.PhaseBudgets.Morning;
		case ETimePhase::Day: return Time.PhaseBudgets.Day;
		case ETimePhase::Dusk: return Time.PhaseBudgets.Dusk;
		case ETimePhase::Night: return Time.PhaseBudgets.Night;
		case ETimePhase::Sunrise: return Time.PhaseBudgets.Sunrise;
		default: return INDEX_NONE;
		}
	}

	void AddTimeEvent(TArray<FRunExplorationEvent>& Events, const FName Detail)
	{
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::TimeAdvanced;
		Event.Detail = Detail;
	}

	void AddPressureEvent(TArray<FRunExplorationEvent>& Events, const FName Detail)
	{
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::PressureChanged;
		Event.Detail = Detail;
	}

	FName PhaseDetail(const ETimePhase Phase)
	{
		switch (Phase)
		{
		case ETimePhase::Morning: return TEXT("Morning");
		case ETimePhase::Day: return TEXT("Day");
		case ETimePhase::Dusk: return TEXT("Dusk");
		case ETimePhase::Night: return TEXT("Night");
		case ETimePhase::Sunrise: return TEXT("Sunrise");
		default: return TEXT("InvalidPhase");
		}
	}

	void CommitWorkingState(
		FRunState& State,
		FRunState&& WorkingState,
		TArray<FRunExplorationEvent>& OutEvents,
		TArray<FRunExplorationEvent>&& WorkingEvents)
	{
		State = MoveTemp(WorkingState);
		OutEvents.Append(MoveTemp(WorkingEvents));
	}
}

FWacomStatus FRunTimeModule::TrySpendActionPoints(
	FRunState& State,
	const int32 Cost,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (Cost <= 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("InvalidActionPointCost"));
	}
	if (State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("ExplorationActivityAlreadyActive"));
	}
	if (State.TimeState.CurrentTimePhase == ETimePhase::Night
		&& State.TimeState.NightGate != ERunNightGate::ExplorationOpen)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NightChoiceRequired"));
	}
	if (State.TimeState.RemainingActionPoints < Cost)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InsufficientActionPoints"));
	}

	FRunState WorkingState = State;
	TArray<FRunExplorationEvent> WorkingEvents;
	WorkingState.TimeState.RemainingActionPoints -= Cost;
	if (WorkingState.TimeState.RemainingActionPoints == 0)
	{
		const FWacomStatus AdvanceStatus = AdvanceWorkingState(WorkingState, WorkingEvents);
		if (!AdvanceStatus.IsOk())
		{
			return AdvanceStatus;
		}
	}
	CommitWorkingState(State, MoveTemp(WorkingState), OutEvents, MoveTemp(WorkingEvents));
	return FWacomStatus::Ok();
}

FWacomStatus FRunTimeModule::AdvanceToNextPhase(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	FRunState WorkingState = State;
	TArray<FRunExplorationEvent> WorkingEvents;
	const FWacomStatus Status = AdvanceWorkingState(WorkingState, WorkingEvents);
	if (!Status.IsOk())
	{
		return Status;
	}
	CommitWorkingState(State, MoveTemp(WorkingState), OutEvents, MoveTemp(WorkingEvents));
	return FWacomStatus::Ok();
}

FWacomStatus FRunTimeModule::ChooseNightExploration(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None
		|| State.TimeState.CurrentTimePhase != ETimePhase::Night
		|| State.TimeState.NightGate != ERunNightGate::AwaitingChoice
		|| State.TimeState.RemainingActionPoints <= 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NightExplorationUnavailable"));
	}

	State.TimeState.NightGate = ERunNightGate::ExplorationOpen;
	AddTimeEvent(OutEvents, TEXT("NightExplorationOpened"));
	return FWacomStatus::Ok();
}

FWacomStatus FRunTimeModule::CompleteCampAndAdvanceToMorning(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if ((State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None
			&& State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::Camp)
		|| State.TimeState.CurrentTimePhase != ETimePhase::Night
		|| State.TimeState.NightGate != ERunNightGate::AwaitingChoice
		|| State.TimeState.RemainingActionPoints < 1)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("CampAdvanceUnavailable"));
	}

	FRunState WorkingState = State;
	TArray<FRunExplorationEvent> WorkingEvents;
	WorkingState.TimeState.RemainingActionPoints = 0;
	WorkingState.ExplorationState.ActiveActivityKind = ERunExplorationActivityKind::None;
	const FWacomStatus MorningStatus = EnterNewMorning(WorkingState, WorkingEvents);
	if (!MorningStatus.IsOk())
	{
		return MorningStatus;
	}
	FRunExplorationEvent& CampEvent = WorkingEvents.AddDefaulted_GetRef();
	CampEvent.Type = ERunExplorationEventType::CampCompleted;
	CampEvent.Node = {
		WorkingState.ExplorationState.CurrentFloorId,
		WorkingState.ExplorationState.CurrentNodeId };
	CommitWorkingState(State, MoveTemp(WorkingState), OutEvents, MoveTemp(WorkingEvents));
	return FWacomStatus::Ok();
}

FWacomStatus FRunTimeModule::AdvanceWorkingState(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	ETimePhase NextPhase = ETimePhase::Count;
	switch (State.TimeState.CurrentTimePhase)
	{
	case ETimePhase::Morning: NextPhase = ETimePhase::Day; break;
	case ETimePhase::Day: NextPhase = ETimePhase::Dusk; break;
	case ETimePhase::Dusk: NextPhase = ETimePhase::Night; break;
	case ETimePhase::Night:
		if (State.TimeState.NightGate != ERunNightGate::ExplorationOpen)
		{
			return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NightChoiceRequired"));
		}
		NextPhase = ETimePhase::Sunrise;
		break;
	case ETimePhase::Sunrise:
		return EnterNewMorning(State, OutEvents);
	default:
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InvalidTimePhase"));
	}

	const int32 Budget = BudgetForPhase(State.TimeState, NextPhase);
	if (Budget < 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("InvalidPhaseBudget"));
	}
	State.TimeState.CurrentTimePhase = NextPhase;
	State.TimeState.RemainingActionPoints = Budget;
	State.TimeState.NightGate = NextPhase == ETimePhase::Night
		? ERunNightGate::AwaitingChoice
		: ERunNightGate::Closed;
	AddTimeEvent(OutEvents, PhaseDetail(NextPhase));

	if (NextPhase == ETimePhase::Dusk)
	{
		State.Pressure.Add(EWacomPressureType::Hunger, 5);
		AddPressureEvent(OutEvents, TEXT("DuskHunger"));
	}
	else if (NextPhase == ETimePhase::Sunrise)
	{
		State.Pressure.Add(EWacomPressureType::Fatigue, 10);
		AddPressureEvent(OutEvents, TEXT("SunriseFatigue"));
	}
	return FWacomStatus::Ok();
}

FWacomStatus FRunTimeModule::EnterNewMorning(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	const int32 MorningBudget = BudgetForPhase(State.TimeState, ETimePhase::Morning);
	if (MorningBudget < 1)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("MorningPlanningBudgetUnavailable"));
	}
	++State.TimeState.CurrentDayNumber;
	State.TimeState.CurrentTimePhase = ETimePhase::Morning;
	State.TimeState.RemainingActionPoints = MorningBudget - 1;
	State.TimeState.NightGate = ERunNightGate::Closed;
	AddTimeEvent(OutEvents, TEXT("Morning"));
	State.Pressure.Add(EWacomPressureType::Hunger, 5);
	AddPressureEvent(OutEvents, TEXT("MorningHunger"));
	return FRunFloorExposureModule::ApplyDailyDecay(State, OutEvents);
}
