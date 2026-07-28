// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleCardEffectMagnitudeSnapshot;
struct FBattleState;
struct FRuntimeCardInstance;

namespace WacomBattleCardRuntimeSnapshotBuilder
{
	/**
	 * Projects current source-card/runtime modifiers without a concrete target
	 * and without consuming critical RNG.
	 */
	void BuildCurrentEffectMagnitudes(
		const FBattleState& State,
		const FRuntimeCardInstance& Card,
		TArray<FBattleCardEffectMagnitudeSnapshot>& OutMagnitudes);
}
