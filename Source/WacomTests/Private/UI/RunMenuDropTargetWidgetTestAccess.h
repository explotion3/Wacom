// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UObject;
class UWacomRunMenuDropTargetWidgetProbe;

struct FWacomRunMenuDropTargetWidgetTestAccess
{
	static UWacomRunMenuDropTargetWidgetProbe* MakeProbe(
		UObject* Outer,
		FName ZoneId,
		bool bProbeHit = true);

	static void SetProbeHit(
		UWacomRunMenuDropTargetWidgetProbe* Probe,
		bool bProbeHit);

	static FVector2D LastWidgetPosition(
		const UWacomRunMenuDropTargetWidgetProbe* Probe);
};
