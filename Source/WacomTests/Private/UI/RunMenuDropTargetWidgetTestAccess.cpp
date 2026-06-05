// Copyright Wacom. All Rights Reserved.

#include "UI/RunMenuDropTargetWidgetTestAccess.h"

#include "UI/WacomShopRunEventTestProbes.h"

UWacomRunMenuDropTargetWidgetProbe*
FWacomRunMenuDropTargetWidgetTestAccess::MakeProbe(
	UObject* Outer,
	FName ZoneId,
	bool bProbeHit)
{
	UWacomRunMenuDropTargetWidgetProbe* Probe =
		NewObject<UWacomRunMenuDropTargetWidgetProbe>(Outer);
	if (Probe)
	{
		Probe->ZoneId = ZoneId;
		Probe->bProbeHitForTest = bProbeHit;
	}
	return Probe;
}

void FWacomRunMenuDropTargetWidgetTestAccess::SetProbeHit(
	UWacomRunMenuDropTargetWidgetProbe* Probe,
	bool bProbeHit)
{
	if (Probe)
	{
		Probe->bProbeHitForTest = bProbeHit;
	}
}

FVector2D FWacomRunMenuDropTargetWidgetTestAccess::LastWidgetPosition(
	const UWacomRunMenuDropTargetWidgetProbe* Probe)
{
	return Probe ? Probe->GetLastWidgetPositionForTest() : FVector2D::ZeroVector;
}
