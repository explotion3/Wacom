// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"

class AActor;
class AWacomPlayerController;

namespace WacomWorldInteractableContractHelpers
{
	bool IsWorldInteractableActor(const AActor* Actor);
	bool IsRunWorldClickableInteractableActor(const AActor* Actor);

	FText GetInteractPromptTextFromActor(AActor* Actor, AWacomPlayerController* PC);
	FVector GetInteractLocationFromActor(AActor* Actor, AWacomPlayerController* PC);
	bool CanInteractWithActor(AActor* Actor, AWacomPlayerController* PC);
	bool TryInteractWithActor(AActor* Actor, AWacomPlayerController* PC);

	FText GetRunWorldClickHoverPromptFromActor(AActor* Actor, AWacomPlayerController* PC);
	FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugViewFromActor(
		AActor* Actor,
		AWacomPlayerController* PC);
	FName BuildRunWorldClickableHoverReason(
		const FWacomRunWorldClickableInteractableDebugView& TriggerDebug);
}
