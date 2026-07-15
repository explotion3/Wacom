// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS && WITH_EDITOR

#include "../BackpackScreenTestAccess.h"
#include "HAL/IConsoleManager.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UObject/StrongObjectPtr.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackPIEValidationState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackPIEValidationCommandsSpec,
	"Wacom.UI.Backpack.PIEValidation.CommandsAndScopedConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackPIEValidationCommandsSpec::RunTest(const FString& Parameters)
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	TestNotNull(
		TEXT("The 24-card validation seed command is registered"),
		ConsoleManager.FindConsoleObject(TEXT("Wacom.Backpack.SeedPIEValidation")));
	TestNotNull(
		TEXT("The 100-card validation seed command is registered"),
		ConsoleManager.FindConsoleObject(TEXT("Wacom.Backpack.SeedPIEValidation100")));
	TestNotNull(
		TEXT("The read-only empty validation command is registered"),
		ConsoleManager.FindConsoleObject(TEXT("Wacom.Backpack.OpenEmptyPIEValidation")));
	TestNotNull(
		TEXT("The native fallback validation command is registered"),
		ConsoleManager.FindConsoleObject(TEXT("Wacom.Backpack.OpenNativeFallbackPIEValidation")));

	TestEqual(
		TEXT("PIE validation mode is disabled by default"),
		GetWacomBackpackPIEValidationMode(),
		EWacomBackpackPIEValidationMode::None);

	{
		FScopedWacomBackpackPIEValidationMode EmptyMode(
			EWacomBackpackPIEValidationMode::EmptySnapshot);
		TestEqual(
			TEXT("The empty validation scope sets its construction mode"),
			GetWacomBackpackPIEValidationMode(),
			EWacomBackpackPIEValidationMode::EmptySnapshot);

		TStrongObjectPtr<UWacomBackpackScreen> EmptyScreen(
			NewObject<UWacomBackpackScreen>(GetTransientPackage()));
		TestTrue(
			TEXT("A screen constructed in the empty scope retains the read-only empty mode"),
			FWacomBackpackScreenTestAccess::UsesEmptyPIEValidationSnapshot(*EmptyScreen));

		{
			FScopedWacomBackpackPIEValidationMode NativeMode(
				EWacomBackpackPIEValidationMode::NativeFallback);
			TestEqual(
				TEXT("A nested native scope temporarily replaces the outer mode"),
				GetWacomBackpackPIEValidationMode(),
				EWacomBackpackPIEValidationMode::NativeFallback);

			TStrongObjectPtr<UWacomBackpackScreen> NativeScreen(
				NewObject<UWacomBackpackScreen>(GetTransientPackage()));
			TestFalse(
				TEXT("The native fallback screen does not inherit the empty snapshot mode"),
				FWacomBackpackScreenTestAccess::UsesEmptyPIEValidationSnapshot(*NativeScreen));
			TestTrue(
				TEXT("The native fallback screen selects only C++ visual child classes"),
				FWacomBackpackScreenTestAccess::UsesNativeFallbackVisualClasses(*NativeScreen));
		}

		TestEqual(
			TEXT("Leaving the nested scope restores the outer empty mode"),
			GetWacomBackpackPIEValidationMode(),
			EWacomBackpackPIEValidationMode::EmptySnapshot);
	}

	TestEqual(
		TEXT("Leaving all scopes restores normal construction"),
		GetWacomBackpackPIEValidationMode(),
		EWacomBackpackPIEValidationMode::None);

	TStrongObjectPtr<UWacomBackpackScreen> NormalScreen(
		NewObject<UWacomBackpackScreen>(GetTransientPackage()));
	TestFalse(
		TEXT("A subsequently constructed normal screen has no empty validation override"),
		FWacomBackpackScreenTestAccess::UsesEmptyPIEValidationSnapshot(*NormalScreen));
	return true;
}

#endif
