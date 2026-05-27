// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"

namespace WacomInputContextSpec
{
	struct FCoordinatorHarness
	{
		TStrongObjectPtr<APlayerController> PC;
		TStrongObjectPtr<ULocalPlayer> LocalPlayer;
		TStrongObjectPtr<UWacomInputContextCoordinatorSubsystem> Coordinator;

		FCoordinatorHarness()
			: PC(NewObject<APlayerController>())
			, LocalPlayer(NewObject<ULocalPlayer>(GEngine))
			, Coordinator(NewObject<UWacomInputContextCoordinatorSubsystem>(LocalPlayer.Get()))
		{
			Coordinator->InitializeForPlayerController(PC.Get());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextProfilesTest,
	"Wacom.UI.InputContext.Profiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextProfilesTest::RunTest(const FString& Parameters)
{
	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	Harness.Coordinator->SetExplorationProfile(EWacomExplorationInputProfile::FreeLook);
	TestFalse(TEXT("Exploration free-look hides cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetExplorationProfile(EWacomExplorationInputProfile::RunTunnel);
	TestTrue(TEXT("Run tunnel shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Battle);
	TestTrue(TEXT("Battle shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::MainMenu);
	TestTrue(TEXT("Main menu shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextBattleRestoresRunTunnelProfileTest,
	"Wacom.UI.InputContext.BattleRestoresRunTunnelProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextBattleRestoresRunTunnelProfileTest::RunTest(const FString& Parameters)
{
	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	Harness.Coordinator->SetExplorationProfile(EWacomExplorationInputProfile::RunTunnel);
	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Battle);
	TestEqual(TEXT("Battle does not erase exploration subprofile"), Harness.Coordinator->GetExplorationProfile(), EWacomExplorationInputProfile::RunTunnel);

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	TestEqual(TEXT("Exploration returns to run tunnel profile"), Harness.Coordinator->GetExplorationProfile(), EWacomExplorationInputProfile::RunTunnel);
	TestTrue(TEXT("Restored run tunnel profile shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetExplorationProfile(EWacomExplorationInputProfile::FreeLook);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextInteractionEventLeaseTest,
	"Wacom.UI.InputContext.InteractionEventsAreOwnerRefCounted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextInteractionEventLeaseTest::RunTest(const FString& Parameters)
{
	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("PlayerController"), Harness.PC.Get()))
	{
		return false;
	}

	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	Harness.Coordinator->ReleaseAllPlayerControllerInteractionEvents();

	const bool bOriginalClickEvents = Harness.PC->bEnableClickEvents;
	const bool bOriginalMouseOverEvents = Harness.PC->bEnableMouseOverEvents;
	Harness.PC->bEnableClickEvents = false;
	Harness.PC->bEnableMouseOverEvents = false;

	TStrongObjectPtr<APlayerController> FirstOwner(NewObject<APlayerController>());
	TStrongObjectPtr<APlayerController> SecondOwner(NewObject<APlayerController>());

	Harness.Coordinator->AcquirePlayerControllerInteractionEvents(FirstOwner.Get(), true, true);
	Harness.Coordinator->AcquirePlayerControllerInteractionEvents(SecondOwner.Get(), true, false);
	TestTrue(TEXT("Click events enabled by leases"), Harness.PC->bEnableClickEvents);
	TestTrue(TEXT("Mouse over events enabled by first lease"), Harness.PC->bEnableMouseOverEvents);

	Harness.Coordinator->ReleasePlayerControllerInteractionEvents(FirstOwner.Get(), true, true);
	TestTrue(TEXT("Second owner keeps click events enabled"), Harness.PC->bEnableClickEvents);
	TestFalse(TEXT("Mouse over restores when its only lease releases"), Harness.PC->bEnableMouseOverEvents);

	Harness.Coordinator->ReleasePlayerControllerInteractionEvents(SecondOwner.Get(), true, false);
	TestFalse(TEXT("All click leases released"), Harness.PC->bEnableClickEvents);
	TestFalse(TEXT("All mouse over leases released"), Harness.PC->bEnableMouseOverEvents);

	Harness.PC->bEnableClickEvents = bOriginalClickEvents;
	Harness.PC->bEnableMouseOverEvents = bOriginalMouseOverEvents;
	return true;
}
