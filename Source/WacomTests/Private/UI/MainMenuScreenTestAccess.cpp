// Copyright Wacom. All Rights Reserved.

#include "UI/MainMenuScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Components/Button.h"

void FWacomMainMenuScreenTestAccess::Build(UWacomMainMenuScreen& Screen)
{
	Screen.TakeWidget();
}

void FWacomMainMenuScreenTestAccess::Construct(UWacomMainMenuScreen& Screen)
{
	Screen.NativeConstruct();
}

void FWacomMainMenuScreenTestAccess::Destruct(UWacomMainMenuScreen& Screen)
{
	Screen.NativeDestruct();
}

void FWacomMainMenuScreenTestAccess::Click(
	UWacomMainMenuScreen& Screen,
	EWacomMainMenuAction Action)
{
	UButton* Button = nullptr;
	switch (Action)
	{
	case EWacomMainMenuAction::ContinueJourney:
		Button = Screen.ContinueButton;
		break;
	case EWacomMainMenuAction::StartNewJourney:
		Button = Screen.NewJourneyButton;
		break;
	case EWacomMainMenuAction::JourneyHistory:
		Button = Screen.JourneyHistoryButton;
		break;
	case EWacomMainMenuAction::Settings:
		Button = Screen.SettingsButton;
		break;
	case EWacomMainMenuAction::Credits:
		Button = Screen.CreditsButton;
		break;
	case EWacomMainMenuAction::Quit:
		Button = Screen.QuitButton;
		break;
	default:
		break;
	}

	if (Button)
	{
		Button->OnClicked.Broadcast();
	}
}

FWacomMainMenuScreenAutomationTestView FWacomMainMenuScreenTestAccess::View(
	const UWacomMainMenuScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

#endif
