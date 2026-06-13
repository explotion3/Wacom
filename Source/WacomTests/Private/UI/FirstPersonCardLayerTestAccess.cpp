// Copyright Wacom. All Rights Reserved.

#include "UI/FirstPersonCardLayerTestAccess.h"

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

void FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(
	UWacomFirstPersonCardLayerSlotWidget& Slot,
	float DeltaTime)
{
	Slot.TickSlotMotionForTest(DeltaTime);
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

#endif
