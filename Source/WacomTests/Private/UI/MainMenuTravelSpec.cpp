// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomMenuGameMode.h"
#include "UI/Menus/WacomPauseMenuScreen.h"

namespace
{
	constexpr const TCHAR* ExplorationPackagePath = TEXT("/Game/Wacom/Maps/L_Exploration");
	constexpr const TCHAR* MainMenuPackagePath = TEXT("/Game/Wacom/Maps/L_MainMenu");

	bool IsLevelPackagePath(const FName LevelName)
	{
		const FString Value = LevelName.ToString();
		return Value.StartsWith(TEXT("/Game/")) && !Value.Contains(TEXT("."));
	}

	struct FWacomMainMenuTravelHarness
	{
		UWorld* World = nullptr;
		AWacomMenuGameMode* GameMode = nullptr;

		FWacomMainMenuTravelHarness()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World)
			{
				GameMode = World->SpawnActor<AWacomMenuGameMode>();
				if (GameMode)
				{
					GameMode->SetSuppressActualTravelForAutomation(true);
				}
			}
		}

		~FWacomMainMenuTravelHarness()
		{
			if (World)
			{
				World->DestroyWorld(false);
				World = nullptr;
				GameMode = nullptr;
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuTravelPathsUsePackageNamesSpec,
	"Wacom.UI.MainMenu.TravelPathsUsePackageNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuTravelPathsUsePackageNamesSpec::RunTest(const FString& /*Parameters*/)
{
	const AWacomMenuGameMode* MenuGameMode = GetDefault<AWacomMenuGameMode>();
	if (!TestNotNull(TEXT("MenuGameMode CDO"), MenuGameMode))
	{
		return false;
	}

	TestEqual(TEXT("MenuGameMode exploration target uses package path"),
		MenuGameMode->ExplorationLevelName.ToString(),
		FString(ExplorationPackagePath));
	TestTrue(TEXT("MenuGameMode exploration target has no object suffix"),
		IsLevelPackagePath(MenuGameMode->ExplorationLevelName));

	TestEqual(TEXT("PauseMenu quit target uses package path"),
		UWacomPauseMenuScreen::GetMainMenuLevelPackagePathForTravel().ToString(),
		FString(MainMenuPackagePath));
	TestTrue(TEXT("PauseMenu quit target has no object suffix"),
		IsLevelPackagePath(UWacomPauseMenuScreen::GetMainMenuLevelPackagePathForTravel()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuStartNewGameTearsDownBeforeTravelSpec,
	"Wacom.UI.MainMenu.StartNewGameTearsDownBeforeTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuStartNewGameTearsDownBeforeTravelSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomMainMenuTravelHarness Harness;
	if (!TestNotNull(TEXT("World"), Harness.World)
		|| !TestNotNull(TEXT("MenuGameMode"), Harness.GameMode))
	{
		return false;
	}

	Harness.GameMode->RequestStartNewGame();

	FWacomMenuTravelDebugView View = Harness.GameMode->GetLastMenuTravelDebugView();
	TestEqual(TEXT("StartNewGame schedules exploration package path"),
		View.TravelLevelName.ToString(),
		FString(ExplorationPackagePath));
	TestTrue(TEXT("Target is a package path"), View.bTravelTargetUsesPackagePath);
	TestFalse(TEXT("Default target is not an object path"), View.bRequestedObjectPath);
	TestTrue(TEXT("Primary layout teardown is requested before scheduling"), View.bPrimaryLayoutTeardownRequested);
	TestTrue(TEXT("Travel is scheduled for next tick"), View.bTravelScheduledForNextTick);
	TestTrue(TEXT("Teardown order exists"), View.TeardownOrder > 0);
	TestTrue(TEXT("Schedule order exists"), View.ScheduleOrder > 0);
	TestTrue(TEXT("Teardown is recorded before scheduling"), View.TeardownOrder < View.ScheduleOrder);
	TestFalse(TEXT("Travel has not executed in the button callback"), View.bTravelExecuted);

	Harness.GameMode->FlushPendingTravelForAutomation();
	View = Harness.GameMode->GetLastMenuTravelDebugView();
	TestTrue(TEXT("Deferred travel execution can be flushed"), View.bTravelExecuted);
	TestTrue(TEXT("Execution remains after scheduling"), View.ScheduleOrder < View.ExecuteOrder);
	TestTrue(TEXT("Automation suppresses actual OpenLevel"), View.bActualTravelSuppressedForAutomation);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMainMenuSaveDisabledStartNewGameDoesNotLoadSaveSpec,
	"Wacom.UI.MainMenu.SaveDisabledStartNewGameDoesNotLoadSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMainMenuSaveDisabledStartNewGameDoesNotLoadSaveSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomMainMenuTravelHarness Harness;
	if (!TestNotNull(TEXT("World"), Harness.World)
		|| !TestNotNull(TEXT("MenuGameMode"), Harness.GameMode))
	{
		return false;
	}

	TestFalse(TEXT("Save system remains disabled for the current prototype"),
		AWacomGameMode::bSaveSystemEnabled);

	Harness.GameMode->RequestStartNewGame();
	const FWacomMenuTravelDebugView View = Harness.GameMode->GetLastMenuTravelDebugView();

	TestFalse(TEXT("StartNewGame does not attempt save cleanup while save system is disabled"),
		View.bStartNewGameSaveCleanupAttempted);
	TestEqual(TEXT("Save-disabled StartNewGame still targets exploration"),
		View.TravelLevelName.ToString(),
		FString(ExplorationPackagePath));
	TestTrue(TEXT("Save-disabled StartNewGame still schedules travel"), View.bTravelScheduledForNextTick);

	return true;
}
