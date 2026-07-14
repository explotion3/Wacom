// Copyright Wacom. All Rights Reserved.

#include "Settings/SettingsScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Settings/WacomSettingsSubsystem.h"
#include "Components/Border.h"
#include "Input/Events.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Settings/WacomSettingsConfirmationDialog.h"

void FWacomSettingsFocusPresentationTestAccess::Construct(
	UWacomMenuButtonWidget& Button)
{
	Button.Initialize();
	Button.TakeWidget();
	Button.NativeConstruct();
}

void FWacomSettingsFocusPresentationTestAccess::Destruct(
	UWacomMenuButtonWidget& Button)
{
	Button.NativeDestruct();
}

void FWacomSettingsFocusPresentationTestAccess::Hover(
	UWacomMenuButtonWidget& Button)
{
	Button.NativeOnHovered();
}

void FWacomSettingsFocusPresentationTestAccess::Unhover(
	UWacomMenuButtonWidget& Button)
{
	Button.NativeOnUnhovered();
}

void FWacomSettingsFocusPresentationTestAccess::Focus(
	UWacomMenuButtonWidget& Button)
{
	Button.OnFocusReceived().Broadcast();
}

void FWacomSettingsFocusPresentationTestAccess::Unfocus(
	UWacomMenuButtonWidget& Button)
{
	Button.OnFocusLost().Broadcast();
}

FLinearColor FWacomSettingsFocusPresentationTestAccess::ButtonBackdropColor(
	const UWacomMenuButtonWidget& Button)
{
	return Button.ButtonBackdrop
		? Button.ButtonBackdrop->GetBrushColor()
		: FLinearColor::Transparent;
}

bool FWacomSettingsFocusPresentationTestAccess::ButtonHasFocusPresentation(
	const UWacomMenuButtonWidget& Button)
{
	return Button.bPresentationFocused;
}

bool FWacomSettingsFocusPresentationTestAccess::ButtonHasAuthoredVisualBindings(
	const UWacomMenuButtonWidget& Button)
{
	return Button.ButtonBackdrop && Button.Accent;
}

void FWacomSettingsFocusPresentationTestAccess::Construct(
	UWacomSettingsOptionRow& Row)
{
	Row.Initialize();
	Row.TakeWidget();
	Row.NativeConstruct();
}

void FWacomSettingsFocusPresentationTestAccess::Destruct(
	UWacomSettingsOptionRow& Row)
{
	Row.NativeDestruct();
}

void FWacomSettingsFocusPresentationTestAccess::Focus(
	UWacomSettingsOptionRow& Row)
{
	Row.NativeOnAddedToFocusPath(FFocusEvent(EFocusCause::Navigation, 0));
}

void FWacomSettingsFocusPresentationTestAccess::Unfocus(
	UWacomSettingsOptionRow& Row)
{
	Row.NativeOnRemovedFromFocusPath(FFocusEvent(EFocusCause::Navigation, 0));
}

FLinearColor FWacomSettingsFocusPresentationTestAccess::RowBackdropColor(
	const UWacomSettingsOptionRow& Row)
{
	return Row.RowBackdrop
		? Row.RowBackdrop->GetBrushColor()
		: FLinearColor::Transparent;
}

bool FWacomSettingsFocusPresentationTestAccess::RowHasFocusPresentation(
	const UWacomSettingsOptionRow& Row)
{
	return Row.bFocusWithin;
}

bool FWacomSettingsFocusPresentationTestAccess::RowHasAuthoredVisualBindings(
	const UWacomSettingsOptionRow& Row)
{
	return Row.RowBackdrop && Row.LabelText && Row.ValueText;
}

void FWacomSettingsScreenTestAccess::BuildAndConstruct(UWacomSettingsScreen& Screen)
{
	Screen.Initialize();
	Screen.TakeWidget();
	Screen.NativeConstruct();
}

void FWacomSettingsScreenTestAccess::Destruct(UWacomSettingsScreen& Screen)
{
	Screen.NativeDestruct();
}

bool FWacomSettingsScreenTestAccess::BeginWithSubsystem(
	UWacomSettingsScreen& Screen,
	UWacomSettingsSubsystem& Subsystem)
{
	Screen.SettingsSubsystem = &Subsystem;
	return Screen.BeginEditSession();
}

void FWacomSettingsScreenTestAccess::SelectCategory(
	UWacomSettingsScreen& Screen,
	EWacomSettingsCategory Category)
{
	Screen.SelectCategory(Category);
}

void FWacomSettingsScreenTestAccess::Step(
	UWacomSettingsScreen& Screen,
	EWacomSettingsField Field,
	int32 Direction)
{
	Screen.HandleOptionStep(Field, Direction);
}

void FWacomSettingsScreenTestAccess::SetNormalized(
	UWacomSettingsScreen& Screen,
	EWacomSettingsField Field,
	float Value)
{
	Screen.HandleOptionNormalizedValue(Field, Value);
}

void FWacomSettingsScreenTestAccess::Apply(UWacomSettingsScreen& Screen)
{
	Screen.HandleApplyClicked();
}

void FWacomSettingsScreenTestAccess::RestoreDefaults(UWacomSettingsScreen& Screen)
{
	Screen.HandleRestoreDefaultsClicked();
}

FWacomSettingsOptionRowViewData FWacomSettingsScreenTestAccess::Row(
	const UWacomSettingsScreen& Screen,
	EWacomSettingsField Field)
{
	const TWeakObjectPtr<UWacomSettingsOptionRow>* Found = Screen.OptionRowsByField.Find(Field);
	return Found && Found->IsValid()
		? Found->Get()->GetViewData()
		: FWacomSettingsOptionRowViewData();
}

FWacomSettingsScreenAutomationTestView FWacomSettingsScreenTestAccess::View(
	const UWacomSettingsScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

FWacomSettingsScreenClassView FWacomSettingsScreenTestAccess::Classes(
	const UWacomSettingsScreen& Screen)
{
	FWacomSettingsScreenClassView View;
	View.ButtonClass = Screen.SettingsButtonClass.Get();
	View.RowClass = Screen.OptionRowClass.Get();
	View.DialogClass = Screen.ConfirmationDialogClass.Get();
	return View;
}

bool FWacomSettingsScreenTestAccess::HasRequiredBindings(
	const UWacomSettingsScreen& Screen)
{
	return Screen.CategoryContainer
		&& Screen.OptionsContainer
		&& Screen.CategoryTitleText
		&& Screen.StatusText
		&& Screen.RestoreDefaultsButton
		&& Screen.ApplyButton
		&& Screen.BackButton;
}

bool FWacomSettingsScreenTestAccess::IsRestoreDefaultsFocused(
	const UWacomSettingsScreen& Screen)
{
	return Screen.RestoreDefaultsButton
		&& Screen.RestoreDefaultsButton->HasKeyboardFocus();
}

#endif
