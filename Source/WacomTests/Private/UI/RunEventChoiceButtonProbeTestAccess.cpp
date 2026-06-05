// Copyright Wacom. All Rights Reserved.

#include "UI/RunEventChoiceButtonProbeTestAccess.h"

#include "UI/WacomShopRunEventTestProbes.h"

int32 FWacomRunEventChoiceButtonProbeTestAccess::SnapshotAppliedCount(
	const UWacomRunEventChoiceButtonClassProbe* Probe)
{
	return Probe ? Probe->SnapshotAppliedCountForTest : 0;
}

FName FWacomRunEventChoiceButtonProbeTestAccess::LastAppliedChoiceId(
	const UWacomRunEventChoiceButtonClassProbe* Probe)
{
	return Probe ? Probe->LastAppliedChoiceIdForTest : NAME_None;
}

bool FWacomRunEventChoiceButtonProbeTestAccess::HasAppliedSnapshot(
	const UWacomRunEventChoiceButtonClassProbe* Probe)
{
	return SnapshotAppliedCount(Probe) > 0;
}
