// Copyright Wacom. All Rights Reserved.

#include "Settings/SettingsScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Settings/WacomSettingsConfirmationDialog.h"

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

void FWacomSettingsScreenTestAccess::SetSupportedResolutions(
	UWacomSettingsScreen& Screen,
	const TArray<FIntPoint>& Resolutions)
{
	Screen.SupportedResolutions = Resolutions;
	Screen.SupportedResolutions.AddUnique(Screen.Draft.ScreenResolution);
	Screen.SupportedResolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X == B.X ? A.Y < B.Y : A.X < B.X;
	});
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
