// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Interaction/WacomRunWorldCardDropReceiver.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"

#if WITH_AUTOMATION_TESTS

class AActor;
class AWacomPlayerControllerProbe;
class UPrimitiveComponent;
class URunSession;
class UWacomAppToastSubsystem;
class UWacomRunMenuDropTargetWidget;
class UWacomRunWorldInteractionTargetBridgeComponent;
class UWacomRunWorldCardDropReceiverComponent;

struct FWacomPlayerControllerRunInteractionTestAccess
{
	static AActor* ClosestInteractable(const AWacomPlayerControllerProbe* PC);
	static FText CurrentInteractPrompt(const AWacomPlayerControllerProbe* PC);
	static FString RunWorldInteractableHoverDebugSummary(const AWacomPlayerControllerProbe* PC);

	static void SetRunSceneHit(
		AWacomPlayerControllerProbe* PC,
		AActor* InActor,
		UPrimitiveComponent* InComponent = nullptr);
	static void ClearRunSceneHit(AWacomPlayerControllerProbe* PC);
	static void SetRunProbeExplorationFlow(AWacomPlayerControllerProbe* PC, bool bInExploration);
	static bool RouteRunWorldInteractableClick(AWacomPlayerControllerProbe* PC);
	static bool InputLeftMouseReleased(AWacomPlayerControllerProbe* PC);
	static bool ProbeRunSceneTarget(
		const AWacomPlayerControllerProbe* PC,
		FWacomInteractionTargetHandle& OutHandle);
	static bool ProbeRunSceneTargetAtWidgetPosition(
		const AWacomPlayerControllerProbe* PC,
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle);
	static void UpdateRunWorldTargetProbePreview(AWacomPlayerControllerProbe* PC);
	static void ClearRunWorldTargetProbePreview(AWacomPlayerControllerProbe* PC);

	static void RegisterRunMenuDropTarget(
		AWacomPlayerControllerProbe* PC,
		UWacomRunMenuDropTargetWidget* Target);
	static void UnregisterRunMenuDropTarget(
		AWacomPlayerControllerProbe* PC,
		UWacomRunMenuDropTargetWidget* Target);
	static bool ProbeRunMenuDropTargetAtWidgetPosition(
		const AWacomPlayerControllerProbe* PC,
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle);
	static void ClearRunMenuDropTargetProbe(AWacomPlayerControllerProbe* PC);
	static bool ApplyRunMenuDropProbeFeedback(
		AWacomPlayerControllerProbe* PC,
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	static FString RunMenuDropProbeDebugSummary(const AWacomPlayerControllerProbe* PC);
	static FWacomRunMenuCardDropResolveResult ResolveRunMenuCardDropIntent(
		const AWacomPlayerControllerProbe* PC,
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);

	static bool ApplyRunWorldCardDropProbeFeedback(
		AWacomPlayerControllerProbe* PC,
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	static FRunWorldCardInteractionValidation ResolveRunWorldCardDropIntent(
		const AWacomPlayerControllerProbe* PC,
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		AActor*& OutTargetActor,
		UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
		UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
		FString& OutDebugSummary);
	static FString RunWorldCardDropDebugSummary(const AWacomPlayerControllerProbe* PC);

	static void SetRunSession(AWacomPlayerControllerProbe* PC, URunSession* RunSession);
	static void SetRunFirstPersonMenuLease(
		AWacomPlayerControllerProbe* PC,
		FName LeaseId = TEXT("Test.MenuLease"));
	static void SetAppToastSubsystem(
		AWacomPlayerControllerProbe* PC,
		UWacomAppToastSubsystem* ToastSubsystem);
};

#endif
