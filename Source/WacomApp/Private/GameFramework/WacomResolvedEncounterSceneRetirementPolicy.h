// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Types/WacomEnums.h"

namespace WacomResolvedEncounterSceneRetirementPolicy
{
	/** Only a successfully committed, non-withdrawn victory resolves the authored encounter scene. */
	inline bool ShouldRetire(
		const bool bSettlementSucceeded,
		const EBattleOutcome Outcome,
		const bool bWithdrawn)
	{
		return bSettlementSucceeded
			&& Outcome == EBattleOutcome::Victory
			&& !bWithdrawn;
	}
}
