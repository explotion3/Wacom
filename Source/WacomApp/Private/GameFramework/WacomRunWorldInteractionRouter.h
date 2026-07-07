// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"

class AActor;
class AWacomPlayerController;
class UWacomRunWorldInteractionTargetBridgeComponent;

/**
 * App-private Run world interaction router.
 *
 * Owns Run world probe preview, hover prompt state, left-click routing, and
 * clickable target resolution while PlayerController remains the input facade.
 */
class FWacomRunWorldInteractionRouter
{
public:
	explicit FWacomRunWorldInteractionRouter(AWacomPlayerController& InPlayerController);

	bool TryProbeSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const;
	bool TryProbeSceneInteractionTargetAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const;

	bool TryRouteInteractableClick();

	void UpdateTargetProbePreview();
	void ClearTargetProbePreview();
	void ClearHoverPrompt(FName Reason);

	bool ResolveClickableInteractableFromHandle(
		const FWacomInteractionTargetHandle& Handle,
		AActor*& OutInteractableActor,
		UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
		FName& OutRejectReason) const;

	AActor* ResolveSourceActorFromHandle(const FWacomInteractionTargetHandle& Handle) const;
	FString BuildHoverDebugSummary() const;
	FText GetHoverPrompt() const;

private:
	bool CanShowHoverPrompt() const;
	UWacomRunWorldInteractionTargetBridgeComponent* ResolveTargetBridgeFromHandle(
		const FWacomInteractionTargetHandle& Handle) const;
	void UpdateHoverPrompt(
		const FWacomInteractionTargetHandle& Handle,
		AActor* InteractableActor);

	AWacomPlayerController& PlayerController;

	TWeakObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> PreviewedRunWorldTargetBridge;
	TWeakObjectPtr<AActor> HoveredRunWorldInteractableActor;
	FWacomInteractionTargetHandle HoveredRunWorldInteractableHandle;
	FText HoveredRunWorldInteractablePrompt;
	FName LastRunWorldInteractableHoverReason = TEXT("None");
};
