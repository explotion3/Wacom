// Copyright Wacom. All Rights Reserved.

#include "UI/MainMenuScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"

void FWacomMainMenuScreenTestAccess::Build(UWacomMainMenuScreen& Screen)
{
	Screen.Initialize();
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

void FWacomMainMenuScreenTestAccess::Construct(UWacomMainMenuButtonWidget& Button)
{
	Button.NativeConstruct();
}

void FWacomMainMenuScreenTestAccess::Destruct(UWacomMainMenuButtonWidget& Button)
{
	Button.NativeDestruct();
}

void FWacomMainMenuScreenTestAccess::BroadcastFocusReceived(
	UWacomMainMenuButtonWidget& Button)
{
	Button.OnFocusReceived().Broadcast();
}

void FWacomMainMenuScreenTestAccess::BroadcastFocusLost(
	UWacomMainMenuButtonWidget& Button)
{
	Button.OnFocusLost().Broadcast();
}

FReply FWacomMainMenuScreenTestAccess::SendEscapeKeyDown(
	UWacomMainMenuScreen& Screen)
{
	const FKeyEvent KeyEvent(
		EKeys::Escape,
		FModifierKeysState(),
		0,
		false,
		0,
		0);
	return Screen.NativeOnKeyDown(FGeometry(), KeyEvent);
}

float FWacomMainMenuScreenTestAccess::TargetEmphasis(
	const UWacomMainMenuButtonWidget& Button)
{
	return Button.TargetEmphasis;
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

bool FWacomMainMenuScreenTestAccess::UsesAuthoredMainMenuWidgets(
	const UWacomMainMenuScreen& Screen,
	const UClass* ExpectedButtonClass)
{
	if (!ExpectedButtonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuScreenTestAccess] Missing expected button class"));
		return false;
	}

	if (!Screen.MenuContentRoot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuScreenTestAccess] Missing Screen binding: MenuContentRoot"));
		return false;
	}
	if (!Screen.JourneySummaryPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuScreenTestAccess] Missing Screen binding: JourneySummaryPanel"));
		return false;
	}
	if (!Screen.ActiveJourneyTitleText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuScreenTestAccess] Missing Screen binding: ActiveJourneyTitleText"));
		return false;
	}
	if (!Screen.ActiveJourneySummaryText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuScreenTestAccess] Missing Screen binding: ActiveJourneySummaryText"));
		return false;
	}

	const UWacomMainMenuButtonWidget* Buttons[] = {
		Screen.ContinueButton,
		Screen.NewJourneyButton,
		Screen.JourneyHistoryButton,
		Screen.SettingsButton,
		Screen.CreditsButton,
		Screen.QuitButton
	};
	for (int32 ButtonIndex = 0; ButtonIndex < UE_ARRAY_COUNT(Buttons); ++ButtonIndex)
	{
		const UWacomMainMenuButtonWidget* Button = Buttons[ButtonIndex];
		if (!Button)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[MainMenuScreenTestAccess] Missing nav button binding at index %d"),
				ButtonIndex);
			return false;
		}
		if (Button->GetClass() != ExpectedButtonClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[MainMenuScreenTestAccess] Nav button %s has class %s, expected %s"),
				*Button->GetName(),
				*GetNameSafe(Button->GetClass()),
				*GetNameSafe(ExpectedButtonClass));
			return false;
		}

		const FName RequiredButtonWidgets[] = {
			TEXT("ButtonBackdrop"),
			TEXT("ButtonAccent"),
			TEXT("ButtonGlyph"),
			TEXT("ButtonText")
		};
		for (const FName RequiredWidgetName : RequiredButtonWidgets)
		{
			if (!Button->GetWidgetFromName(RequiredWidgetName))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[MainMenuScreenTestAccess] Nav button %s is missing %s"),
					*Button->GetName(),
					*RequiredWidgetName.ToString());
				return false;
			}
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
