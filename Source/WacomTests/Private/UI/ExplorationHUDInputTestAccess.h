// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UWacomExplorationHUD;

struct FWacomExplorationHUDInputTestAccess
{
	static bool IsGameViewportFocusPending(const UWacomExplorationHUD& HUD);
};

#endif
