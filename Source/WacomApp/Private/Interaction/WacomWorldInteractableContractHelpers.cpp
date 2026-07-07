// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomWorldInteractableContractHelpers.h"

#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomWorldInteractable.h"

namespace WacomWorldInteractableContractHelpers
{
	bool IsWorldInteractableActor(const AActor* Actor)
	{
		return Actor && Actor->GetClass()->ImplementsInterface(UWacomWorldInteractable::StaticClass());
	}

	bool IsRunWorldClickableInteractableActor(const AActor* Actor)
	{
		return Actor
			&& Actor->GetClass()->ImplementsInterface(
				UWacomRunWorldClickableInteractable::StaticClass());
	}

	FText GetInteractPromptTextFromActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetInteractPromptText_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			? IWacomWorldInteractable::Execute_GetInteractPromptText(Actor, PC)
			: FText::GetEmpty();
	}

	FVector GetInteractLocationFromActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetInteractLocation_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			? IWacomWorldInteractable::Execute_GetInteractLocation(Actor, PC)
			: FVector::ZeroVector;
	}

	bool CanInteractWithActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->CanInteract_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			&& IWacomWorldInteractable::Execute_CanInteract(Actor, PC);
	}

	bool TryInteractWithActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->TryInteract_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			&& IWacomWorldInteractable::Execute_TryInteract(Actor, PC);
	}

	FText GetRunWorldClickHoverPromptFromActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomRunWorldClickableInteractable* Native =
			Cast<IWacomRunWorldClickableInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetRunWorldClickHoverPrompt_Implementation(PC);
			}
		}
		return IsRunWorldClickableInteractableActor(Actor)
			? IWacomRunWorldClickableInteractable::Execute_GetRunWorldClickHoverPrompt(Actor, PC)
			: FText::GetEmpty();
	}

	FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugViewFromActor(
		AActor* Actor,
		AWacomPlayerController* PC)
	{
		if (IWacomRunWorldClickableInteractable* Native =
			Cast<IWacomRunWorldClickableInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetRunWorldClickableDebugView_Implementation(PC);
			}
		}
		return IsRunWorldClickableInteractableActor(Actor)
			? IWacomRunWorldClickableInteractable::Execute_GetRunWorldClickableDebugView(Actor, PC)
			: FWacomRunWorldClickableInteractableDebugView();
	}

	FName BuildRunWorldClickableHoverReason(
		const FWacomRunWorldClickableInteractableDebugView& TriggerDebug)
	{
		if (TriggerDebug.bIsCompleted)
		{
			return TEXT("Completed");
		}
		if (TriggerDebug.bCanInteract)
		{
			return TEXT("Ok");
		}
		return TriggerDebug.RejectReason.IsNone()
			? TriggerDebug.LastDebugResult
			: TriggerDebug.RejectReason;
	}
}
