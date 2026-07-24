// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomPlayerController;
class UWacomFirstPersonCardAnchorComponent;

enum class EWacomFirstPersonCardInputAdapter : uint8
{
	PlayerControllerInputKey,
	SlatePreprocessor,
	ViewportReroute
};

struct FWacomFirstPersonCardInputEvent
{
	FKey Key;
	EInputEvent Event = IE_Pressed;
	EWacomFirstPersonCardInputAdapter Adapter =
		EWacomFirstPersonCardInputAdapter::PlayerControllerInputKey;
	TOptional<FVector2D> AbsoluteScreenPosition;
};

/**
 * App-private first-person card input arbitrator.
 *
 * This class owns routing policy only. The active gesture, locked inspection,
 * hit testing and matching-release reservation remain owned by the card
 * Layer/Slot hierarchy.
 */
class FWacomFirstPersonCardInputRouter
{
public:
	explicit FWacomFirstPersonCardInputRouter(
		AWacomPlayerController& InPlayerController);

	bool RouteInput(const FWacomFirstPersonCardInputEvent& Input);
	void PumpActivePointer();
	bool TryStartBattleHandShortcut(int32 OneBasedIndex);
	bool TryCancelForTurnBoundary();
	void ResetTransientState(bool bBroadcastCancel);

private:
	UWacomFirstPersonCardAnchorComponent* ResolveAnchor() const;
	bool TryToggleLockedInspection();
	bool TryCloseLockedInspection();
	bool TryRouteLockedPointerPress(const FVector2D& AbsoluteScreenPosition);
	bool TryConsumeLockedPointerRelease();
	bool TryReleaseActiveDrag();
	bool TryReleaseKeyboardShortcutDrag();
	bool TryCancelKeyboardShortcutDrag();

	AWacomPlayerController& PlayerController;
};
