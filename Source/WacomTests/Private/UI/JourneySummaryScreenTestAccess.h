// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Menus/WacomJourneySummaryScreen.h"

struct FWacomJourneySummaryScreenTestAccess
{
	static void Build(UWacomJourneySummaryScreen& Screen);
	static void Construct(UWacomJourneySummaryScreen& Screen);
	static void Destruct(UWacomJourneySummaryScreen& Screen);
	static void ClickContinue(UWacomJourneySummaryScreen& Screen);
	static FReply SendEscapeKeyDown(UWacomJourneySummaryScreen& Screen);
	static TArray<FName> WidgetNames(const UWacomJourneySummaryScreen& Screen);
	static FWacomJourneySummaryScreenAutomationTestView View(
		const UWacomJourneySummaryScreen& Screen);
};

#endif
