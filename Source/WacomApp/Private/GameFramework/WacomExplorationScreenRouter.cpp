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

	bool CanPushExplorationGameMenu(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager,
		const TCHAR* LogPrefix)
	{
		AWacomPlayerController* PC = WeakPC.Get();
		UWacomGameUIManagerSubsystem* UIManager = WeakUIManager.Get();
		if (!PC || !UIManager || !IsExplorationState(*PC, LogPrefix))
		{
			return false;
		}

		return GetActiveGameMenuWidget(*UIManager) == nullptr;
	}

	void LogAsyncPushResult(const FWacomAsyncWidgetPushResult& Result, const TCHAR* SuccessMessage, const TCHAR* FailurePrefix)
	{
		if (Result.bSucceeded)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] %s"), SuccessMessage);
			return;
		}

		if (Result.FailureReason == TEXT("LayerPending")
			|| Result.FailureReason == TEXT("PrePushGuardRejected"))
		{
			return;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] %s 失败 Reason=%s"),
			FailurePrefix,
			*Result.FailureReason.ToString());
	}

	void BeginGameMenuTransitionSuppression(AWacomPlayerController& PC)
	{
		PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(true);
	}

	void EndGameMenuTransitionSuppressionOnFailure(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		const FWacomAsyncWidgetPushResult& Result)
	{
		if (Result.bSucceeded)
		{
			return;
		}

		if (AWacomPlayerController* PC = WeakPC.Get())
		{
			PC->SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
		}
	}

	void LogShopAsyncPushResult(const FWacomAsyncWidgetPushResult& Result, FName ShopId)
	{
		if (Result.bSucceeded)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] 打开商店 ShopId=%s"), *ShopId.ToString());
			return;
		}
		LogAsyncPushResult(Result, TEXT("打开商店"), TEXT("OpenShop: Push ShopScreen"));
	}

	void LogRunEventAsyncPushResult(const FWacomAsyncWidgetPushResult& Result, FName PersistentId)
	{
		if (Result.bSucceeded)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] 打开事件 PersistentId=%s"), *PersistentId.ToString());
			return;
		}
		LogAsyncPushResult(Result, TEXT("打开事件"), TEXT("OpenRunEvent: Push RunEventScreen"));
	}

	bool BeginShopVisitForAsyncPush(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		FName ShopId,
		TArray<FRunShopOfferInput> Offers,
		FName& OutFailureReason)
	{
		AWacomPlayerController* PC = WeakPC.Get();
		URunSession* RunSession = PC ? PC->GetRunSession() : nullptr;
		if (!RunSession || !RunSession->BeginShopVisit(ShopId, Offers))
		{
			OutFailureReason = TEXT("BeginShopVisitFailed");
			UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenShop.AsyncPush: BeginShopVisit 失败 ShopId=%s"), *ShopId.ToString());
			return false;
		}
		return true;
	}

	bool RefreshShopAfterAsyncPush(UCommonActivatableWidget& PushedWidget, FName& OutFailureReason)
	{
		UWacomShopScreen* ShopScreen = Cast<UWacomShopScreen>(&PushedWidget);
		if (!ShopScreen)
		{
			OutFailureReason = TEXT("InvalidShopScreen");
			return false;
		}
		ShopScreen->RefreshShop();
		return true;
	}

	void RollbackShopAsyncPush(TWeakObjectPtr<AWacomPlayerController> WeakPC, FName /*FailureReason*/)
	{
		if (AWacomPlayerController* PC = WeakPC.Get())
		{
			if (URunSession* RunSession = PC->GetRunSession())
			{
				RunSession->EndShopVisit();
			}
		}
	}

	void PrepareFailedShopAsyncPush(UCommonActivatableWidget& PushedWidget, FName /*FailureReason*/)
	{
		if (UWacomShopScreen* ShopScreen = Cast<UWacomShopScreen>(&PushedWidget))
		{
			ShopScreen->SuppressEndShopVisitOnNextDeactivate();
		}
	}

	bool BeginRunEventForAsyncPush(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		FName PersistentId,
		TWeakObjectPtr<UWacomRunEventDefinition> WeakEventDefinition,
		FName& OutFailureReason)
	{
		AWacomPlayerController* PC = WeakPC.Get();
		URunSession* RunSession = PC ? PC->GetRunSession() : nullptr;
		UWacomRunEventDefinition* EventDefinition = WeakEventDefinition.Get();
		if (!RunSession || !RunSession->BeginRunEvent(PersistentId, EventDefinition))
		{
			OutFailureReason = TEXT("BeginRunEventFailed");
			UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OpenRunEvent.AsyncPush: BeginRunEvent 失败 PersistentId=%s"), *PersistentId.ToString());
			return false;
		}
		return true;
	}

	bool RefreshRunEventAfterAsyncPush(UCommonActivatableWidget& PushedWidget, FName& OutFailureReason)
	{
		UWacomRunEventScreen* EventScreen = Cast<UWacomRunEventScreen>(&PushedWidget);
		if (!EventScreen)
		{
			OutFailureReason = TEXT("InvalidRunEventScreen");
			return false;
		}
		EventScreen->RefreshEvent();
		return true;
	}

	void RollbackRunEventAsyncPush(TWeakObjectPtr<AWacomPlayerController> WeakPC, FName /*FailureReason*/)
	{
		if (AWacomPlayerController* PC = WeakPC.Get())
		{
			if (URunSession* RunSession = PC->GetRunSession())
			{
				RunSession->EndRunEvent();
			}
		}
	}

	void PrepareFailedRunEventAsyncPush(UCommonActivatableWidget& PushedWidget, FName /*FailureReason*/)
	{
		if (UWacomRunEventScreen* EventScreen = Cast<UWacomRunEventScreen>(&PushedWidget))
		{
			EventScreen->SuppressEndRunEventOnNextDeactivate();
		}
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
		UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
		PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] B: 关闭 GameMenu 顶层"));
		return;
	}

	if (UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] OpenBackpack: GameMenu 正在异步打开，忽略重复请求"));
		return;
	}

	TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
	TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(UIManager);
	BeginGameMenuTransitionSuppression(PC);
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Request.FallbackClass = UWacomBackpackScreen::StaticClass();
	Request.OwningPlayer = &PC;
	Request.bLogMissingEntry = false;
	Request.CanPush = [WeakPC, WeakUIManager]()
	{
		return CanPushExplorationGameMenu(WeakPC, WeakUIManager, TEXT("OpenBackpack.AsyncPush"));
	};
	Request.OnComplete = [WeakPC](const FWacomAsyncWidgetPushResult& Result)
	{
		EndGameMenuTransitionSuppressionOnFailure(WeakPC, Result);
		LogAsyncPushResult(Result, TEXT("B: 打开背包"), TEXT("OpenBackpack: Push BackpackScreen"));
	};
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
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
		UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
		PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] ESC: 关闭 GameMenu 顶层"));
		return;
	}

	if (UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] TogglePauseMenu: GameMenu 正在异步打开，忽略重复请求"));
		return;
	}

	TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
	TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(UIManager);
	BeginGameMenuTransitionSuppression(PC);
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_PauseMenuScreen.GetTag();
	Request.FallbackClass = UWacomPauseMenuScreen::StaticClass();
	Request.OwningPlayer = &PC;
	Request.bLogMissingEntry = false;
	Request.CanPush = [WeakPC, WeakUIManager]()
	{
		return CanPushExplorationGameMenu(WeakPC, WeakUIManager, TEXT("TogglePauseMenu.AsyncPush"));
	};
	Request.OnComplete = [WeakPC](const FWacomAsyncWidgetPushResult& Result)
	{
		EndGameMenuTransitionSuppressionOnFailure(WeakPC, Result);
		LogAsyncPushResult(Result, TEXT("ESC: 打开暂停菜单"), TEXT("TogglePauseMenu: Push PauseMenuScreen"));
	};
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
}

void FWacomExplorationScreenRouter::CloseTopGameMenu(AWacomPlayerController& PC)
{
	UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, TEXT("CloseTopGameMenu"));
	if (!UIManager)
	{
		return;
	}

	UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
	PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
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

	if (DeactivateActiveGameMenuWidget(*UIManager))
	{
		UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
		PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
	}

	if (UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] OpenShop: GameMenu 正在异步打开，忽略请求"));
		return false;
	}

	TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
	TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(UIManager);
	BeginGameMenuTransitionSuppression(PC);
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_ShopScreen.GetTag();
	Request.FallbackClass = UWacomShopScreen::StaticClass();
	Request.OwningPlayer = &PC;
	Request.bLogMissingEntry = false;
	Request.CanPush = [WeakPC, WeakUIManager]()
	{
		return CanPushExplorationGameMenu(WeakPC, WeakUIManager, TEXT("OpenShop.AsyncPush"));
	};
	Request.BeforePush = [WeakPC, ShopId, Offers](FName& OutFailureReason)
	{
		return BeginShopVisitForAsyncPush(WeakPC, ShopId, Offers, OutFailureReason);
	};
	Request.AfterPush = [](UCommonActivatableWidget& PushedWidget, FName& OutFailureReason)
	{
		return RefreshShopAfterAsyncPush(PushedWidget, OutFailureReason);
	};
	Request.PrepareFailedPushedWidget = [](UCommonActivatableWidget& PushedWidget, FName FailureReason)
	{
		PrepareFailedShopAsyncPush(PushedWidget, FailureReason);
	};
	Request.Rollback = [WeakPC](FName FailureReason)
	{
		RollbackShopAsyncPush(WeakPC, FailureReason);
	};
	Request.OnComplete = [WeakPC, ShopId](const FWacomAsyncWidgetPushResult& Result)
	{
		EndGameMenuTransitionSuppressionOnFailure(WeakPC, Result);
		LogShopAsyncPushResult(Result, ShopId);
	};
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
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

	if (DeactivateActiveGameMenuWidget(*UIManager))
	{
		UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
		PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
	}

	if (UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] OpenRunEvent: GameMenu 正在异步打开，忽略请求"));
		return false;
	}

	TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
	TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(UIManager);
	TWeakObjectPtr<UWacomRunEventDefinition> WeakEventDefinition(EventDefinition);
	BeginGameMenuTransitionSuppression(PC);
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_RunEventScreen.GetTag();
	Request.FallbackClass = UWacomRunEventScreen::StaticClass();
	Request.OwningPlayer = &PC;
	Request.bLogMissingEntry = false;
	Request.CanPush = [WeakPC, WeakUIManager]()
	{
		return CanPushExplorationGameMenu(WeakPC, WeakUIManager, TEXT("OpenRunEvent.AsyncPush"));
	};
	Request.BeforePush = [WeakPC, PersistentId, WeakEventDefinition](FName& OutFailureReason)
	{
		return BeginRunEventForAsyncPush(WeakPC, PersistentId, WeakEventDefinition, OutFailureReason);
	};
	Request.AfterPush = [](UCommonActivatableWidget& PushedWidget, FName& OutFailureReason)
	{
		return RefreshRunEventAfterAsyncPush(PushedWidget, OutFailureReason);
	};
	Request.PrepareFailedPushedWidget = [](UCommonActivatableWidget& PushedWidget, FName FailureReason)
	{
		PrepareFailedRunEventAsyncPush(PushedWidget, FailureReason);
	};
	Request.Rollback = [WeakPC](FName FailureReason)
	{
		RollbackRunEventAsyncPush(WeakPC, FailureReason);
	};
	Request.OnComplete = [WeakPC, PersistentId](const FWacomAsyncWidgetPushResult& Result)
	{
		EndGameMenuTransitionSuppressionOnFailure(WeakPC, Result);
		LogRunEventAsyncPushResult(Result, PersistentId);
	};
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	return true;
}
