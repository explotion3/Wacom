// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"

class AWacomPlayerController;

/**
 * App-private Battle scene world-target router.
 *
 * Keeps PlayerController as the input facade while isolating the Battle-specific
 * hit/probe/HUD-registry routing rules from the rest of the controller.
 */
class FWacomBattleSceneInteractionRouter
{
public:
	explicit FWacomBattleSceneInteractionRouter(AWacomPlayerController& InPlayerController);

	bool TryRouteTargetClick(bool bRequireTargetSelect);
	bool TryProbeInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const;
	bool TryProbeInteractionTargetAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const;

private:
	AWacomPlayerController& PlayerController;
};
