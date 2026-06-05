// Copyright Wacom. All Rights Reserved.

#include "UI/PlayerControllerRunInteractionTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/WacomShopRunEventTestProbes.h"

AActor* FWacomPlayerControllerRunInteractionTestAccess::ClosestInteractable(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->ReadClosestInteractable() : nullptr;
}

FText FWacomPlayerControllerRunInteractionTestAccess::CurrentInteractPrompt(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->ReadCurrentInteractPrompt() : FText::GetEmpty();
}

FString FWacomPlayerControllerRunInteractionTestAccess::RunWorldInteractableHoverDebugSummary(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->ReadRunWorldInteractableHoverDebugSummaryForTest() : FString();
}

void FWacomPlayerControllerRunInteractionTestAccess::SetRunSceneHit(
	AWacomPlayerControllerProbe* PC,
	AActor* InActor,
	UPrimitiveComponent* InComponent)
{
	if (PC)
	{
		PC->SetRunSceneHitForTest(InActor, InComponent);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::ClearRunSceneHit(
	AWacomPlayerControllerProbe* PC)
{
	if (PC)
	{
		PC->ClearRunSceneHitForTest();
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::SetRunProbeExplorationFlow(
	AWacomPlayerControllerProbe* PC,
	bool bInExploration)
{
	if (PC)
	{
		PC->SetRunProbeExplorationFlowForTest(bInExploration);
	}
}

bool FWacomPlayerControllerRunInteractionTestAccess::RouteRunWorldInteractableClick(
	AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->RouteRunWorldInteractableClickForTest() : false;
}

bool FWacomPlayerControllerRunInteractionTestAccess::InputLeftMouseReleased(
	AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->InputLeftMouseReleasedForTest() : false;
}

bool FWacomPlayerControllerRunInteractionTestAccess::ProbeRunSceneTarget(
	const AWacomPlayerControllerProbe* PC,
	FWacomInteractionTargetHandle& OutHandle)
{
	return PC ? PC->ProbeRunSceneTargetForTest(OutHandle) : false;
}

bool FWacomPlayerControllerRunInteractionTestAccess::ProbeRunSceneTargetAtWidgetPosition(
	const AWacomPlayerControllerProbe* PC,
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle)
{
	return PC
		? PC->ProbeRunSceneTargetAtWidgetPositionForTest(WidgetPosition, OutHandle)
		: false;
}

void FWacomPlayerControllerRunInteractionTestAccess::UpdateRunWorldTargetProbePreview(
	AWacomPlayerControllerProbe* PC)
{
	if (PC)
	{
		PC->UpdateRunWorldTargetProbePreviewForTest();
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::ClearRunWorldTargetProbePreview(
	AWacomPlayerControllerProbe* PC)
{
	if (PC)
	{
		PC->ClearRunWorldTargetProbePreviewForTest();
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::RegisterRunMenuDropTarget(
	AWacomPlayerControllerProbe* PC,
	UWacomRunMenuDropTargetWidget* Target)
{
	if (PC)
	{
		PC->RegisterRunMenuDropTargetForTest(Target);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::UnregisterRunMenuDropTarget(
	AWacomPlayerControllerProbe* PC,
	UWacomRunMenuDropTargetWidget* Target)
{
	if (PC)
	{
		PC->UnregisterRunMenuDropTargetForTest(Target);
	}
}

bool FWacomPlayerControllerRunInteractionTestAccess::ProbeRunMenuDropTargetAtWidgetPosition(
	const AWacomPlayerControllerProbe* PC,
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle)
{
	return PC
		? PC->ProbeRunMenuDropTargetAtWidgetPositionForTest(WidgetPosition, OutHandle)
		: false;
}

void FWacomPlayerControllerRunInteractionTestAccess::ClearRunMenuDropTargetProbe(
	AWacomPlayerControllerProbe* PC)
{
	if (PC)
	{
		PC->ClearRunMenuDropTargetProbeForTest();
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragStarted(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerDragStartedForTest(CardInstanceId, DragView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragUpdated(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerDragUpdatedForTest(CardInstanceId, DragView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragReleased(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerDragReleasedForTest(CardInstanceId, DragView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerDragCancelled(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerDragCancelledForTest(CardInstanceId, DragView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerPointerMoved(
	AWacomPlayerControllerProbe* PC,
	const FWacomFirstPersonCardPointerView& PointerView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerPointerMovedForTest(PointerView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerPointerLeft(
	AWacomPlayerControllerProbe* PC)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerPointerLeftForTest();
	}
}

bool FWacomPlayerControllerRunInteractionTestAccess::ApplyRunMenuDropProbeFeedback(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	return PC ? PC->ApplyRunMenuDropProbeFeedbackForTest(CardInstanceId, DragView, bReleased) : false;
}

FString FWacomPlayerControllerRunInteractionTestAccess::RunMenuDropProbeDebugSummary(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->ReadRunMenuDropProbeDebugSummaryForTest() : FString();
}

FWacomRunMenuCardDropResolveResult
FWacomPlayerControllerRunInteractionTestAccess::ResolveRunMenuCardDropIntent(
	const AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	return PC
		? PC->ResolveRunMenuCardDropIntentForTest(CardInstanceId, DragView)
		: FWacomRunMenuCardDropResolveResult();
}

bool FWacomPlayerControllerRunInteractionTestAccess::ApplyRunWorldCardDropProbeFeedback(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	return PC
		? PC->ApplyRunWorldCardDropProbeFeedbackForTest(CardInstanceId, DragView, bReleased)
		: false;
}

FRunWorldCardInteractionValidation
FWacomPlayerControllerRunInteractionTestAccess::ResolveRunWorldCardDropIntent(
	const AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	AActor*& OutTargetActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
	UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
	FString& OutDebugSummary)
{
	return PC
		? PC->ResolveRunWorldCardDropIntentForTest(
			CardInstanceId,
			DragView,
			OutTargetHandle,
			OutTargetActor,
			OutTargetBridge,
			OutReceiver,
			OutDebugSummary)
		: FRunWorldCardInteractionValidation();
}

FString FWacomPlayerControllerRunInteractionTestAccess::RunWorldCardDropDebugSummary(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->ReadRunWorldCardDropDebugSummaryForTest() : FString();
}

void FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(
	AWacomPlayerControllerProbe* PC,
	URunSession* RunSession)
{
	if (PC)
	{
		PC->SetRunSessionForTest(RunSession);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::SetRunFirstPersonMenuLease(
	AWacomPlayerControllerProbe* PC,
	FName LeaseId)
{
	if (PC)
	{
		PC->SetRunFirstPersonMenuLeaseForTest(LeaseId);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::SetAppToastSubsystem(
	AWacomPlayerControllerProbe* PC,
	UWacomAppToastSubsystem* ToastSubsystem)
{
	if (PC)
	{
		PC->SetAppToastSubsystemForTest(ToastSubsystem);
	}
}

#endif
