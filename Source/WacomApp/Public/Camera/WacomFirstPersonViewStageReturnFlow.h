// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class APlayerController;
class AWacomPlayerCharacter;

/**
 * Reusable App-layer flow for leaving a temporary first-person stage and
 * returning to the active Run Tunnel view.
 */
struct WACOMAPP_API FWacomFirstPersonViewStageReturnFlow
{
	/**
	 * Stages the first-person view back to the current Run Tunnel pose, then
	 * resumes exploration input.
	 *
	 * Returns true when the return is deferred by an active blend. When false is
	 * returned, exploration input has been restored synchronously.
	 */
	static bool ReturnToRunTunnel(
		AWacomPlayerCharacter& Pawn,
		APlayerController& PlayerController,
		TFunction<void()>&& OnCompleted = nullptr);
};
