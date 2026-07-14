// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Settings/SettingsScreenTestAccess.h"
#include "UI/GameViewportClientTestAccess.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Settings/WacomSettingsOptionRow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsOwnsFocusPresentationSpec,
	"Wacom.UI.Settings.FocusPresentation.OwnershipContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsOwnsFocusPresentationSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMenuButtonWidget> Button(
		NewObject<UWacomMenuButtonWidget>());
	Button->Initialize();
	TestTrue(
		TEXT("Settings menu buttons replace the engine focus brush with a project focus skin"),
		FWacomGameViewportClientTestAccess::HasProjectOwnedFocusPresentation(
			Button->TakeWidget()));

	TStrongObjectPtr<UWacomSettingsOptionRow> Row(
		NewObject<UWacomSettingsOptionRow>());
	Row->Initialize();
	TestTrue(
		TEXT("Settings rows replace the engine focus brush with a project focus skin"),
		FWacomGameViewportClientTestAccess::HasProjectOwnedFocusPresentation(
			Row->TakeWidget()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsFocusMatchesHoverSpec,
	"Wacom.UI.Settings.FocusPresentation.FocusMatchesHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsFocusMatchesHoverSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMenuButtonWidget> Button(
		NewObject<UWacomMenuButtonWidget>());
	FWacomSettingsFocusPresentationTestAccess::Construct(*Button);
	const FLinearColor RestingButtonColor =
		FWacomSettingsFocusPresentationTestAccess::ButtonBackdropColor(*Button);

	FWacomSettingsFocusPresentationTestAccess::Hover(*Button);
	const FLinearColor HoveredButtonColor =
		FWacomSettingsFocusPresentationTestAccess::ButtonBackdropColor(*Button);
	TestNotEqual(
		TEXT("Mouse hover visibly emphasizes a Settings button"),
		HoveredButtonColor,
		RestingButtonColor);

	FWacomSettingsFocusPresentationTestAccess::Unhover(*Button);
	FWacomSettingsFocusPresentationTestAccess::Focus(*Button);
	TestTrue(
		TEXT("CommonUI focus is tracked by the Settings button skin"),
		FWacomSettingsFocusPresentationTestAccess::ButtonHasFocusPresentation(*Button));
	TestEqual(
		TEXT("Keyboard and gamepad focus use the same visual as mouse hover"),
		FWacomSettingsFocusPresentationTestAccess::ButtonBackdropColor(*Button),
		HoveredButtonColor);
	FWacomSettingsFocusPresentationTestAccess::Unfocus(*Button);
	TestEqual(
		TEXT("Focus loss restores the resting Settings button visual"),
		FWacomSettingsFocusPresentationTestAccess::ButtonBackdropColor(*Button),
		RestingButtonColor);
	FWacomSettingsFocusPresentationTestAccess::Destruct(*Button);

	TStrongObjectPtr<UWacomSettingsOptionRow> Row(
		NewObject<UWacomSettingsOptionRow>());
	FWacomSettingsFocusPresentationTestAccess::Construct(*Row);
	FWacomSettingsOptionRowViewData RowData;
	RowData.bEnabled = true;
	Row->ApplyViewData(RowData);
	const FLinearColor RestingRowColor =
		FWacomSettingsFocusPresentationTestAccess::RowBackdropColor(*Row);
	FWacomSettingsFocusPresentationTestAccess::Focus(*Row);
	TestTrue(
		TEXT("Settings row tracks focus anywhere in its focus path"),
		FWacomSettingsFocusPresentationTestAccess::RowHasFocusPresentation(*Row));
	TestNotEqual(
		TEXT("Focused Settings row has an authored visual instead of only the engine brush"),
		FWacomSettingsFocusPresentationTestAccess::RowBackdropColor(*Row),
		RestingRowColor);
	FWacomSettingsFocusPresentationTestAccess::Unfocus(*Row);
	TestEqual(
		TEXT("Settings row returns to its resting visual after focus leaves"),
		FWacomSettingsFocusPresentationTestAccess::RowBackdropColor(*Row),
		RestingRowColor);
	FWacomSettingsFocusPresentationTestAccess::Destruct(*Row);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAuthoredFocusSkinSpec,
	"Wacom.UI.Settings.FocusPresentation.AuthoredSkinBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAuthoredFocusSkinSpec::RunTest(
	const FString& /*Parameters*/)
{
	UClass* ButtonClass = LoadClass<UWacomMenuButtonWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Settings/WBP_SettingsButton.WBP_SettingsButton_C"));
	UClass* RowClass = LoadClass<UWacomSettingsOptionRow>(
		nullptr,
		TEXT("/Game/Wacom/UI/Settings/WBP_SettingsOptionRow.WBP_SettingsOptionRow_C"));
	if (!TestNotNull(TEXT("Authored Settings button class"), ButtonClass)
		|| !TestNotNull(TEXT("Authored Settings row class"), RowClass))
	{
		return false;
	}

	TStrongObjectPtr<UWacomMenuButtonWidget> Button(
		NewObject<UWacomMenuButtonWidget>(GetTransientPackage(), ButtonClass));
	FWacomSettingsFocusPresentationTestAccess::Construct(*Button);
	TestTrue(
		TEXT("Authored Settings button binds its backdrop and accent"),
		FWacomSettingsFocusPresentationTestAccess::ButtonHasAuthoredVisualBindings(*Button));
	FWacomSettingsFocusPresentationTestAccess::Destruct(*Button);

	TStrongObjectPtr<UWacomSettingsOptionRow> Row(
		NewObject<UWacomSettingsOptionRow>(GetTransientPackage(), RowClass));
	FWacomSettingsFocusPresentationTestAccess::Construct(*Row);
	TestTrue(
		TEXT("Authored Settings row binds its focus backdrop and value labels"),
		FWacomSettingsFocusPresentationTestAccess::RowHasAuthoredVisualBindings(*Row));
	FWacomSettingsFocusPresentationTestAccess::Destruct(*Row);
	return true;
}

#endif
