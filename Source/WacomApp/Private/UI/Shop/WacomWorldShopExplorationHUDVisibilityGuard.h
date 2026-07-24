// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/SlateWrapperTypes.h"
#include "CoreMinimal.h"

class AWacomPlayerController;
class UWacomExplorationHUD;

/**
 * World Shop 活动期间只折叠 ExplorationHUD 的可逆守卫。
 *
 * 不 Deactivate、不 Pop CommonUI Widget，因此 GameAndUI 输入配置继续由原 HUD
 * 持有。商店 HUD、Toast 和其它 Layer 不在本守卫职责内。
 */
class WACOMAPP_API FWacomWorldShopExplorationHUDVisibilityGuard
{
public:
	~FWacomWorldShopExplorationHUDVisibilityGuard();

	bool SuppressForPlayerController(AWacomPlayerController& PlayerController);
	bool Suppress(UWacomExplorationHUD& HUD);
	void Restore();

	bool IsSuppressing() const { return ExplorationHUD.IsValid(); }

private:
	TWeakObjectPtr<UWacomExplorationHUD> ExplorationHUD;
	ESlateVisibility PreviousVisibility = ESlateVisibility::Visible;
};
