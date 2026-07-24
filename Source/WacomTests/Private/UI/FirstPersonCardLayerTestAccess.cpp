// Copyright Wacom. All Rights Reserved.

#include "UI/FirstPersonCardLayerTestAccess.h"

#include "Components/CanvasPanel.h"
#include "Components/RetainerBox.h"

#if WITH_AUTOMATION_TESTS

FWacomFirstPersonCardLayerAutomationTestView FWacomFirstPersonCardLayerTestAccess::View(
	const UWacomFirstPersonCardLayerWidget& Layer)
{
	return Layer.GetAutomationTestViewForTest();
}

FWacomFirstPersonCardSlotAutomationTestView FWacomFirstPersonCardLayerTestAccess::View(
	const UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	return Slot.GetAutomationTestViewForTest();
}

FWacomFirstPersonCardAnchorAutomationTestView FWacomFirstPersonCardLayerTestAccess::View(
	const UWacomFirstPersonCardAnchorComponent& Anchor)
{
	return Anchor.GetAutomationTestViewForTest();
}

void FWacomFirstPersonCardLayerTestAccess::TickAnchor(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	const float DeltaTime)
{
	Anchor.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
}

void FWacomFirstPersonCardLayerTestAccess::SetCardViewRetainerEffectMaterialBeforeSlate(
	UWacomFirstPersonCardViewWidget& CardView,
	UMaterialInterface* Material)
{
	CardView.EnsureFallbackWidgetTree();
	if (CardView.Fake3DSurfaceRetainer)
	{
		CardView.Fake3DSurfaceRetainer->SetEffectMaterial(Material);
	}
}

const UMaterialInterface* FWacomFirstPersonCardLayerTestAccess::CardViewRetainerEffectMaterialInterface(
	UWacomFirstPersonCardViewWidget& CardView)
{
	CardView.EnsureFallbackWidgetTree();
	return CardView.Fake3DSurfaceRetainer
		? CardView.Fake3DSurfaceRetainer->GetEffectMaterialInterface()
		: nullptr;
}

bool FWacomFirstPersonCardLayerTestAccess::ResolveCenteredCardBodyUVRect(
	const FVector2D& SurfaceSize,
	const FVector2D& CardBodySize,
	FLinearColor& OutMin,
	FLinearColor& OutMax)
{
	return UWacomFirstPersonCardViewWidget::ResolveCenteredCardBodyUVRect(
		SurfaceSize,
		CardBodySize,
		OutMin,
		OutMax);
}

FWacomRunFirstPersonCardSourceRefreshCountersForTest FWacomFirstPersonCardLayerTestAccess::DefaultSourceCounters(
	const UWacomRunFirstPersonCardSourceComponent& Source)
{
	return Source.GetDefaultSourceRefreshCountersForTest();
}

FWacomRunFirstPersonCardSourceRefreshCountersForTest FWacomFirstPersonCardLayerTestAccess::ProviderLeaseCounters(
	const UWacomRunFirstPersonCardSourceComponent& Source)
{
	return Source.GetProviderLeaseRefreshCountersForTest();
}

void FWacomFirstPersonCardLayerTestAccess::ResetSourceCounters(
	UWacomRunFirstPersonCardSourceComponent& Source)
{
	Source.ResetRunFirstPersonCardSourcePerfCountersForTest();
}

void FWacomFirstPersonCardLayerTestAccess::SetActiveProviderLeaseRequest(
	UWacomRunFirstPersonCardSourceComponent& Source,
	const FWacomRunMenuCardLeaseRequest& Request)
{
	Source.SetActiveProviderLeaseRequestForTest(Request);
}

UWacomFirstPersonCardLayerWidget* FWacomFirstPersonCardLayerTestAccess::CardLayer(
	const UWacomFirstPersonCardAnchorComponent& Anchor)
{
	return Anchor.GetAutomationTestViewForTest().CardLayerWidget;
}

void FWacomFirstPersonCardLayerTestAccess::SetCardLayer(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	UWacomFirstPersonCardLayerWidget* Layer)
{
	Anchor.SetCardLayerWidgetForTest(Layer);
}

void FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	const FGuid& CardInstanceId)
{
	Anchor.SetHoveredCardInstanceIdForTest(CardInstanceId);
}

void FWacomFirstPersonCardLayerTestAccess::ResetAnchorScreenSmoothing(
	UWacomFirstPersonCardAnchorComponent& Anchor)
{
	Anchor.ResetAnchorScreenSmoothingForTest();
}

void FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	Anchor.SetRuntimeCardLayerEntries(SourceId, Entries);
}

void FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerPresentationFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	Anchor.SetRuntimeCardLayerPresentationFrame(Frame);
}

void FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerPresentationFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	Anchor.SetRuntimeCardLayerPresentationFrame(
		SourceId,
		Entries,
		TransitionHints);
}

void FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerTransitionHints(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints)
{
	Anchor.SetRuntimeCardLayerTransitionHints(SourceId, Hints);
}

void FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerTransitionPresentationEnabled(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	bool bEnabled)
{
	Anchor.SetRuntimeCardLayerTransitionPresentationEnabled(SourceId, bEnabled);
}

void FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerData(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId,
	const TArray<FWacomCardViewData>& Cards)
{
	Anchor.SetRuntimeCardLayerData(SourceId, Cards);
}

void FWacomFirstPersonCardLayerTestAccess::ClearRuntimeCardLayerData(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	FName SourceId)
{
	Anchor.ClearRuntimeCardLayerData(SourceId);
}

void FWacomFirstPersonCardLayerTestAccess::CommitRuntimeCardLayerFrame(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	const FWacomFirstPersonCardLayerPresentationFrame& Frame)
{
	Anchor.CommitRuntimeCardLayerFrame(Frame);
}

void FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(
	UWacomFirstPersonCardAnchorComponent& Anchor,
	bool bEnabled)
{
	Anchor.SetFirstPersonCardLayerInteractionEnabled(bEnabled);
}

void FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(
	UWacomFirstPersonCardLayerWidget& Layer,
	float DeltaTime)
{
	Layer.TickSlotMotionForTest(DeltaTime);
}

UWacomFirstPersonCardLayerSlotWidget* FWacomFirstPersonCardLayerTestAccess::FindSlotWidgetByKey(
	const UWacomFirstPersonCardLayerWidget& Layer,
	const FString& SlotKey)
{
	return Layer.FindSlotWidgetByKeyForTest(SlotKey);
}

UWacomFirstPersonCardLayerSlotWidget* FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(
	const UWacomFirstPersonCardLayerWidget& Layer,
	int32 Index)
{
	return Layer.GetOutgoingSlotWidgetAtForTest(Index);
}

void FWacomFirstPersonCardLayerTestAccess::AddUntrackedSlotChild(
	UWacomFirstPersonCardLayerWidget& Layer)
{
	Layer.AddUntrackedSlotChildForTest();
}

void FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FVector2D& WidgetViewportSize)
{
	Layer.SetViewportSizeOverrideForTest(WidgetViewportSize);
}

void FWacomFirstPersonCardLayerTestAccess::TickWorldActivitySuppression(
	UWacomFirstPersonCardLayerWidget& Layer,
	float DeltaTime)
{
	Layer.TickWorldActivitySuppressionForTest(DeltaTime);
}

FVector2D FWacomFirstPersonCardLayerTestAccess::WorldActivitySuppressionRenderTranslation(
	const UWacomFirstPersonCardLayerWidget& Layer)
{
	return Layer.RootCanvas
		? Layer.RootCanvas->GetRenderTransform().Translation
		: FVector2D::ZeroVector;
}

float FWacomFirstPersonCardLayerTestAccess::WorldActivitySuppressionRenderOpacity(
	const UWacomFirstPersonCardLayerWidget& Layer)
{
	return Layer.RootCanvas
		? Layer.RootCanvas->GetRenderOpacity()
		: 1.0f;
}

FGuid FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FVector2D& WidgetPosition)
{
	return Layer.ResolveHoveredCardAtWidgetPositionForTest(WidgetPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& WidgetPosition)
{
	return Layer.HandleSlotPointerEnteredAtWidgetPositionForTest(SourceSlot, WidgetPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& WidgetPosition)
{
	return Layer.HandleSlotPointerMovedAtWidgetPositionForTest(SourceSlot, WidgetPosition);
}

EWacomFirstPersonCardPointerRouteAction
FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedRouteActionAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
	const FVector2D& WidgetPosition)
{
	return Layer.HandleSlotPointerMovedRouteActionAtWidgetPositionForTest(SourceSlot, WidgetPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FVector2D& WidgetPosition)
{
	return Layer.RequestPressAtWidgetPositionForTest(WidgetPosition);
}

EWacomFirstPersonCardPointerRouteAction FWacomFirstPersonCardLayerTestAccess::RequestPressRouteActionAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FVector2D& WidgetPosition)
{
	return Layer.RequestPressRouteActionAtWidgetPositionForTest(WidgetPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestReleaseAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FVector2D& WidgetPosition)
{
	return Layer.RequestReleaseAtWidgetPositionForTest(WidgetPosition);
}

EWacomFirstPersonCardPointerRouteAction
FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FVector2D& WidgetPosition)
{
	return Layer.RequestReleaseRouteActionAtWidgetPositionForTest(WidgetPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestHover(
	UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	return Slot.RequestHoverForTest();
}

void FWacomFirstPersonCardLayerTestAccess::RequestUnhover(
	UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	Slot.RequestUnhoverForTest();
}

bool FWacomFirstPersonCardLayerTestAccess::RequestPress(
	UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	return Slot.RequestPressForTest();
}

bool FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(
	UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	return Slot.RequestMouseUpForTest();
}

void FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FWacomFirstPersonCardSlotMotionConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Layer).SlotRuntimeConfig;
	RuntimeConfig.Motion = Config;
	Layer.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FWacomFirstPersonCardSlotMotionConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Slot).SlotRuntimeConfig;
	RuntimeConfig.Motion = Config;
	Slot.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FWacomFirstPersonCardSlotVisualConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Layer).SlotRuntimeConfig;
	RuntimeConfig.Visual = Config;
	Layer.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FWacomFirstPersonCardSlotVisualConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Slot).SlotRuntimeConfig;
	RuntimeConfig.Visual = Config;
	Slot.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FWacomFirstPersonCardInteractionFeedbackConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Layer).SlotRuntimeConfig;
	RuntimeConfig.Interaction = Config;
	Layer.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FWacomFirstPersonCardInteractionFeedbackConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Slot).SlotRuntimeConfig;
	RuntimeConfig.Interaction = Config;
	Slot.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetDragPickupConfig(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FWacomFirstPersonCardDragPickupConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Slot).SlotRuntimeConfig;
	RuntimeConfig.DragPickup = Config;
	Slot.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
	UWacomFirstPersonCardLayerWidget& Layer,
	const FWacomFirstPersonCardDragConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Layer).SlotRuntimeConfig;
	RuntimeConfig.Drag = Config;
	Layer.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FWacomFirstPersonCardDragConfig& Config)
{
	FWacomFirstPersonCardSlotRuntimeConfig RuntimeConfig = View(Slot).SlotRuntimeConfig;
	RuntimeConfig.Drag = Config;
	Slot.SetSlotRuntimeConfig(RuntimeConfig);
}

void FWacomFirstPersonCardLayerTestAccess::TriggerDenyFeedback(
	UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	Slot.TriggerDenyFeedback();
}

void FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	float DeltaTime)
{
	const UWacomFirstPersonCardViewWidget* CardView = Slot.CardView;
	const bool bHadUnpaintedPresentationGeneration = CardView
		&& ((CardView->SurfaceRequestedGeneration != 0
				&& CardView->SurfacePaintedGeneration != CardView->SurfaceRequestedGeneration)
			|| (CardView->CostDigitRequestedGeneration != 0
				&& CardView->CostDigitPaintedGeneration != CardView->CostDigitRequestedGeneration)
			|| (CardView->EffectBadgeRequestedGeneration != 0
				&& CardView->EffectBadgePaintedGeneration != CardView->EffectBadgeRequestedGeneration));
	AcknowledgePendingPresentationPaint(Slot);
	if (bHadUnpaintedPresentationGeneration)
	{
		// Mirror the production contract: the first Tick after a real Paint only
		// consumes the start edge with DeltaTime zero. Unrelated motion ticks must
		// not receive an extra zero-delta sample because that changes drag velocity.
		Slot.TickSlotMotionForTest(0.0f);
	}
	Slot.TickSlotMotionForTest(DeltaTime);
}

void FWacomFirstPersonCardLayerTestAccess::TickSlotMotionWithoutPresentationPaint(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	float DeltaTime)
{
	Slot.TickSlotMotionForTest(DeltaTime);
}

void FWacomFirstPersonCardLayerTestAccess::AcknowledgePendingPresentationPaint(
	UWacomFirstPersonCardLayerSlotWidget& Slot)
{
	UWacomFirstPersonCardViewWidget* CardView = Slot.CardView;
	if (!CardView)
	{
		return;
	}
	if (CardView->SurfaceRequestedGeneration != 0)
	{
		CardView->SurfaceMaterialReadyGeneration = CardView->SurfaceRequestedGeneration;
		CardView->SurfacePaintedGeneration = CardView->SurfaceRequestedGeneration;
	}
	if (CardView->CostDigitRequestedGeneration != 0)
	{
		CardView->CostDigitMaterialReadyGeneration = CardView->CostDigitRequestedGeneration;
		CardView->CostDigitPaintedGeneration = CardView->CostDigitRequestedGeneration;
	}
	if (CardView->EffectBadgeRequestedGeneration != 0)
	{
		CardView->EffectBadgeMaterialReadyGeneration = CardView->EffectBadgeRequestedGeneration;
		CardView->EffectBadgePaintedGeneration = CardView->EffectBadgeRequestedGeneration;
	}
}

void FWacomFirstPersonCardLayerTestAccess::SetLocalHitCanvasSizeOverride(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const TOptional<FVector2D>& Size)
{
	Slot.SetLocalHitCanvasSizeOverrideForTest(Size);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestHoverAtLocalPosition(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FVector2D& LocalPosition)
{
	return Slot.RequestHoverAtLocalPositionForTest(LocalPosition);
}

void FWacomFirstPersonCardLayerTestAccess::RequestMoveAtLocalPosition(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FVector2D& LocalPosition)
{
	Slot.RequestMoveAtLocalPositionForTest(LocalPosition);
}

void FWacomFirstPersonCardLayerTestAccess::SetCardDepthPointerPosition(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FVector2D& WidgetPosition)
{
	Slot.SetCardDepthPointerPositionForTest(WidgetPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestPressAtLocalPosition(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FVector2D& LocalPosition)
{
	return Slot.RequestPressAtLocalPositionForTest(LocalPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FVector2D& ScreenPosition)
{
	return Slot.RequestGesturePressForTest(ScreenPosition);
}

void FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	float DeltaTime,
	const FVector2D& ScreenPosition)
{
	Slot.RequestGestureMoveForTest(DeltaTime, ScreenPosition);
}

bool FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	const FVector2D& ScreenPosition)
{
	return Slot.RequestGestureReleaseForTest(ScreenPosition);
}

void FWacomFirstPersonCardLayerTestAccess::SetGestureState(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	EWacomFirstPersonCardGestureState State)
{
	Slot.SetGestureState(State, false);
}

#endif
