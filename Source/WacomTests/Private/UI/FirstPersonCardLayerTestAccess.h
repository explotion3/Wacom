// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"

#if WITH_AUTOMATION_TESTS

struct FWacomFirstPersonCardLayerTestAccess
{
	static FWacomFirstPersonCardLayerAutomationTestView View(
		const UWacomFirstPersonCardLayerWidget& Layer);
	static FWacomFirstPersonCardSlotAutomationTestView View(
		const UWacomFirstPersonCardLayerSlotWidget& Slot);
	static FWacomFirstPersonCardAnchorAutomationTestView View(
		const UWacomFirstPersonCardAnchorComponent& Anchor);

	static FWacomRunFirstPersonCardSourceRefreshCountersForTest DefaultSourceCounters(
		const UWacomRunFirstPersonCardSourceComponent& Source);
	static FWacomRunFirstPersonCardSourceRefreshCountersForTest ProviderLeaseCounters(
		const UWacomRunFirstPersonCardSourceComponent& Source);
	static void ResetSourceCounters(UWacomRunFirstPersonCardSourceComponent& Source);
	static void SetActiveProviderLeaseRequest(
		UWacomRunFirstPersonCardSourceComponent& Source,
		const FWacomRunMenuCardLeaseRequest& Request);

	static UWacomFirstPersonCardLayerWidget* StaticLayer(
		const UWacomFirstPersonCardAnchorComponent& Anchor);
	static void SetHoveredCardInstanceId(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FGuid& CardInstanceId);
	static void ResetAnchorScreenSmoothing(UWacomFirstPersonCardAnchorComponent& Anchor);

	static void TickSlotMotion(UWacomFirstPersonCardLayerWidget& Layer, float DeltaTime);
	static UWacomFirstPersonCardLayerSlotWidget* FindSlotWidgetByKey(
		const UWacomFirstPersonCardLayerWidget& Layer,
		const FString& SlotKey);
	static UWacomFirstPersonCardLayerSlotWidget* OutgoingSlotAt(
		const UWacomFirstPersonCardLayerWidget& Layer,
		int32 Index);
	static void AddUntrackedSlotChild(UWacomFirstPersonCardLayerWidget& Layer);
	static void SetViewportSizeOverride(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetViewportSize);
	static FGuid ResolveHoveredCardAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);
	static bool RequestPressAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);
	static bool RequestReleaseAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);

	static bool RequestHover(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static void RequestUnhover(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static bool RequestPress(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static bool RequestClick(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static bool RequestMouseUp(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static void TickSlotMotion(UWacomFirstPersonCardLayerSlotWidget& Slot, float DeltaTime);
	static void SetLocalHitCanvasSizeOverride(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const TOptional<FVector2D>& Size);
	static bool RequestHoverAtLocalPosition(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& LocalPosition);
	static void RequestMoveAtLocalPosition(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& LocalPosition);
	static bool RequestPressAtLocalPosition(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& LocalPosition);
	static bool RequestGesturePress(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& ScreenPosition);
	static void RequestGestureMove(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		float DeltaTime,
		const FVector2D& ScreenPosition);
	static bool RequestGestureRelease(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& ScreenPosition);
};

#endif
