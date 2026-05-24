// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleTargetSelectionView;
class UBattleHUD;

struct FWacomBattleHUDTargetingFlow
{
	static void HandleCardClicked(UBattleHUD& HUD, const FGuid& CardInstanceId);
	static void HandleEnemyPartClicked(UBattleHUD& HUD, const FGuid& PartInstanceId);
	static void CancelTargetSelect(UBattleHUD& HUD);
	static FBattleTargetSelectionView BuildTargetSelectionView(const UBattleHUD& HUD);
	static void ClearTargetSelection(UBattleHUD& HUD);
};
