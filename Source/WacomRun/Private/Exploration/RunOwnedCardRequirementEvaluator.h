// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FRunState;
struct FWacomOwnedCardRequirement;

/** Floor Entrance 的真实持有卡条件求值器；只读全部物理持有区。 */
class FRunOwnedCardRequirementEvaluator
{
public:
	static bool AreAllSatisfied(
		const FRunState& State,
		const TArray<FWacomOwnedCardRequirement>& Requirements);
};
