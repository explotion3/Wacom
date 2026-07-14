// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Settings/WacomSettingsOptionRow.h"
#include "UI/Settings/WacomSettingsScreen.h"

class UWacomSettingsSubsystem;

struct FWacomSettingsScreenClassView
{
	UClass* ButtonClass = nullptr;
	UClass* RowClass = nullptr;
	UClass* DialogClass = nullptr;
};

struct FWacomSettingsScreenTestAccess
{
	static void BuildAndConstruct(UWacomSettingsScreen& Screen);
	static void Destruct(UWacomSettingsScreen& Screen);
	static bool BeginWithSubsystem(
		UWacomSettingsScreen& Screen,
		UWacomSettingsSubsystem& Subsystem);
	static void SelectCategory(
		UWacomSettingsScreen& Screen,
		EWacomSettingsCategory Category);
	static void Step(
		UWacomSettingsScreen& Screen,
		EWacomSettingsField Field,
		int32 Direction);
	static void SetNormalized(
		UWacomSettingsScreen& Screen,
		EWacomSettingsField Field,
		float Value);
	static void Apply(UWacomSettingsScreen& Screen);
	static void RestoreDefaults(UWacomSettingsScreen& Screen);
	static FWacomSettingsOptionRowViewData Row(
		const UWacomSettingsScreen& Screen,
		EWacomSettingsField Field);
	static FWacomSettingsScreenAutomationTestView View(
		const UWacomSettingsScreen& Screen);
	static FWacomSettingsScreenClassView Classes(
		const UWacomSettingsScreen& Screen);
	static bool HasRequiredBindings(const UWacomSettingsScreen& Screen);
	static bool IsRestoreDefaultsFocused(const UWacomSettingsScreen& Screen);
};

#endif
