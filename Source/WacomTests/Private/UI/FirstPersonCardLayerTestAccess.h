// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

#if WITH_AUTOMATION_TESTS

struct FWacomFirstPersonCardLayerTestAccess
{
	static FWacomFirstPersonCardLayerAutomationTestView View(
		const UWacomFirstPersonCardLayerWidget& Layer);
	static FWacomFirstPersonCardSlotAutomationTestView View(
		const UWacomFirstPersonCardLayerSlotWidget& Slot);
	static FWacomFirstPersonCardAnchorAutomationTestView View(
		const UWacomFirstPersonCardAnchorComponent& Anchor);
	static void TickAnchor(UWacomFirstPersonCardAnchorComponent& Anchor, float DeltaTime);
	static void SetCardViewRetainerEffectMaterialBeforeSlate(
		UWacomFirstPersonCardViewWidget& CardView,
		UMaterialInterface* Material);
	static const UMaterialInterface* CardViewRetainerEffectMaterialInterface(
		UWacomFirstPersonCardViewWidget& CardView);

	static FWacomRunFirstPersonCardSourceRefreshCountersForTest DefaultSourceCounters(
		const UWacomRunFirstPersonCardSourceComponent& Source);
	static FWacomRunFirstPersonCardSourceRefreshCountersForTest ProviderLeaseCounters(
		const UWacomRunFirstPersonCardSourceComponent& Source);
	static void ResetSourceCounters(UWacomRunFirstPersonCardSourceComponent& Source);
	static void SetActiveProviderLeaseRequest(
		UWacomRunFirstPersonCardSourceComponent& Source,
		const FWacomRunMenuCardLeaseRequest& Request);

	static UWacomFirstPersonCardLayerWidget* CardLayer(
		const UWacomFirstPersonCardAnchorComponent& Anchor);
	static void SetCardLayer(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		UWacomFirstPersonCardLayerWidget* Layer);
	static void SetHoveredCardInstanceId(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FGuid& CardInstanceId);
	static void ResetAnchorScreenSmoothing(UWacomFirstPersonCardAnchorComponent& Anchor);
	static void SetRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries);
	static void SetRuntimeCardLayerPresentationFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);
	static void SetRuntimeCardLayerPresentationFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	static void SetRuntimeCardLayerTransitionHints(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints);
	static void SetRuntimeCardLayerTransitionPresentationEnabled(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		bool bEnabled);
	static void SetRuntimeCardLayerData(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomCardViewData>& Cards);
	static void ClearRuntimeCardLayerData(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId);
	static void CommitRuntimeCardLayerFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);
	static void SetFirstPersonCardLayerInteractionEnabled(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		bool bEnabled);

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
	static bool HandleSlotPointerEnteredAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& WidgetPosition);
	static bool HandleSlotPointerMovedAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& WidgetPosition);
	static EWacomFirstPersonCardPointerRouteAction HandleSlotPointerMovedRouteActionAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		UWacomFirstPersonCardLayerSlotWidget& SourceSlot,
		const FVector2D& WidgetPosition);
	static bool RequestPressAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);
	static EWacomFirstPersonCardPointerRouteAction RequestPressRouteActionAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);
	static bool RequestReleaseAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);
	static EWacomFirstPersonCardPointerRouteAction RequestReleaseRouteActionAtWidgetPosition(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FVector2D& WidgetPosition);

	static bool RequestHover(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static void RequestUnhover(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static bool RequestPress(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static bool RequestMouseUp(UWacomFirstPersonCardLayerSlotWidget& Slot);
	static void TickSlotMotion(UWacomFirstPersonCardLayerSlotWidget& Slot, float DeltaTime);
	static void TickSlotMotionWithoutPresentationPaint(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		float DeltaTime);
	static void AcknowledgePendingPresentationPaint(
		UWacomFirstPersonCardLayerSlotWidget& Slot);
	static void SetLocalHitCanvasSizeOverride(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const TOptional<FVector2D>& Size);
	static bool RequestHoverAtLocalPosition(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& LocalPosition);
	static void RequestMoveAtLocalPosition(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& LocalPosition);
	static void SetCardDepthPointerPosition(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		const FVector2D& WidgetPosition);
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
	static void SetGestureState(
		UWacomFirstPersonCardLayerSlotWidget& Slot,
		EWacomFirstPersonCardGestureState State);
};

#endif
