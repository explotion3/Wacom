// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopExplorationHUDVisibilityGuard.h"

#include "Engine/GameInstance.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

FWacomWorldShopExplorationHUDVisibilityGuard::
	~FWacomWorldShopExplorationHUDVisibilityGuard()
{
	Restore();
}

bool FWacomWorldShopExplorationHUDVisibilityGuard::
	SuppressForPlayerController(AWacomPlayerController& PlayerController)
{
	UWacomGameUIManagerSubsystem* UIManager =
		PlayerController.GetGameInstance()
			? PlayerController.GetGameInstance()
				->GetSubsystem<UWacomGameUIManagerSubsystem>()
			: nullptr;
	UWacomPrimaryGameLayout* Layout =
		UIManager ? UIManager->GetPrimaryLayout() : nullptr;
	UCommonActivatableWidgetStack* GameStack =
		Layout
			? Layout->GetLayerStack(WacomUITags::UI_Layer_Game.GetTag())
			: nullptr;
	UWacomExplorationHUD* HUD =
		GameStack
			? Cast<UWacomExplorationHUD>(GameStack->GetActiveWidget())
			: nullptr;
	return HUD ? Suppress(*HUD) : false;
}

bool FWacomWorldShopExplorationHUDVisibilityGuard::Suppress(
	UWacomExplorationHUD& HUD)
{
	if (ExplorationHUD.Get() == &HUD)
	{
		return true;
	}

	Restore();
	ExplorationHUD = &HUD;
	PreviousVisibility = HUD.GetVisibility();
	HUD.SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

void FWacomWorldShopExplorationHUDVisibilityGuard::Restore()
{
	if (UWacomExplorationHUD* HUD = ExplorationHUD.Get())
	{
		HUD->SetVisibility(PreviousVisibility);
	}
	ExplorationHUD.Reset();
	PreviousVisibility = ESlateVisibility::Visible;
}
