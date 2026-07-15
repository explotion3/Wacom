// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/RunWorldInteractionActorTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomGameMenuBlocksRunScenePointerRoutingTest,
	"Wacom.UI.GameMenu.PointerRouting.ActiveMenuBlocksRunSceneClicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomGameMenuBlocksRunScenePointerRoutingTest::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(
		NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>());
	TStrongObjectPtr<UWacomPauseMenuScreen> Menu(
		NewObject<UWacomPauseMenuScreen>());

	FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(Target.Get());
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSceneHit(
		PC.Get(), Target.Get(), Target->ClickBounds);

	TestTrue(
		TEXT("Exploration without a menu permits Run scene pointer routing"),
		FWacomPlayerControllerRunInteractionTestAccess::CanRouteRunScenePointerInput(
			PC.Get()));
	TestTrue(
		TEXT("Baseline world click is routed before a menu opens"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
			PC.Get()));
	TestEqual(
		TEXT("Baseline route submits exactly once"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Target.Get()),
		1);

	FWacomPlayerControllerRunInteractionTestAccess::RegisterActiveGameMenu(
		PC.Get(), Menu.Get());
	TestFalse(
		TEXT("An active GameMenu closes the Run scene pointer route"),
		FWacomPlayerControllerRunInteractionTestAccess::CanRouteRunScenePointerInput(
			PC.Get()));
	TestFalse(
		TEXT("Direct Run world click routing is rejected while the menu owns input"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
			PC.Get()));
	TestFalse(
		TEXT("Unhandled left release is not consumed by Run scene routing behind the menu"),
		FWacomPlayerControllerRunInteractionTestAccess::InputLeftMouseReleased(PC.Get()));
	TestEqual(
		TEXT("Menu click cannot leak into the world target"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Target.Get()),
		1);

	FWacomPlayerControllerRunInteractionTestAccess::UnregisterActiveGameMenu(
		PC.Get(), Menu.Get());
	TestTrue(
		TEXT("Closing the GameMenu restores Run scene pointer routing"),
		FWacomPlayerControllerRunInteractionTestAccess::CanRouteRunScenePointerInput(
			PC.Get()));
	TestTrue(
		TEXT("World click routes again after the menu closes"),
		FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
			PC.Get()));
	TestEqual(
		TEXT("Restored route submits exactly once more"),
		FWacomRunWorldInteractionActorTestAccess::TryInteractCount(Target.Get()),
		2);

	return true;
}

#endif
