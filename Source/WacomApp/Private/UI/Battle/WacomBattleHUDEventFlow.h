// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBattleHUD;
struct FBattleEvent;

struct FWacomBattleHUDEventFlow
{
	static void ConsumeAndLogEvents(UBattleHUD& HUD);
	static void AppendBattleEventLogEntries(UBattleHUD& HUD, const TArray<FBattleEvent>& Events);
	static void TrimBattleEventLogHistory(UBattleHUD& HUD);
	static void SyncBattleEventLogPanel(UBattleHUD& HUD);
};
