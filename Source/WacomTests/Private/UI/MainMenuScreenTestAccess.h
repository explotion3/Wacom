// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Menus/WacomMainMenuScreen.h"

struct FWacomMainMenuScreenTestAccess
{
	static void Build(UWacomMainMenuScreen& Screen);
	static void Construct(UWacomMainMenuScreen& Screen);
	static void Destruct(UWacomMainMenuScreen& Screen);
	static void Click(UWacomMainMenuScreen& Screen, EWacomMainMenuAction Action);
	static FWacomMainMenuScreenAutomationTestView View(const UWacomMainMenuScreen& Screen);
};

#endif
