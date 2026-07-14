// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Menus/WacomTitleScreen.h"

struct FWacomTitleScreenTestAccess
{
	static void Build(UWacomTitleScreen& Screen);
	static FReply SendKeyDown(UWacomTitleScreen& Screen, const FKey& Key);
	static FReply SendMouseButtonDown(UWacomTitleScreen& Screen, const FKey& Button);
	static bool HasRequiredPresentationBindings(const UWacomTitleScreen& Screen);
};

#endif
