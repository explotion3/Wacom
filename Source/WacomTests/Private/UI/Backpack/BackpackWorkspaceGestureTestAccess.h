// Copyright Wacom. All Rights Reserved.

#pragma once

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceGestureController.h"

/** Test-only driver for private pending-gesture state. */
struct FWacomBackpackWorkspaceGestureTestAccess
{
	static void BeginCardPress(
		FWacomBackpackWorkspaceGestureController& Gesture,
		const FGuid InstanceId,
		const FVector2D LocalPosition,
		const FVector2D ScreenPosition,
		const bool bControlDown)
	{
		Gesture.BeginCardPress(
			InstanceId,
			LocalPosition,
			ScreenPosition,
			bControlDown);
	}

	static void BeginPilePress(
		FWacomBackpackWorkspaceGestureController& Gesture,
		UWacomBackpackZonePileWidget& Pile,
		const FVector2D LocalPosition,
		const FVector2D ScreenPosition,
		const FVector2D PileStartPosition,
		const bool bControlDown,
		const bool bOnDragHandle)
	{
		Gesture.BeginPilePress(
			Pile,
			LocalPosition,
			ScreenPosition,
			PileStartPosition,
			bControlDown,
			bOnDragHandle);
	}

	static void BeginMarqueePress(
		FWacomBackpackWorkspaceGestureController& Gesture,
		const FWacomBackpackZoneKey& SourceZone,
		const FVector2D LocalPosition,
		const FVector2D ScreenPosition,
		const bool bControlDown)
	{
		Gesture.BeginMarqueePress(
			SourceZone,
			LocalPosition,
			ScreenPosition,
			bControlDown);
	}
};

#endif
