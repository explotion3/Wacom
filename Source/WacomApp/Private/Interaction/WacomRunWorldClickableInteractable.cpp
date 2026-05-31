// Copyright Wacom. All Rights Reserved.

#include "Interaction/WacomRunWorldClickableInteractable.h"

#include "Components/BoxComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "GameFramework/Actor.h"

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
	const UWacomRunWorldInteractionTargetBridgeComponent* BridgeComponent)
{
	FWacomRunWorldClickableInteractableDebugView View;
	View.ActorName = Actor ? Actor->GetName() : TEXT("None");
	View.StableId = StableId;
	View.bCanInteract = bCanInteract;
	View.bHasCompletionState = bHasCompletionState;
	View.bIsCompleted = bHasCompletionState && bIsCompleted;
	View.HoverPrompt = HoverPrompt.ToString();
	View.LastDebugResult = LastDebugResult;

	if (BridgeComponent)
	{
		const FWacomRunWorldInteractionTargetDebugView ClickDebug =
			BridgeComponent->GetRunWorldTargetDebugView();
		View.bClickTargetConfigured = ClickDebug.bInteractionTargetConfigured;
		View.ClickTargetStableId = ClickDebug.RunTargetStableId;
	}
	else if (InteractionTargetComponent)
	{
		View.ClickTargetStableId = InteractionTargetComponent->GetStableTargetId();
	}

	return View;
}
