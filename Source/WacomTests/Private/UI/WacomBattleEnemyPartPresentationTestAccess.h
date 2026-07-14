// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/WacomBattleEnemyPartPresentationComponent.h"

/** WacomTests-private access to deterministic component playback advancement. */
struct FWacomBattleEnemyPartPresentationTestAccess
{
	static void TickCuePlayback(
		UWacomBattleEnemyPartPresentationComponent& Component,
		float DeltaSeconds)
	{
		Component.TickComponent(DeltaSeconds, LEVELTICK_All, nullptr);
	}

	static void SetAccessibility(
		UWacomBattleEnemyPartPresentationComponent& Component,
		float DecorativeIntensity,
		bool bSimplifiedMotion)
	{
		Component.RuntimeDecorativeFlashIntensityScale = DecorativeIntensity;
		Component.bRuntimeSimplifiedMotion = bSimplifiedMotion;
	}
};
