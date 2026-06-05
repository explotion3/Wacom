// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunEventChoiceButtonClassProbe;

struct FWacomRunEventChoiceButtonProbeTestAccess
{
	static int32 SnapshotAppliedCount(
		const UWacomRunEventChoiceButtonClassProbe* Probe);

	static FName LastAppliedChoiceId(
		const UWacomRunEventChoiceButtonClassProbe* Probe);

	static bool HasAppliedSnapshot(
		const UWacomRunEventChoiceButtonClassProbe* Probe);
};
