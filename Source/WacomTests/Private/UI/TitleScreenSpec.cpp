// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "GameFramework/WacomMenuGameMode.h"
#include "InputCoreTypes.h"
#include "UI/Menus/WacomTitleScreen.h"
#include "UI/TitleScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	TStrongObjectPtr<UWacomTitleScreen> MakeTitleScreen(UClass* ScreenClass = nullptr)
	{
		TStrongObjectPtr<UWacomTitleScreen> Screen(
			NewObject<UWacomTitleScreen>(
				GetTransientPackage(),
				ScreenClass ? ScreenClass : UWacomTitleScreen::StaticClass()));
		FWacomTitleScreenTestAccess::Build(*Screen);
		return Screen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUITitleScreenFallbackContractSpec,
	"Wacom.UI.MainMenu.TitleScreen.FallbackContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUITitleScreenFallbackContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomTitleScreen> Screen = MakeTitleScreen();
	TestTrue(
		TEXT("Native fallback provides the title content and input prompt bindings"),
		FWacomTitleScreenTestAccess::HasRequiredPresentationBindings(*Screen));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUITitleScreenInputContractSpec,
	"Wacom.UI.MainMenu.TitleScreen.InputContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUITitleScreenInputContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomTitleScreen> Screen = MakeTitleScreen();
	int32 AdvanceCount = 0;
	Screen->OnAdvanceRequestedNative.AddLambda([&AdvanceCount]()
	{
		++AdvanceCount;
	});
	Screen->ActivateWidget();

	TestTrue(
		TEXT("Keyboard input is handled"),
		FWacomTitleScreenTestAccess::SendKeyDown(*Screen, EKeys::SpaceBar).IsEventHandled());
	TestEqual(TEXT("Keyboard input requests main menu"), AdvanceCount, 1);

	TestTrue(
		TEXT("Escape is consumed at the stable root"),
		FWacomTitleScreenTestAccess::SendKeyDown(*Screen, EKeys::Escape).IsEventHandled());
	TestTrue(
		TEXT("Gamepad Back is consumed at the stable root"),
		FWacomTitleScreenTestAccess::SendKeyDown(
			*Screen,
			EKeys::Gamepad_FaceButton_Right).IsEventHandled());
	TestEqual(TEXT("Back inputs never request main menu"), AdvanceCount, 1);
	TestTrue(TEXT("Back inputs cannot deactivate TitleScreen"), Screen->IsActivated());

	TestTrue(
		TEXT("Gamepad confirm is handled"),
		FWacomTitleScreenTestAccess::SendKeyDown(
			*Screen,
			EKeys::Gamepad_FaceButton_Bottom).IsEventHandled());
	TestEqual(TEXT("Gamepad confirm requests main menu"), AdvanceCount, 2);

	TestTrue(
		TEXT("Left mouse button is handled"),
		FWacomTitleScreenTestAccess::SendMouseButtonDown(
			*Screen,
			EKeys::LeftMouseButton).IsEventHandled());
	TestEqual(TEXT("Left mouse button requests main menu"), AdvanceCount, 3);

	FWacomTitleScreenTestAccess::SendMouseButtonDown(*Screen, EKeys::RightMouseButton);
	TestEqual(TEXT("Right mouse button does not advance"), AdvanceCount, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUITitleScreenAuthoredAssetContractSpec,
	"Wacom.UI.MainMenu.TitleScreen.AuthoredAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUITitleScreenAuthoredAssetContractSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* TitleClass = LoadClass<UWacomTitleScreen>(
		nullptr,
		TEXT("/Game/Wacom/UI/Menus/WBP_TitleScreen.WBP_TitleScreen_C"));
	if (!TestNotNull(TEXT("Authored TitleScreen class"), TitleClass))
	{
		return false;
	}

	TStrongObjectPtr<UWacomTitleScreen> Screen = MakeTitleScreen(TitleClass);
	TestTrue(
		TEXT("Authored title page provides its fixed presentation bindings"),
		FWacomTitleScreenTestAccess::HasRequiredPresentationBindings(*Screen));

	const AWacomMenuGameMode* MenuGameMode = GetDefault<AWacomMenuGameMode>();
	if (TestNotNull(TEXT("MenuGameMode CDO"), MenuGameMode))
	{
		TestEqual(
			TEXT("MenuGameMode defaults to the authored TitleScreen"),
			MenuGameMode->TitleScreenClass.Get(),
			TitleClass);
	}
	return true;
}

#endif
