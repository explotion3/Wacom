// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"

class UBattleHUD;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;

struct FWacomBattleHUDCommandFlow
{
	static void SubmitPlayCard(UBattleHUD& HUD, const FGuid& CardId, const FGuid& TargetPartId);
	static void SubmitPlayCardOnHandCard(UBattleHUD& HUD, const FGuid& CardId, const FGuid& TargetCardId);
	static void SubmitWait(UBattleHUD& HUD);
	static void SubmitEndTurn(UBattleHUD& HUD);
	static void SubmitKnockdownChoice(UBattleHUD& HUD, EKnockdownChoice Choice);
	static void AfterCommand(UBattleHUD& HUD);
	static void AfterCommand(
		UBattleHUD& HUD,
		const FWacomBattleCombatLogCommandContext& LogContext,
		const FBattleSnapshot& PreCommandSnapshot);
};
