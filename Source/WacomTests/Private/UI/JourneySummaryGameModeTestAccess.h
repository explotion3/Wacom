// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "GameFramework/WacomGameMode.h"

struct FWacomJourneySummaryGameModeTestAccess
{
	static bool Consume(
		AWacomGameMode& GameMode,
		const FRunExplorationResolution& Resolution,
		const URunSession& RunSession);
	static void CompleteSuccessBarrier(AWacomGameMode& GameMode);
	static void BindScreen(
		AWacomGameMode& GameMode,
		UWacomJourneySummaryScreen& Screen);
	static void RequestHandoff(AWacomGameMode& GameMode);
	static void UseNativeFallback(AWacomGameMode& GameMode);
};

#endif
