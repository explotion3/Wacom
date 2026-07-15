// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunTimeAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Exploration/RunFloorExposureModule.h"
#include "Exploration/RunTimeModule.h"

FWacomStatus FWacomRunTimeAutomationTestView::TrySpendActionPoints(
	FRunState& State, const int32 Cost, TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunTimeModule::TrySpendActionPoints(State, Cost, OutEvents);
}

FWacomStatus FWacomRunTimeAutomationTestView::AdvanceToNextPhase(
	FRunState& State, TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunTimeModule::AdvanceToNextPhase(State, OutEvents);
}

FWacomStatus FWacomRunTimeAutomationTestView::ChooseNightExploration(
	FRunState& State, TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunTimeModule::ChooseNightExploration(State, OutEvents);
}

FWacomStatus FWacomRunTimeAutomationTestView::CompleteCampAndAdvanceToMorning(
	FRunState& State, TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunTimeModule::CompleteCampAndAdvanceToMorning(State, OutEvents);
}

FWacomStatus FWacomRunTimeAutomationTestView::ApplyDailyDecay(
	FRunState& State, TArray<FRunExplorationEvent>& OutEvents)
{
	return FRunFloorExposureModule::ApplyDailyDecay(State, OutEvents);
}

#endif
