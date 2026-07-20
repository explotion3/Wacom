// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Materials/MaterialInstanceDynamic.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

/** WacomTests-private access to the non-reflected Enemy HUD presentation state. */
struct FWacomBattleEnemyPartEntryWidgetTestAccess
{
	static EWacomBattleEnemySegmentRole GetSegmentRole(
		const UWacomBattleEnemyPartEntryWidget& Widget)
	{
		return Widget.SegmentRole;
	}

	static int32 GetSegmentCount(const UWacomBattleEnemyPartEntryWidget& Widget)
	{
		return Widget.SegmentCount;
	}

	static float GetMaterialScalar(
		const UWacomBattleEnemyPartEntryWidget& Widget,
		const FName ParameterName)
	{
		float Value = 0.0f;
		return Widget.VitalsMaterialInstance
			&& Widget.VitalsMaterialInstance->GetScalarParameterValue(ParameterName, Value)
			? Value
			: TNumericLimits<float>::Lowest();
	}

	static UMaterialInstanceDynamic* GetVitalsMaterial(
		const UWacomBattleEnemyPartEntryWidget& Widget)
	{
		return Widget.VitalsMaterialInstance;
	}

	static float GetDamageTrailStartPercent(
		const UWacomBattleEnemyPartEntryWidget& Widget)
	{
		return Widget.DamageTrailStartPercent;
	}

	static float GetRuntimeFlashIntensity(
		const UWacomBattleEnemyPartEntryWidget& Widget)
	{
		return Widget.RuntimeFlashIntensity;
	}

	static bool IsUsingSimplifiedMotion(
		const UWacomBattleEnemyPartEntryWidget& Widget)
	{
		return Widget.bRuntimeSimplifiedMotion;
	}

	static void ApplyRuntimeSettings(
		UWacomBattleEnemyPartEntryWidget& Widget,
		const FWacomLocalSettingsSnapshot& Snapshot)
	{
		Widget.HandleRuntimeSettingsChanged(
			Snapshot,
			EWacomRuntimeSettingsChangeReason::Applied);
	}
};
