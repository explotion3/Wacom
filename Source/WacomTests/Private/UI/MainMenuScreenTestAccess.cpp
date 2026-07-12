// Copyright Wacom. All Rights Reserved.

#include "UI/MainMenuScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"

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
	UWacomMainMenuButtonWidget* Button = nullptr;
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
		Button->OnClicked().Broadcast();
	}
}

bool FWacomMainMenuScreenTestAccess::HasCompleteFocusableCommonUIButtonSet(
	const UWacomMainMenuScreen& Screen)
{
	const UWacomMainMenuButtonWidget* Buttons[] = {
		Screen.ContinueButton,
		Screen.NewJourneyButton,
		Screen.JourneyHistoryButton,
		Screen.SettingsButton,
		Screen.CreditsButton,
		Screen.QuitButton
	};

	for (const UWacomMainMenuButtonWidget* Button : Buttons)
	{
		if (!Button || !Button->GetIsFocusable())
		{
			return false;
		}
	}
	return true;
}

TArray<FName> FWacomMainMenuScreenTestAccess::WidgetNames(
	const UWacomMainMenuScreen& Screen)
{
	TArray<FName> Names;
	if (Screen.WidgetTree)
	{
		Screen.WidgetTree->ForEachWidget(
			[&Names](UWidget* Widget)
			{
				if (Widget)
				{
					Names.Add(Widget->GetFName());
				}
			});
	}
	return Names;
}

FWacomMainMenuScreenAutomationTestView FWacomMainMenuScreenTestAccess::View(
	const UWacomMainMenuScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

#endif
