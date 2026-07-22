// Copyright Wacom. All Rights Reserved.

#include "UI/WacomBattleEnemyPanelWidgetTestAccess.h"

#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

void FWacomBattleEnemyPanelWidgetTestAccess::ApplyRuntimeSettings(
	UWacomBattleEnemyPanelWidget& Widget,
	const FWacomLocalSettingsSnapshot& Snapshot)
{
	Widget.HandleRuntimeSettingsChanged(
		Snapshot,
		EWacomRuntimeSettingsChangeReason::Applied);
}

bool FWacomBattleEnemyPanelWidgetTestAccess::HasRuntimeSettingsSubscription(
	const UWacomBattleEnemyPanelWidget& Widget)
{
	return Widget.BoundSettingsSubsystem.IsValid()
		&& Widget.RuntimeSettingsChangedHandle.IsValid();
}

void FWacomBattleEnemyPanelWidgetTestAccess::Destruct(
	UWacomBattleEnemyPanelWidget& Widget)
{
	Widget.NativeDestruct();
}
