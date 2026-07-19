// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Actors/WacomRunTunnelPaperLayerActor.h"

struct FWacomRunTunnelPaperLayerActorTestAccess
{
	static void RestoreAuthoredMaterialForSerialization(
		AWacomRunTunnelPaperLayerActor& Actor)
	{
		Actor.RestoreAuthoredMaterialForSerialization();
	}
};
