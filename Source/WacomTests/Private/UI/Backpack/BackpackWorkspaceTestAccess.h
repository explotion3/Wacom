// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

struct FWacomBackpackWorkspaceAutomationTestView;

/** WacomTests/Private 适配层：只消费 production automation view。 */
struct FWacomBackpackWorkspaceTestAccess
{
	static FGuid CurrentCarriedInstanceId(
		const FWacomBackpackWorkspaceAutomationTestView& View);

	static bool HasStableCarryIndices(
		const FWacomBackpackWorkspaceAutomationTestView& View);
};

#endif
