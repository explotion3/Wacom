// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBattleHUD;
struct FBattleEvent;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;

struct FWacomBattleHUDEventFlow
{
	static bool ConsumeAndLogEvents(UBattleHUD& HUD);
	static bool ConsumeAndLogEvents(
		UBattleHUD& HUD,
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);
};
