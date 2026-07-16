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

bool FWacomPlayerControllerRunInteractionTestAccess::CanRouteRunScenePointerInput(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->CanRouteRunScenePointerInput() : false;
}

void FWacomPlayerControllerRunInteractionTestAccess::RegisterActiveGameMenu(
	AWacomPlayerControllerProbe* PC,
	UWacomMenuWidgetBase* Menu)
{
	if (PC)
	{
		PC->RegisterActiveGameMenuWidget(Menu);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::UnregisterActiveGameMenu(
	AWacomPlayerControllerProbe* PC,
	UWacomMenuWidgetBase* Menu)
{
	if (PC)
	{
		PC->UnregisterActiveGameMenuWidget(Menu);
	}
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

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardHovered(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerCardHoveredForTest(CardInstanceId, SlotView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerCardUnhovered(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerCardUnhoveredForTest(CardInstanceId, SlotView);
	}
}

void FWacomPlayerControllerRunInteractionTestAccess::HandleRunFirstPersonCardLayerHoveredCardLayoutUpdated(
	AWacomPlayerControllerProbe* PC,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (PC)
	{
		PC->HandleRunFirstPersonCardLayerHoveredCardLayoutUpdatedForTest(CardInstanceId, SlotView);
	}
}

bool FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelVisible(
	const AWacomPlayerControllerProbe* PC)
{
	return PC && PC->IsRunFirstPersonCardDetailPanelVisibleForTest();
}

FText FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelNameText(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->GetRunFirstPersonCardDetailPanelNameTextForTest() : FText::GetEmpty();
}

FVector2D FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelPosition(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->GetRunFirstPersonCardDetailPanelPositionForTest() : FVector2D::ZeroVector;
}

bool FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailPanelPrewarmed(
	const AWacomPlayerControllerProbe* PC)
{
	return PC && PC->IsRunFirstPersonCardDetailPanelPrewarmedForTest();
}

bool FWacomPlayerControllerRunInteractionTestAccess::IsRunFirstPersonCardDetailMotionPending(
	const AWacomPlayerControllerProbe* PC)
{
	return PC && PC->IsRunFirstPersonCardDetailMotionPendingForTest();
}

float FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailPanelOpacity(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->GetRunFirstPersonCardDetailPanelOpacityForTest() : 0.0f;
}

int32 FWacomPlayerControllerRunInteractionTestAccess::RunFirstPersonCardDetailDataApplyCount(
	const AWacomPlayerControllerProbe* PC)
{
	return PC ? PC->GetRunFirstPersonCardDetailDataApplyCountForTest() : 0;
}

void FWacomPlayerControllerRunInteractionTestAccess::TickRunFirstPersonCardDetail(
	AWacomPlayerControllerProbe* PC,
	float DeltaTime)
{
	if (PC)
	{
		PC->TickRunFirstPersonCardDetailForTest(DeltaTime);
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
		PC->RunSession = RunSession;
		PC->SetRunSessionForTest(RunSession);
	}
}

bool FWacomPlayerControllerRunInteractionTestAccess::RefreshRunExplorationPresentationBinding(
	AWacomPlayerControllerProbe* PC)
{
	return PC && PC->RefreshRunExplorationPresentationBindingForTest();
}

bool FWacomPlayerControllerRunInteractionTestAccess::CanPresentRunMap(
	const AWacomPlayerControllerProbe* PC,
	FName& OutRejectDetail)
{
	bool bPreferRecommendedTarget = false;
	return PC && PC->CanPresentRunMapScreen(
		bPreferRecommendedTarget,
		&OutRejectDetail);
}

void FWacomPlayerControllerRunInteractionTestAccess::PrepareExplorationRunFirstPersonCardLayer(
	AWacomPlayerControllerProbe* PC)
{
	if (PC)
	{
		PC->PrepareExplorationRunFirstPersonCardLayerForTest();
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
