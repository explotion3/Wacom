// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomRunWorldClickableInteractable.h"

#include "Components/BoxComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomWorldInteractable.h"

void FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(UBoxComponent* ClickBounds)
{
	if (!ClickBounds)
	{
		return;
	}

	ClickBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBounds->SetGenerateOverlapEvents(false);
}

void FWacomRunWorldClickableInteractableHelper::BindClickTarget(
	FName StableId,
	UPrimitiveComponent* VisualTargetComponent,
	UWacomInteractionTargetComponent* InteractionTargetComponent,
	UWacomRunWorldInteractionTargetBridgeComponent* BridgeComponent)
{
	if (BridgeComponent)
	{
		BridgeComponent->RunTargetStableId = StableId;
		BridgeComponent->VisualTargetComponent = VisualTargetComponent;
	}
	if (InteractionTargetComponent)
	{
		InteractionTargetComponent->SetStableTargetId(StableId);
	}
}

FWacomRunWorldClickableInteractableDebugView
FWacomRunWorldClickableInteractableHelper::BuildDebugView(
	const AActor* Actor,
	FName StableId,
	const FText& HoverPrompt,
	bool bCanInteract,
	bool bHasCompletionState,
	bool bIsCompleted,
	FName LastDebugResult,
	const UWacomInteractionTargetComponent* InteractionTargetComponent,
	const UWacomRunWorldInteractionTargetBridgeComponent* BridgeComponent,
	const UBoxComponent* ClickBounds)
{
	FWacomRunWorldClickableInteractableDebugView View;
	View.ActorName = Actor ? Actor->GetName() : TEXT("None");
	View.StableId = StableId;
	View.bHasStableId = !StableId.IsNone();
	View.bImplementsWorldInteractable = Actor
		&& Actor->GetClass()->ImplementsInterface(UWacomWorldInteractable::StaticClass());
	View.bImplementsClickableContract = Actor
		&& Actor->GetClass()->ImplementsInterface(UWacomRunWorldClickableInteractable::StaticClass());
	View.bCanInteract = bCanInteract;
	View.bHasCompletionState = bHasCompletionState;
	View.bIsCompleted = bHasCompletionState && bIsCompleted;
	View.HoverPrompt = HoverPrompt.ToString();
	View.RejectReason = LastDebugResult == FName(TEXT("Ok")) ? NAME_None : LastDebugResult;
	View.LastDebugResult = LastDebugResult;
	View.bClickBoundsConfigured = ClickBounds
		&& ClickBounds->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& ClickBounds->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block
		&& !ClickBounds->GetGenerateOverlapEvents();
	View.bHasInteractionTargetComponent = InteractionTargetComponent != nullptr;
	View.bHasBridgeComponent = BridgeComponent != nullptr;

	if (BridgeComponent)
	{
		const FWacomRunWorldInteractionTargetDebugView ClickDebug =
			BridgeComponent->GetRunWorldTargetDebugView();
		View.bClickTargetConfigured = ClickDebug.bInteractionTargetConfigured;
		View.ClickTargetStableId = ClickDebug.RunTargetStableId;
		View.bHasInteractionTargetComponent = ClickDebug.bHasInteractionTargetComponent;
		View.bHasVisualTarget = ClickDebug.bHasVisualTarget;
		View.bHasRenderableVisualTarget = ClickDebug.bHasRenderableVisualTarget;
		View.VisualTargetName = ClickDebug.VisualTargetName;
		View.bProbePreviewActive = ClickDebug.bProbePreviewActive;
	}
	else if (InteractionTargetComponent)
	{
		View.ClickTargetStableId = InteractionTargetComponent->GetStableTargetId();
	}

	return View;
}

FString FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(
	const FWacomRunWorldClickableInteractableDebugView& View)
{
	return FString::Printf(
		TEXT("RunWorldClickable{Actor=%s StableId=%s HasStableId=%s WorldInteractable=%s Clickable=%s CanInteract=%s Completed=%s ClickBounds=%s ClickTarget=%s ClickStableId=%s HasTarget=%s HasBridge=%s HasVisual=%s HasRenderableVisual=%s VisualTarget=%s PreviewActive=%s Prompt=%s Reject=%s Last=%s}"),
		*View.ActorName,
		*View.StableId.ToString(),
		View.bHasStableId ? TEXT("true") : TEXT("false"),
		View.bImplementsWorldInteractable ? TEXT("true") : TEXT("false"),
		View.bImplementsClickableContract ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsCompleted ? TEXT("true") : TEXT("false"),
		View.bClickBoundsConfigured ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		View.bHasInteractionTargetComponent ? TEXT("true") : TEXT("false"),
		View.bHasBridgeComponent ? TEXT("true") : TEXT("false"),
		View.bHasVisualTarget ? TEXT("true") : TEXT("false"),
		View.bHasRenderableVisualTarget ? TEXT("true") : TEXT("false"),
		*View.VisualTargetName.ToString(),
		View.bProbePreviewActive ? TEXT("true") : TEXT("false"),
		*View.HoverPrompt,
		*View.RejectReason.ToString(),
		*View.LastDebugResult.ToString());
}
