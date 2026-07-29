// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomWorldInteractableContractHelpers.h"

#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Exploration/RunExplorationTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomWorldInteractable.h"
#include "RunSession.h"

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

	bool IsAvailableAtBoundRunMapNode(
		const AActor* Actor,
		AWacomPlayerController* PC,
		FName* OutRejectReason)
	{
		auto Reject = [OutRejectReason](const FName Reason)
		{
			if (OutRejectReason)
			{
				*OutRejectReason = Reason;
			}
			return false;
		};

		if (OutRejectReason)
		{
			*OutRejectReason = NAME_None;
		}
		if (!Actor)
		{
			return Reject(TEXT("MissingActor"));
		}

		const UWacomRunMapNodeBindingComponent* Binding =
			Actor->FindComponentByClass<UWacomRunMapNodeBindingComponent>();
		if (!Binding)
		{
			// Standalone prototypes remain usable until they opt into the authoritative map binding.
			return true;
		}
		if (Binding->NodeId.IsNone() || Binding->NodeType == EWacomMapNodeType::Navigation)
		{
			return Reject(TEXT("InvalidRunMapNodeBinding"));
		}
		if (!PC)
		{
			return Reject(TEXT("MissingPlayerController"));
		}

		URunSession* Run = PC->GetRunSession();
		if (!Run)
		{
			return Reject(TEXT("MissingRunSession"));
		}

		const FRunExplorationSnapshot Snapshot = Run->BuildExplorationSnapshot();
		if (!Snapshot.CurrentNode.IsValid() || Snapshot.CurrentNode.NodeId != Binding->NodeId)
		{
			return Reject(TEXT("BoundNodeNotCurrent"));
		}
		if (Snapshot.ActiveActivityKind != ERunExplorationActivityKind::None)
		{
			// BeginTraversal intentionally keeps CurrentNode at the source until arrival.
			// A node-id-only check would therefore leave the departed source content
			// interactable throughout the corridor and allow presentation staging to
			// begin before WacomRun rejects the conflicting activity.
			return Reject(TEXT("RunActivityActive"));
		}

		const FRunMapNodeSnapshot* CurrentNode = Snapshot.Nodes.FindByPredicate(
			[&Snapshot](const FRunMapNodeSnapshot& Node)
			{
				return Node.Handle == Snapshot.CurrentNode;
			});
		if (!CurrentNode || CurrentNode->NodeType != Binding->NodeType)
		{
			return Reject(TEXT("BoundNodeTypeMismatch"));
		}

		return true;
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
		if (!IsAvailableAtBoundRunMapNode(Actor, PC))
		{
			return false;
		}
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
		if (!IsAvailableAtBoundRunMapNode(Actor, PC))
		{
			return false;
		}
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
