// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "UI/MainMenuScreenTestAccess.h"
#include "UI/GameViewportClientTestAccess.h"
#include "Engine/UserInterfaceSettings.h"
#include "GameFramework/WacomMenuGameMode.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Menus/WacomMainMenuScreen.h"
#include "Components/Widget.h"
#include "UObject/UnrealType.h"

namespace
{
	TStrongObjectPtr<UWacomMainMenuScreen> MakeMainMenuScreen()
	{
		TStrongObjectPtr<UWacomMainMenuScreen> Screen(NewObject<UWacomMainMenuScreen>());
		FWacomMainMenuScreenTestAccess::Build(*Screen);
		return Screen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuUsesProjectFocusPresentationSpec,
	"Wacom.UI.MainMenu.Screen.UsesProjectFocusPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuUsesProjectFocusPresentationSpec::RunTest(const FString& /*Parameters*/)
{
	const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>();
	TestNotNull(TEXT("Project UI settings are available"), UISettings);
	if (UISettings)
	{
		TestNotEqual(
			TEXT("Engine focus brush remains available outside project-owned focus widgets"),
			UISettings->RenderFocusRule,
			ERenderFocusRule::Never);
	}

	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	TestTrue(
		TEXT("Main menu keeps focusable CommonUI buttons for keyboard and gamepad navigation"),
		FWacomMainMenuScreenTestAccess::HasCompleteFocusableCommonUIButtonSet(*Screen));

	UWacomMainMenuButtonWidget* MainMenuButton = Cast<UWacomMainMenuButtonWidget>(
		Screen->GetWidgetFromName(TEXT("NewJourneyButton")));
	if (TestNotNull(TEXT("Main menu button is available"), MainMenuButton))
	{
		TestTrue(
			TEXT("Main menu button owns its focus presentation"),
			FWacomGameViewportClientTestAccess::HasProjectOwnedFocusPresentation(
				MainMenuButton->TakeWidget()));
	}

	TStrongObjectPtr<UWacomMenuButtonWidget> GenericButton(
		NewObject<UWacomMenuButtonWidget>());
	GenericButton->Initialize();
	TestTrue(
		TEXT("Generic project menu buttons now own a reusable focus skin"),
		FWacomGameViewportClientTestAccess::HasProjectOwnedFocusPresentation(
			GenericButton->TakeWidget()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuCommonButtonFocusDrivesHoverPresentationSpec,
	"Wacom.UI.MainMenu.Screen.CommonButtonFocusDrivesHoverPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuCommonButtonFocusDrivesHoverPresentationSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	UWacomMainMenuButtonWidget* Button = Cast<UWacomMainMenuButtonWidget>(
		Screen->GetWidgetFromName(TEXT("NewJourneyButton")));
	if (!TestNotNull(TEXT("Main menu button is available"), Button))
	{
		return false;
	}

	FWacomMainMenuScreenTestAccess::Construct(*Button);
	TestEqual(
		TEXT("Button starts without keyboard emphasis"),
		FWacomMainMenuScreenTestAccess::TargetEmphasis(*Button),
		0.0f);

	FWacomMainMenuScreenTestAccess::BroadcastFocusReceived(*Button);
	TestEqual(
		TEXT("CommonButton focus uses the same emphasis target as mouse hover"),
		FWacomMainMenuScreenTestAccess::TargetEmphasis(*Button),
		1.0f);

	FWacomMainMenuScreenTestAccess::BroadcastFocusLost(*Button);
	TestEqual(
		TEXT("CommonButton focus loss clears emphasis when the mouse is not hovering"),
		FWacomMainMenuScreenTestAccess::TargetEmphasis(*Button),
		0.0f);
	FWacomMainMenuScreenTestAccess::Destruct(*Button);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuAuthoredWidgetBlueprintContractSpec,
	"Wacom.UI.MainMenu.Screen.AuthoredWidgetBlueprintContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuAuthoredWidgetBlueprintContractSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* ScreenClass = LoadClass<UWacomMainMenuScreen>(
		nullptr,
		TEXT("/Game/Wacom/UI/Menus/WBP_MainMenuScreen.WBP_MainMenuScreen_C"));
	UClass* NavButtonClass = LoadClass<UWacomMainMenuButtonWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Menus/WBP_MainMenuNavButton.WBP_MainMenuNavButton_C"));
	if (!TestNotNull(TEXT("Authored main menu Screen class"), ScreenClass)
		|| !TestNotNull(TEXT("Authored main menu navigation button class"), NavButtonClass))
	{
		return false;
	}

	TStrongObjectPtr<UWacomMainMenuScreen> Screen(
		NewObject<UWacomMainMenuScreen>(GetTransientPackage(), ScreenClass));
	FWacomMainMenuScreenTestAccess::Build(*Screen);
	TestTrue(
		TEXT("Authored Screen binds the required presentation widgets and six nav button instances"),
		FWacomMainMenuScreenTestAccess::UsesAuthoredMainMenuWidgets(*Screen, NavButtonClass));

	Screen->ApplyViewData(FWacomMainMenuViewData());
	const FWacomMainMenuScreenAutomationTestView View =
		FWacomMainMenuScreenTestAccess::View(*Screen);
	TestEqual(TEXT("Authored New Journey label survives nested WBP serialization"),
		Screen->GetWidgetFromName(TEXT("NewJourneyButton"))
			? CastChecked<UWacomMainMenuButtonWidget>(
				Screen->GetWidgetFromName(TEXT("NewJourneyButton")))->GetButtonText().ToString()
			: FString(),
		FString(TEXT("开始新旅程")));
	TestEqual(TEXT("Authored no-journey summary title"),
		View.SummaryTitle.ToString(),
		FString(TEXT("准备启程")));

	const AWacomMenuGameMode* MenuGameMode = GetDefault<AWacomMenuGameMode>();
	TestNotNull(TEXT("MenuGameMode defaults to authored main menu Screen"), MenuGameMode);
	if (MenuGameMode)
	{
		TestEqual(TEXT("MenuGameMode authored Screen class"),
			MenuGameMode->MainMenuScreenClass.Get(),
			ScreenClass);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuCommonUIButtonContractSpec,
	"Wacom.UI.MainMenu.Screen.CommonUIButtonContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuCommonUIButtonContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	TestTrue(
		TEXT("Fallback provides six focusable CommonUI navigation buttons"),
		FWacomMainMenuScreenTestAccess::HasCompleteFocusableCommonUIButtonSet(*Screen));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuRootBackCannotEmptyLayerSpec,
	"Wacom.UI.MainMenu.Screen.RootBackCannotEmptyLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuRootBackCannotEmptyLayerSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	int32 ReturnToTitleRequestCount = 0;
	Screen->OnActionRequestedNative.AddLambda(
		[&ReturnToTitleRequestCount](EWacomMainMenuAction Action)
		{
			if (Action == EWacomMainMenuAction::ReturnToTitle)
			{
				++ReturnToTitleRequestCount;
			}
		});
	Screen->ActivateWidget();
	TestTrue(TEXT("Main menu starts active"), Screen->IsActivated());

	const FReply Reply = FWacomMainMenuScreenTestAccess::SendEscapeKeyDown(*Screen);
	TestTrue(TEXT("Main menu handles Escape"), Reply.IsEventHandled());
	TestTrue(
		TEXT("Main menu cannot deactivate itself when no title flow consumes Back"),
		Screen->IsActivated());
	TestEqual(
		TEXT("Main menu routes Escape to the app-owned ReturnToTitle action"),
		ReturnToTitleRequestCount,
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuFallbackNamesAvoidWidgetPropertiesSpec,
	"Wacom.UI.MainMenu.Screen.FallbackNamesAvoidUWidgetProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuFallbackNamesAvoidWidgetPropertiesSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	const TArray<FName> WidgetNames = FWacomMainMenuScreenTestAccess::WidgetNames(*Screen);

	TSet<FName> ReservedWidgetPropertyNames;
	for (TFieldIterator<FProperty> PropertyIt(UWidget::StaticClass(), EFieldIteratorFlags::IncludeSuper);
		PropertyIt;
		++PropertyIt)
	{
		ReservedWidgetPropertyNames.Add(PropertyIt->GetFName());
	}

	for (FName WidgetName : WidgetNames)
	{
		TestFalse(
			*FString::Printf(
				TEXT("Fallback widget name '%s' must not shadow a UWidget property"),
				*WidgetName.ToString()),
			ReservedWidgetPropertyNames.Contains(WidgetName));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuDefaultViewDataSpec,
	"Wacom.UI.MainMenu.Screen.DefaultViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuDefaultViewDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	Screen->ApplyViewData(FWacomMainMenuViewData());

	const FWacomMainMenuScreenAutomationTestView View =
		FWacomMainMenuScreenTestAccess::View(*Screen);
	TestFalse(TEXT("Continue is hidden without an active journey"), View.bContinueVisible);
	TestFalse(TEXT("Continue is disabled without an active journey"), View.bContinueEnabled);
	TestTrue(TEXT("New Journey remains visible"), View.bNewJourneyVisible);
	TestFalse(TEXT("Journey History is hidden until its page exists"), View.bJourneyHistoryVisible);
	TestFalse(TEXT("Settings is hidden until its page exists"), View.bSettingsVisible);
	TestFalse(TEXT("Credits is hidden until its page exists"), View.bCreditsVisible);
	TestTrue(TEXT("Quit remains visible"), View.bQuitVisible);
	TestTrue(TEXT("CommonUI focus restoration is enabled"), View.bAutoRestoreFocus);
	TestEqual(TEXT("New Journey is the default focus target"),
		View.DesiredFocusTargetName,
		FName(TEXT("NewJourneyButton")));
	TestEqual(TEXT("No-journey fallback summary title"),
		View.SummaryTitle.ToString(),
		FString(TEXT("准备启程")));
	TestEqual(TEXT("No-journey fallback summary body"),
		View.SummaryBody.ToString(),
		FString(TEXT("开始一段新的旅程。")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuActiveJourneyViewDataSpec,
	"Wacom.UI.MainMenu.Screen.ActiveJourneyViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuActiveJourneyViewDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();

	FWacomMainMenuViewData ViewData;
	ViewData.bHasActiveJourney = true;
	ViewData.bCanContinueJourney = true;
	ViewData.ActiveJourneyTitle = FText::FromString(TEXT("测试旅程"));
	ViewData.ActiveJourneySummary = FText::FromString(TEXT("区域 2 · 42 分钟"));
	ViewData.bShowJourneyHistory = true;
	ViewData.bShowSettings = true;
	ViewData.bShowCredits = true;
	Screen->ApplyViewData(ViewData);

	FWacomMainMenuScreenAutomationTestView View =
		FWacomMainMenuScreenTestAccess::View(*Screen);
	TestTrue(TEXT("Continue is visible for an active journey"), View.bContinueVisible);
	TestTrue(TEXT("Continue is enabled for a valid active journey"), View.bContinueEnabled);
	TestTrue(TEXT("Journey History follows feature availability"), View.bJourneyHistoryVisible);
	TestTrue(TEXT("Settings follows feature availability"), View.bSettingsVisible);
	TestTrue(TEXT("Credits follows feature availability"), View.bCreditsVisible);
	TestEqual(TEXT("Continue is the active-journey focus target"),
		View.DesiredFocusTargetName,
		FName(TEXT("ContinueButton")));
	TestEqual(TEXT("Active journey title is applied"),
		View.SummaryTitle.ToString(),
		ViewData.ActiveJourneyTitle.ToString());
	TestEqual(TEXT("Active journey summary is applied"),
		View.SummaryBody.ToString(),
		ViewData.ActiveJourneySummary.ToString());

	ViewData.bCanContinueJourney = false;
	Screen->ApplyViewData(ViewData);
	View = FWacomMainMenuScreenTestAccess::View(*Screen);
	TestTrue(TEXT("Invalid active journey remains visible"), View.bContinueVisible);
	TestFalse(TEXT("Invalid active journey cannot continue"), View.bContinueEnabled);
	TestEqual(TEXT("Disabled Continue falls back to New Journey focus"),
		View.DesiredFocusTargetName,
		FName(TEXT("NewJourneyButton")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuActionAvailabilitySpec,
	"Wacom.UI.MainMenu.Screen.ActionAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuActionAvailabilitySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	TArray<EWacomMainMenuAction> ReceivedActions;
	Screen->OnActionRequestedNative.AddLambda(
		[&ReceivedActions](EWacomMainMenuAction Action)
		{
			ReceivedActions.Add(Action);
		});

	Screen->ApplyViewData(FWacomMainMenuViewData());
	Screen->RequestAction(EWacomMainMenuAction::ContinueJourney);
	Screen->RequestAction(EWacomMainMenuAction::JourneyHistory);
	Screen->RequestAction(EWacomMainMenuAction::Settings);
	Screen->RequestAction(EWacomMainMenuAction::Credits);
	TestEqual(TEXT("Unavailable actions do not broadcast"), ReceivedActions.Num(), 0);

	FWacomMainMenuViewData ViewData;
	ViewData.bHasActiveJourney = true;
	ViewData.bCanContinueJourney = true;
	ViewData.bShowJourneyHistory = true;
	ViewData.bShowSettings = true;
	ViewData.bShowCredits = true;
	Screen->ApplyViewData(ViewData);

	const EWacomMainMenuAction ExpectedActions[] = {
		EWacomMainMenuAction::ContinueJourney,
		EWacomMainMenuAction::StartNewJourney,
		EWacomMainMenuAction::JourneyHistory,
		EWacomMainMenuAction::Settings,
		EWacomMainMenuAction::Credits,
		EWacomMainMenuAction::Quit
	};
	for (EWacomMainMenuAction Action : ExpectedActions)
	{
		Screen->RequestAction(Action);
	}

	TestEqual(TEXT("All available actions broadcast exactly once"),
		ReceivedActions.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(ExpectedActions)));
	for (int32 Index = 0; Index < ReceivedActions.Num(); ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("Action order [%d]"), Index),
			ReceivedActions[Index],
			ExpectedActions[Index]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuLifecycleBindingSpec,
	"Wacom.UI.MainMenu.Screen.LifecycleBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuLifecycleBindingSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomMainMenuScreen> Screen = MakeMainMenuScreen();
	int32 NewJourneyRequestCount = 0;
	Screen->OnActionRequestedNative.AddLambda(
		[&NewJourneyRequestCount](EWacomMainMenuAction Action)
		{
			if (Action == EWacomMainMenuAction::StartNewJourney)
			{
				++NewJourneyRequestCount;
			}
		});

	Screen->ApplyViewData(FWacomMainMenuViewData());
	Screen->ApplyViewData(FWacomMainMenuViewData());
	TestEqual(TEXT("Applying ViewData has no action side effect"), NewJourneyRequestCount, 0);

	FWacomMainMenuScreenTestAccess::Construct(*Screen);
	FWacomMainMenuScreenTestAccess::Construct(*Screen);
	FWacomMainMenuScreenTestAccess::Click(*Screen, EWacomMainMenuAction::StartNewJourney);
	TestEqual(TEXT("Repeated construct keeps one button binding"), NewJourneyRequestCount, 1);

	FWacomMainMenuScreenTestAccess::Destruct(*Screen);
	FWacomMainMenuScreenTestAccess::Click(*Screen, EWacomMainMenuAction::StartNewJourney);
	TestEqual(TEXT("Destruct removes the button binding"), NewJourneyRequestCount, 1);

	FWacomMainMenuScreenTestAccess::Construct(*Screen);
	FWacomMainMenuScreenTestAccess::Click(*Screen, EWacomMainMenuAction::StartNewJourney);
	TestEqual(TEXT("Reconstruct restores exactly one binding"), NewJourneyRequestCount, 2);
	FWacomMainMenuScreenTestAccess::Destruct(*Screen);
	return true;
}

#endif
