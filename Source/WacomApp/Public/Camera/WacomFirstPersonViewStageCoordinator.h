// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Camera/WacomFirstPersonViewStageRequest.h"

class APlayerController;
class AWacomPlayerCharacter;

/**
 * App-layer consumer for first-person view stage requests.
 *
 * The coordinator owns the presentation handoff from a requested view pose to
 * an active camera mode. Producers only build FWacomFirstPersonViewStageRequest.
 */
struct WACOMAPP_API FWacomFirstPersonViewStageCoordinator
{
	/**
	 * Applies or blends to a first-person view stage request.
	 *
	 * Returns true when completion is deferred by an active blend. When false is
	 * returned, staging is complete by the time the function returns and the
	 * deferred callback is not invoked.
	 */
	static bool StageFirstPersonView(
		AWacomPlayerCharacter& Pawn,
		APlayerController& PlayerController,
		const FWacomFirstPersonViewStageRequest& Request,
		TFunction<void()>&& OnDeferredCompleted = nullptr);

	/**
	 * Stages the first-person view, then activates battle camera look.
	 *
	 * Returns true when battle camera activation is deferred until blend
	 * completion. When false is returned, battle camera look has already been
	 * activated or activation failed synchronously.
	 */
	static bool StageFirstPersonViewAndActivateBattleCameraLook(
		AWacomPlayerCharacter& Pawn,
		APlayerController& PlayerController,
		const FWacomFirstPersonViewStageRequest& Request,
		TFunction<void()>&& OnDeferredBattleCameraActivated = nullptr);

	static bool ActivateBattleCameraLook(AWacomPlayerCharacter& Pawn);

	static bool ActivateBattleCameraLookForStageRequest(
		AWacomPlayerCharacter& Pawn,
		const FWacomFirstPersonViewStageRequest& Request);

	static void CancelActiveStage(AWacomPlayerCharacter& Pawn);
};
