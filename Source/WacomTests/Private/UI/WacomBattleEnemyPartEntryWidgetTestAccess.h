// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Settings/WacomLocalSettingsTypes.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

class UMaterialInstanceDynamic;

/** WacomTests-private access to the non-reflected Enemy HUD presentation state. */
struct FWacomBattleEnemyPartEntryWidgetTestAccess
{
	static void SetView(
		UWacomBattleEnemyPartEntryWidget& Widget,
		const FWacomBattleEnemyPartEntryViewData& View);
	static void SetPreview(
		UWacomBattleEnemyPartEntryWidget& Widget,
		const FWacomBattleEnemyPartEntryViewData& View);
	static void ClearPreview(UWacomBattleEnemyPartEntryWidget& Widget);
	static bool HasPreview(const UWacomBattleEnemyPartEntryWidget& Widget);
	static const FWacomBattleEnemyPartEntryViewData& GetCurrentView(
		const UWacomBattleEnemyPartEntryWidget& Widget);
	static void CancelPresentation(UWacomBattleEnemyPartEntryWidget& Widget);
	static bool IsInspectionInteractionEnabled(
		const UWacomBattleEnemyPartEntryWidget& Widget);

	static EWacomBattleEnemySegmentRole GetSegmentRole(
		const UWacomBattleEnemyPartEntryWidget& Widget);
	static int32 GetSegmentCount(const UWacomBattleEnemyPartEntryWidget& Widget);
	static float GetMaterialScalar(
		const UWacomBattleEnemyPartEntryWidget& Widget,
		FName ParameterName);
	static UMaterialInstanceDynamic* GetVitalsMaterial(
		const UWacomBattleEnemyPartEntryWidget& Widget);
	static float GetDamageTrailStartPercent(
		const UWacomBattleEnemyPartEntryWidget& Widget);
	static float GetRuntimeFlashIntensity(
		const UWacomBattleEnemyPartEntryWidget& Widget);
	static bool IsUsingSimplifiedMotion(
		const UWacomBattleEnemyPartEntryWidget& Widget);
	static void ApplyRuntimeSettings(
		UWacomBattleEnemyPartEntryWidget& Widget,
		const FWacomLocalSettingsSnapshot& Snapshot);
};
