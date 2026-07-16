// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomEncounterSceneLifecycleAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WacomResolvedEncounterSceneRetirementPolicy.h"

bool FWacomEncounterSceneLifecycleAutomationTestView::ShouldRetireResolvedEncounterScene(
	const bool bSettlementSucceeded,
	const EBattleOutcome Outcome,
	const bool bWithdrawn)
{
	return WacomResolvedEncounterSceneRetirementPolicy::ShouldRetire(
		bSettlementSucceeded,
		Outcome,
		bWithdrawn);
}

#endif
