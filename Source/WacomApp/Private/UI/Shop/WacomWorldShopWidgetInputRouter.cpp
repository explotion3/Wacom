// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopWidgetInputRouter.h"

#include "Components/WidgetComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "InputCoreTypes.h"
#include "UI/Shop/WacomWorldShopInteractionPolicy.h"
#include "UI/Shop/WacomWorldShopWidgetInteractionComponent.h"

bool FWacomWorldShopWidgetInputRouter::Initialize(
	AWacomPlayerController& PlayerController,
	float InteractionDistance)
{
	CancelAndClear();
	AActor* InteractionOwner =
		FWacomWorldShopInteractionPolicy::ResolveWidgetInteractionOwner(PlayerController);
	if (!InteractionOwner)
	{
		return false;
	}
	UWacomWorldShopWidgetInteractionComponent* Interaction =
		NewObject<UWacomWorldShopWidgetInteractionComponent>(
		InteractionOwner,
		TEXT("WorldShopWidgetInteraction"));
	if (!Interaction)
	{
		return false;
	}
	InteractionOwner->AddInstanceComponent(Interaction);
	Interaction->InteractionSource = EWidgetInteractionSource::Mouse;
	Interaction->InteractionDistance = FMath::Max(1.0f, InteractionDistance);
	Interaction->TraceChannel = ECollisionChannel::ECC_Visibility;
	Interaction->bShowDebug = false;
	Interaction->VirtualUserIndex = 0;
	Interaction->PointerIndex = 7;
	Interaction->RegisterComponent();
	Interaction->Activate(true);
	Owner = &PlayerController;
	WidgetInteraction = Interaction;
	UE_LOG(LogTemp, Display,
		TEXT("[WorldShopInput] WidgetInteraction ready Owner=%s Pawn=%s Distance=%.1f"),
		*GetNameSafe(InteractionOwner),
		*GetNameSafe(PlayerController.GetPawn()),
		Interaction->InteractionDistance);
	return true;
}

bool FWacomWorldShopWidgetInputRouter::RoutePointerKey(const FKey& Key, EInputEvent Event)
{
	UWacomWorldShopWidgetInteractionComponent* Interaction =
		WidgetInteraction.Get();
	if (!Interaction || Key != EKeys::LeftMouseButton)
	{
		return false;
	}
	if (Event == IE_Pressed)
	{
		const FHitResult& Hit = Interaction->GetLastHitResult();
		UE_LOG(LogTemp, Display,
			TEXT("[WorldShopInput] Press Owner=%s HoveredComponent=%s HitTestVisible=%s HitActor=%s HitComponent=%s Distance=%.1f"),
			*GetNameSafe(Interaction->GetOwner()),
			*GetNameSafe(Interaction->GetHoveredWidgetComponent()),
			Interaction->IsOverHitTestVisibleWidget() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()),
			Hit.Distance);
		if (!bLeftPressed)
		{
			Interaction->PressPointerKey(EKeys::LeftMouseButton);
			bLeftPressed = true;
		}
		return true;
	}
	if (Event == IE_Released)
	{
		if (bLeftPressed)
		{
			Interaction->ReleasePointerKey(EKeys::LeftMouseButton);
		}
		bLeftPressed = false;
		return true;
	}
	return false;
}

bool FWacomWorldShopWidgetInputRouter::GetPointerSample(
	FWacomWorldCardPointerSample& OutSample,
	const bool bForceRefresh)
{
	OutSample = FWacomWorldCardPointerSample();
	UWacomWorldShopWidgetInteractionComponent* Interaction =
		WidgetInteraction.Get();
	if (!Interaction)
	{
		return false;
	}
	if (bForceRefresh)
	{
		Interaction->RefreshPointerSample();
	}
	OutSample.HoveredComponent = Interaction->GetHoveredWidgetComponent();
	OutSample.LocalHitLocation = Interaction->Get2DHitLocation();
	OutSample.bOverHitTestVisibleWidget =
		Interaction->IsOverHitTestVisibleWidget();
	return OutSample.HasHoveredComponent();
}

void FWacomWorldShopWidgetInputRouter::CancelAndClear()
{
	if (UWacomWorldShopWidgetInteractionComponent* Interaction =
		WidgetInteraction.Get())
	{
		if (bLeftPressed)
		{
			Interaction->ReleasePointerKey(EKeys::LeftMouseButton);
		}
		Interaction->DestroyComponent();
	}
	bLeftPressed = false;
	WidgetInteraction.Reset();
	Owner.Reset();
}
