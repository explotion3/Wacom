// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Settings/WacomLocalSettingsTypes.h"

class UWacomBattleEnemyPanelWidget;

struct FWacomBattleEnemyPanelWidgetTestAccess
{
	static void ApplyRuntimeSettings(
		UWacomBattleEnemyPanelWidget& Widget,
		const FWacomLocalSettingsSnapshot& Snapshot);
	static bool HasRuntimeSettingsSubscription(
		const UWacomBattleEnemyPanelWidget& Widget);
	static void Destruct(UWacomBattleEnemyPanelWidget& Widget);
};
