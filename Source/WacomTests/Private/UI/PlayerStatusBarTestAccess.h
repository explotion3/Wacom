// Copyright Wacom. All Rights Reserved.

#pragma once

#include "UI/Battle/PlayerStatusBar.h"

struct FWacomPlayerStatusBarTestAccess
{
	static FWacomPlayerStatusBarAutomationTestView View(const UPlayerStatusBar& Widget);
	static void Tick(UPlayerStatusBar& Widget, float DeltaSeconds);
	static void SetReducedMotion(UPlayerStatusBar& Widget, bool bReducedMotion);
	static void Destruct(UPlayerStatusBar& Widget);
};
