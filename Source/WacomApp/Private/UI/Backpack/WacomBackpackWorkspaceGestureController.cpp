// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceGestureController.h"

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"

void FWacomBackpackWorkspaceGestureController::BeginCardPress(
	FGuid InstanceId,
	FVector2D LocalPosition,
	FVector2D ScreenPosition,
	bool bControlDown)
{
	CardPress.InstanceId = InstanceId;
	CardPress.LocalPosition = LocalPosition;
	CardPress.ScreenPosition = ScreenPosition;
	CardPress.bControlDown = bControlDown;
	CardPress.bActive = true;
}

void FWacomBackpackWorkspaceGestureController::BeginPilePress(
	UWacomBackpackZonePileWidget& Pile,
	FVector2D LocalPosition,
	FVector2D ScreenPosition,
	FVector2D PileStartPosition,
	bool bControlDown,
	bool bOnDragHandle)
{
	PilePress.Pile = &Pile;
	PilePress.LocalPosition = LocalPosition;
	PilePress.ScreenPosition = ScreenPosition;
	PilePress.PileStartPosition = PileStartPosition;
	PilePress.bControlDown = bControlDown;
	PilePress.bOnDragHandle = bOnDragHandle;
	PilePress.bActive = true;
}

void FWacomBackpackWorkspaceGestureController::BeginMarqueePress(
	const FWacomBackpackZoneKey& SourceZone,
	FVector2D LocalPosition,
	FVector2D ScreenPosition,
	bool bControlDown)
{
	MarqueePress.SourceZone = SourceZone;
	MarqueePress.LocalPosition = LocalPosition;
	MarqueePress.ScreenPosition = ScreenPosition;
	MarqueePress.bControlDown = bControlDown;
	MarqueePress.bActive = true;
}

bool FWacomBackpackWorkspaceGestureController::HasCardDragThreshold(
	const FPointerEvent& Event) const
{
	return CardPress.bActive && HasDragThreshold(Event, CardPress.ScreenPosition);
}

bool FWacomBackpackWorkspaceGestureController::HasPileDragThreshold(
	const FPointerEvent& Event) const
{
	return PilePress.bActive && HasDragThreshold(Event, PilePress.ScreenPosition);
}

bool FWacomBackpackWorkspaceGestureController::HasMarqueeDragThreshold(
	const FPointerEvent& Event) const
{
	return MarqueePress.bActive && HasDragThreshold(Event, MarqueePress.ScreenPosition);
}

void FWacomBackpackWorkspaceGestureController::ResetPendingPresses()
{
	CardPress.Reset();
	PilePress.Reset();
	MarqueePress.Reset();
}

void FWacomBackpackWorkspaceGestureController::Reset()
{
	ResetPendingPresses();
	PileMoveSnapshot.Reset();
}

bool FWacomBackpackWorkspaceGestureController::HasDragThreshold(
	const FPointerEvent& Event,
	FVector2D ScreenOrigin)
{
	return FSlateApplication::IsInitialized()
		&& FSlateApplication::Get().HasTraveledFarEnoughToTriggerDrag(Event, ScreenOrigin);
}
