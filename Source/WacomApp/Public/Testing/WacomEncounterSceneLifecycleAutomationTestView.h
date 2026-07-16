// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Non-reflected automation seam for the App-private resolved Encounter scene policy. */
struct WACOMAPP_API FWacomEncounterSceneLifecycleAutomationTestView
{
	static bool ShouldRetireResolvedEncounterScene(
		bool bSettlementSucceeded,
		EBattleOutcome Outcome,
		bool bWithdrawn);
};

#endif
