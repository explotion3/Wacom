// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Shop/WacomWorldShopInteractionPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopInteractionOwnershipSpec,
	"Wacom.UI.WorldShop.Input.WidgetInteractionIsOwnedByControlledPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopInteractionOwnershipSpec::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World())
			{
				World = Context.World();
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("automation world"), World))
	{
		return false;
	}

	AWacomPlayerController* PlayerController = World->SpawnActor<AWacomPlayerController>();
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>();
	if (!TestNotNull(TEXT("player controller"), PlayerController)
		|| !TestNotNull(TEXT("controlled pawn"), Character))
	{
		if (Character)
		{
			Character->Destroy();
		}
		if (PlayerController)
		{
			PlayerController->Destroy();
		}
		return false;
	}

	PlayerController->Possess(Character);
	TestEqual(
		TEXT("interaction owner ignores the controlled pawn collision hierarchy"),
		FWacomWorldShopInteractionPolicy::ResolveWidgetInteractionOwner(*PlayerController),
		static_cast<AActor*>(Character));

	PlayerController->UnPossess();
	TestEqual(
		TEXT("controller is the safe fallback while no pawn is possessed"),
		FWacomWorldShopInteractionPolicy::ResolveWidgetInteractionOwner(*PlayerController),
		static_cast<AActor*>(PlayerController));

	Character->Destroy();
	PlayerController->Destroy();
	return true;
}
