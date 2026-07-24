// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/RunWorldInteractionActorTestAccess.h"
#include "UI/Shop/WacomWorldShopInteractionPolicy.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopWidgetInteractionOwnershipSpec,
	"Wacom.UI.WorldShop.Input.WidgetInteractionIsOwnedByControlledPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopWidgetInteractionOwnershipSpec::RunTest(
	const FString& Parameters)
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

	AWacomPlayerController* PlayerController =
		World->SpawnActor<AWacomPlayerController>();
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>();
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
		FWacomWorldShopInteractionPolicy::ResolveWidgetInteractionOwner(
			*PlayerController),
		static_cast<AActor*>(Character));

	PlayerController->UnPossess();
	TestEqual(
		TEXT("controller is the safe fallback while no pawn is possessed"),
		FWacomWorldShopInteractionPolicy::ResolveWidgetInteractionOwner(
			*PlayerController),
		static_cast<AActor*>(PlayerController));

	Character->Destroy();
	PlayerController->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopInteractionOwnershipSpec,
	"Wacom.UI.WorldShop.Input.OwnershipClearsOrdinaryRunWorldHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopInteractionOwnershipSpec::RunTest(
	const FString& Parameters)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(
		NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	if (!TestNotNull(TEXT("player controller probe"), PC.Get())
		|| !TestNotNull(TEXT("shop probe"), Shop.Get()))
	{
		return false;
	}

	Shop->PersistentId = TEXT("Shop.UI.WorldActivityOwnership");
	FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(Shop.Get());
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		Shop->GetClickTargetBridgeComponent();
	if (!TestNotNull(TEXT("shop target bridge"), Bridge))
	{
		return false;
	}
	Bridge->RefreshRunWorldTargetBinding();

	UStaticMeshComponent* Backboard = NewObject<UStaticMeshComponent>(Shop.Get());
	Shop->AddInstanceComponent(Backboard);
	Backboard->SetRelativeScale3D(FVector(3.0f));
	Backboard->SetRenderCustomDepth(false);
	Bridge->VisualTargetComponent = Backboard;
	Bridge->ProbePreviewScale = 1.1f;

	FWacomPlayerControllerRunInteractionTestAccess::SetRunSceneHit(
		PC.Get(),
		Shop.Get(),
		Shop->GetClickBounds());
	FWacomPlayerControllerRunInteractionTestAccess::
		UpdateRunWorldTargetProbePreview(PC.Get());
	TestTrue(TEXT("ordinary Run hover starts active"), Bridge->IsProbePreviewActive());
	TestEqual(
		TEXT("ordinary Run hover scales the Backboard"),
		Backboard->GetRelativeScale3D(),
		FVector(3.3f));

	FWacomPlayerControllerRunInteractionTestAccess::
		SetRunScenePointerRouteOverride(PC.Get(), false);
	FWacomPlayerControllerRunInteractionTestAccess::
		UpdateRunWorldTargetProbePreview(PC.Get());
	TestFalse(
		TEXT("World Activity input ownership clears the old Run hover"),
		Bridge->IsProbePreviewActive());
	TestEqual(
		TEXT("clearing the old Run hover restores the Backboard scale"),
		Backboard->GetRelativeScale3D(),
		FVector(3.0f));
	TestFalse(
		TEXT("clearing the old Run hover restores custom depth"),
		Backboard->bRenderCustomDepth);

	FWacomInteractionTargetHandle Handle;
	TestFalse(
		TEXT("ordinary Run target probing remains blocked while the activity owns input"),
		FWacomPlayerControllerRunInteractionTestAccess::ProbeRunSceneTarget(
			PC.Get(),
			Handle));

	FWacomPlayerControllerRunInteractionTestAccess::
		SetRunScenePointerRouteOverride(PC.Get(), true);
	FWacomPlayerControllerRunInteractionTestAccess::
		UpdateRunWorldTargetProbePreview(PC.Get());
	TestTrue(
		TEXT("ordinary Run hover can resume after activity ownership is released"),
		Bridge->IsProbePreviewActive());

	FWacomPlayerControllerRunInteractionTestAccess::
		SetRunScenePointerRouteOverride(PC.Get(), TOptional<bool>());
	FWacomPlayerControllerRunInteractionTestAccess::
		ClearRunWorldTargetProbePreview(PC.Get());
	return true;
}

#endif
