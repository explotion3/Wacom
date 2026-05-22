// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomExplorationScreenRouter.h"

#include "CommonActivatableWidget.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunState.h"
#include "Types/WacomEnums.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/Shop/WacomShopScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	bool IsExplorationState(const AWacomPlayerController& PC, const TCHAR* LogPrefix)
	{
		const AWacomGameMode* GM = PC.GetWorld() ? PC.GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
		if (!GM)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] %s: 非探索 GameMode，忽略"), LogPrefix);
			return false;
		}
		if (GM->GetGameFlowState() != EGameFlowState::Exploration)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] %s: 当前不在探索状态，忽略"), LogPrefix);
			return false;
		}
		return true;
	}

	UWacomGameUIManagerSubsystem* GetUIManager(const AWacomPlayerController& PC, const TCHAR* LogPrefix)
	{
		const UGameInstance* GI = PC.GetGameInstance();
		UWacomGameUIManagerSubsystem* UIManager =
			GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
		if (!UIManager)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] %s: UIManager 未就位"), LogPrefix);
		}
		return UIManager;
	}

	UCommonActivatableWidgetStack* GetGameMenuStack(UWacomGameUIManagerSubsystem& UIManager)
	{
		UWacomPrimaryGameLayout* Layout = UIManager.GetPrimaryLayout();
		if (!Layout)
		{
			return nullptr;
		}

		return Layout->GetLayerStack(WacomUITags::UI_Layer_GameMenu.GetTag());
	}

	UCommonActivatableWidget* GetActiveGameMenuWidget(UWacomGameUIManagerSubsystem& UIManager)
	{
		UCommonActivatableWidgetStack* MenuStack = GetGameMenuStack(UIManager);
		return MenuStack ? MenuStack->GetActiveWidget() : nullptr;
	}

	bool DeactivateActiveGameMenuWidget(UWacomGameUIManagerSubsystem& UIManager)
	{
		if (UCommonActivatableWidget* ActiveWidget = GetActiveGameMenuWidget(UIManager))
		{
			ActiveWidget->DeactivateWidget();
			return true;
		}
		return false;
	}
}

void FWacomExplorationScreenRouter::OpenBackpack(AWacomPlayerController& PC)
{
	if (!IsExplorationState(PC, TEXT("OpenBackpack")))
	{
		return;
	}

	UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, TEXT("OpenBackpack"));
	if (!UIManager)
	{
		return;
	}

	UIManager->EnsurePrimaryLayout(&PC);

	if (DeactivateActiveGameMenuWidget(*UIManager))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] B: 关闭 GameMenu 顶层"));
		return;
	}

	if (!PC.BackpackScreenClass)
	{
		PC.BackpackScreenClass = UWacomBackpackScreen::StaticClass();
	}

	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		PC.BackpackScreenClass);
	if (!Pushed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenBackpack: Push BackpackScreen 失败"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] B: 打开背包"));
}

void FWacomExplorationScreenRouter::TogglePauseMenu(AWacomPlayerController& PC)
{
	if (!IsExplorationState(PC, TEXT("TogglePauseMenu")))
	{
		return;
	}

	UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, TEXT("TogglePauseMenu"));
	if (!UIManager)
	{
		return;
	}

	UIManager->EnsurePrimaryLayout(&PC);

	if (DeactivateActiveGameMenuWidget(*UIManager))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] ESC: 关闭 GameMenu 顶层"));
		return;
	}

	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		UWacomPauseMenuScreen::StaticClass());
	if (!Pushed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] TogglePauseMenu: Push PauseMenuScreen 失败"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] ESC: 打开暂停菜单"));
}

void FWacomExplorationScreenRouter::CloseTopGameMenu(AWacomPlayerController& PC)
{
	UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, TEXT("CloseTopGameMenu"));
	if (!UIManager)
	{
		return;
	}

	DeactivateActiveGameMenuWidget(*UIManager);
}

bool FWacomExplorationScreenRouter::OpenShop(AWacomPlayerController& PC, FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	if (!IsExplorationState(PC, TEXT("OpenShop")))
	{
		return false;
	}
	if (ShopId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenShop: ShopId 为 None，拒绝"));
		return false;
	}

	UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, TEXT("OpenShop"));
	if (!UIManager)
	{
		return false;
	}

	UIManager->EnsurePrimaryLayout(&PC);
	if (!IsValid(UIManager->GetPrimaryLayout()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenShop: PrimaryLayout 未就位，拒绝打开商店"));
		return false;
	}

	DeactivateActiveGameMenuWidget(*UIManager);

	URunSession* RunSession = PC.GetRunSession();
	if (!RunSession || !RunSession->BeginShopVisit(ShopId, Offers))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenShop: BeginShopVisit 失败 ShopId=%s"), *ShopId.ToString());
		return false;
	}

	if (!PC.ShopScreenClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(nullptr,
			TEXT("/Game/Wacom/UI/Shop/WBP_ShopScreen.WBP_ShopScreen_C")))
		{
			PC.ShopScreenClass = Loaded;
		}
	}
	if (!PC.ShopScreenClass)
	{
		PC.ShopScreenClass = UWacomShopScreen::StaticClass();
	}

	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		PC.ShopScreenClass);
	UWacomShopScreen* ShopScreen = Cast<UWacomShopScreen>(Pushed);
	if (!ShopScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenShop: Push ShopScreen 失败"));
		RunSession->EndShopVisit();
		return false;
	}

	ShopScreen->RefreshShop();
	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] 打开商店 ShopId=%s"), *ShopId.ToString());
	return true;
}

bool FWacomExplorationScreenRouter::OpenRunEvent(AWacomPlayerController& PC, FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	if (!IsExplorationState(PC, TEXT("OpenRunEvent")))
	{
		return false;
	}
	if (PersistentId.IsNone() || !EventDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenRunEvent: PersistentId 或 EventDefinition 无效，拒绝"));
		return false;
	}

	UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, TEXT("OpenRunEvent"));
	if (!UIManager)
	{
		return false;
	}

	UIManager->EnsurePrimaryLayout(&PC);
	if (!IsValid(UIManager->GetPrimaryLayout()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenRunEvent: PrimaryLayout 未就位，拒绝打开事件"));
		return false;
	}

	DeactivateActiveGameMenuWidget(*UIManager);

	URunSession* RunSession = PC.GetRunSession();
	if (!RunSession || !RunSession->BeginRunEvent(PersistentId, EventDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenRunEvent: BeginRunEvent 失败 PersistentId=%s"), *PersistentId.ToString());
		return false;
	}

	if (!PC.RunEventScreenClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(nullptr,
			TEXT("/Game/Wacom/UI/Event/WBP_RunEventScreen.WBP_RunEventScreen_C")))
		{
			PC.RunEventScreenClass = Loaded;
		}
	}
	if (!PC.RunEventScreenClass)
	{
		PC.RunEventScreenClass = UWacomRunEventScreen::StaticClass();
	}

	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		PC.RunEventScreenClass);
	UWacomRunEventScreen* EventScreen = Cast<UWacomRunEventScreen>(Pushed);
	if (!EventScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenRunEvent: Push RunEventScreen 失败"));
		RunSession->EndRunEvent();
		return false;
	}

	EventScreen->RefreshEvent();
	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] 打开事件 PersistentId=%s"), *PersistentId.ToString());
	return true;
}
