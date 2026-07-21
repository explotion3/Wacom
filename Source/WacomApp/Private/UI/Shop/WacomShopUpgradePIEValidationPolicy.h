// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"

struct FWacomShopUpgradePIEValidationFacts
{
	bool bIsPIEWorld = false;
	bool bRunActive = false;
	FName JourneyId = NAME_None;
	FWacomMapNodeHandle CurrentNode;
	ERunExplorationActivityKind ActiveActivityKind = ERunExplorationActivityKind::None;
};

/** Pure policy seam shared by the Editor-only console command and automation tests. */
WACOMAPP_API bool CanSeedShopUpgradePIEValidation(
	const FWacomShopUpgradePIEValidationFacts& Facts,
	FName& OutDisabledReason);
