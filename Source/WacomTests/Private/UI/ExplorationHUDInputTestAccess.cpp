// Copyright Wacom. All Rights Reserved.

#include "UI/ExplorationHUDInputTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Foundation/WacomExplorationHUD.h"

bool FWacomExplorationHUDInputTestAccess::IsGameViewportFocusPending(
	const UWacomExplorationHUD& HUD)
{
	return HUD.bGameViewportFocusPending;
}

#endif
