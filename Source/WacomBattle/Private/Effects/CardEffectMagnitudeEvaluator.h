// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleState;
struct FCardEffect;

/**
 * Shared final magnitude evaluation for card effects.
 *
 * Used by both real card execution and read-only target preview so UI previews
 * stay aligned with Battle rule truth.
 */
class FCardEffectMagnitudeEvaluator
{
public:
	static int32 ComputeFinalMagnitude(
		const FBattleState& State,
		const FCardEffect& Effect,
		int32 RuntimeCost,
		const FGuid& SelectedPartId,
		const FGuid& SelfCardId);
};
