// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

struct FBattleResultPacket;
struct FRunState;

/**
 * Production-owned automation seam for validating the private Run settlement
 * resolver without driving map-node UI lifecycle.
 */
struct WACOMRUN_API FWacomRunBattleSettlementAutomationTestView
{
	static bool Resolve(
		FRunState& State,
		const FBattleResultPacket& Packet);
};

#endif
