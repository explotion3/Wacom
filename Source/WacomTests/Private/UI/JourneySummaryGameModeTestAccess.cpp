// Copyright Wacom. All Rights Reserved.

#include "UI/JourneySummaryGameModeTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "RunSession.h"
#include "UI/Menus/WacomJourneySummaryScreen.h"

bool FWacomJourneySummaryGameModeTestAccess::Consume(
	AWacomGameMode& GameMode,
	const FRunExplorationResolution& Resolution,
	const URunSession& RunSession)
{
	return GameMode.ConsumeJourneySucceededEvent(Resolution, RunSession);
}

void FWacomJourneySummaryGameModeTestAccess::CompleteSuccessBarrier(
	AWacomGameMode& GameMode)
{
	GameMode.CurrentState = EGameFlowState::JourneySummary;
	GameMode.HandleExitBattlePostRunBarrier(true, nullptr);
}

void FWacomJourneySummaryGameModeTestAccess::BindScreen(
	AWacomGameMode& GameMode,
	UWacomJourneySummaryScreen& Screen)
{
	GameMode.CurrentState = EGameFlowState::JourneySummary;
	GameMode.BindJourneySummaryScreen(Screen);
}

void FWacomJourneySummaryGameModeTestAccess::RequestHandoff(AWacomGameMode& GameMode)
{
	GameMode.RequestJourneySummaryMainMenuHandoff();
}

void FWacomJourneySummaryGameModeTestAccess::UseNativeFallback(AWacomGameMode& GameMode)
{
	GameMode.JourneySummaryScreenClass = UWacomJourneySummaryScreen::StaticClass();
}

#endif
