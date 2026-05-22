// Copyright Wacom. All Rights Reserved.

#include "Time/RunTimeRules.h"

#include "RunState.h"
#include "RunStateTypes.h"

bool FRunTimeRules::ConsumeNode(FRunState& State, int32 Count, int32* OutConsumedNodeCount)
{
	if (OutConsumedNodeCount)
	{
		*OutConsumedNodeCount = 0;
	}

	if (Count <= 0)
	{
		return true;
	}

	const int32 NodesBefore = State.RemainingNodeCount;
	const bool bEnough = NodesBefore >= Count;
	const int32 ConsumedNodeCount = FMath::Min(Count, FMath::Max(0, NodesBefore));
	State.RemainingNodeCount = FMath::Max(0, NodesBefore - Count);

	if (OutConsumedNodeCount)
	{
		*OutConsumedNodeCount = ConsumedNodeCount;
	}

	if (State.RemainingNodeCount <= 0)
	{
		AdvanceToNextPhase(State);
	}

	return bEnough;
}

void FRunTimeRules::AdvanceToNextPhase(FRunState& State)
{
	const ETimePhase PrevPhase = State.CurrentTimePhase;

	switch (State.CurrentTimePhase)
	{
	case ETimePhase::Morning: State.CurrentTimePhase = ETimePhase::Day;     break;
	case ETimePhase::Day:     State.CurrentTimePhase = ETimePhase::Dusk;    break;
	case ETimePhase::Dusk:    State.CurrentTimePhase = ETimePhase::Night;   break;
	case ETimePhase::Night:   State.CurrentTimePhase = ETimePhase::Sunrise; break;
	case ETimePhase::Sunrise:
		State.CurrentTimePhase = ETimePhase::Morning;
		++State.CurrentDayNumber;
		break;
	default:
		ensureMsgf(false, TEXT("[RunTimeRules] AdvanceToNextPhase 收到未知时段 %d"),
			(int32)State.CurrentTimePhase);
		State.CurrentTimePhase = ETimePhase::Morning;
		break;
	}

	ResetRemainingNodeForPhase(State);

	UE_LOG(LogTemp, Display,
		TEXT("[RunTimeRules] Phase advanced: Day=%d Phase=%d RemainingNodes=%d"),
		State.CurrentDayNumber, (int32)State.CurrentTimePhase, State.RemainingNodeCount);

	ApplyPhaseEntryEffects(State, State.CurrentTimePhase, PrevPhase);
}

void FRunTimeRules::ResetRemainingNodeForPhase(FRunState& State)
{
	switch (State.CurrentTimePhase)
	{
	case ETimePhase::Morning: State.RemainingNodeCount = State.InitialNodeCount_Morning; break;
	case ETimePhase::Day:     State.RemainingNodeCount = State.InitialNodeCount_Day;     break;
	case ETimePhase::Dusk:    State.RemainingNodeCount = State.InitialNodeCount_Dusk;    break;
	case ETimePhase::Night:   State.RemainingNodeCount = State.InitialNodeCount_Night;   break;
	case ETimePhase::Sunrise: State.RemainingNodeCount = State.InitialNodeCount_Sunrise; break;
	default:
		State.RemainingNodeCount = 0;
		break;
	}
}

void FRunTimeRules::ApplyPhaseEntryEffects(FRunState& State, ETimePhase NewPhase, ETimePhase PrevPhase)
{
	switch (NewPhase)
	{
	case ETimePhase::Morning:
		State.Pressure.Add(EWacomPressureType::Hunger, 5);
		if (PrevPhase == ETimePhase::Sunrise)
		{
			State.Pressure.Add(EWacomPressureType::Decay, 5);
		}
		break;
	case ETimePhase::Dusk:
		State.Pressure.Add(EWacomPressureType::Hunger, 5);
		break;
	case ETimePhase::Sunrise:
		State.Pressure.Add(EWacomPressureType::Fatigue, 10);
		break;
	case ETimePhase::Day:
	case ETimePhase::Night:
	default:
		break;
	}
}
