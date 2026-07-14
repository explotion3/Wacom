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
	static void Construct(UWacomMainMenuButtonWidget& Button);
	static void Destruct(UWacomMainMenuButtonWidget& Button);
	static void BroadcastFocusReceived(UWacomMainMenuButtonWidget& Button);
	static void BroadcastFocusLost(UWacomMainMenuButtonWidget& Button);
	static FReply SendEscapeKeyDown(UWacomMainMenuScreen& Screen);
	static float TargetEmphasis(const UWacomMainMenuButtonWidget& Button);
	static void Click(UWacomMainMenuScreen& Screen, EWacomMainMenuAction Action);
	static bool HasCompleteFocusableCommonUIButtonSet(const UWacomMainMenuScreen& Screen);
	static bool UsesAuthoredMainMenuWidgets(
		const UWacomMainMenuScreen& Screen,
		const UClass* ExpectedButtonClass);
	static TArray<FName> WidgetNames(const UWacomMainMenuScreen& Screen);
	static FWacomMainMenuScreenAutomationTestView View(const UWacomMainMenuScreen& Screen);
};

#endif
