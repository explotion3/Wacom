// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopInteractionPolicy.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/WacomPlayerController.h"

AActor* FWacomWorldShopInteractionPolicy::ResolveWidgetInteractionOwner(
	AWacomPlayerController& PlayerController)
{
	return PlayerController.GetPawn()
		? static_cast<AActor*>(PlayerController.GetPawn())
		: static_cast<AActor*>(&PlayerController);
}
