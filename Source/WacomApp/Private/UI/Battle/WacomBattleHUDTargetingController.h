// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattleHUD.h"

class FWacomBattleHUDRuntime;
struct FWacomInteractionTargetHandle;

class FWacomBattleHUDTargetingController
{
public:
	explicit FWacomBattleHUDTargetingController(FWacomBattleHUDRuntime& InRuntime);

	void HandleEnemyPartClicked(const FWacomInteractionTargetHandle& TargetHandle);
	void CancelTargetSelect();
	FBattleTargetSelectionView BuildTargetSelectionView() const;
	void ClearTargetSelection();

private:
	FWacomBattleHUDRuntime& Runtime;
};
