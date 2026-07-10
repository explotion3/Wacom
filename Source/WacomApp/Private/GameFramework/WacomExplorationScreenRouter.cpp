// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomExplorationScreenRouter.h"

#include "CommonActivatableWidget.h"
#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunState.h"
#include "Types/WacomEnums.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/Shop/WacomShopScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	using FWacomOpenGameMenuAfterStage =
		TFunction<void(AWacomPlayerController&, UWacomGameUIManagerSubsystem&, bool)>;

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

	bool PrepareOpenGameMenu(
		AWacomPlayerController& PC,
		const TCHAR* LogPrefix,
		const TCHAR* RejectActionLabel,
		TFunctionRef<bool()> ValidateRequest,
		UWacomGameUIManagerSubsystem*& OutUIManager)
	{
		OutUIManager = nullptr;

		if (!IsExplorationState(PC, LogPrefix))
		{
			return false;
		}
		if (!ValidateRequest())
		{
			return false;
		}

		UWacomGameUIManagerSubsystem* UIManager = GetUIManager(PC, LogPrefix);
		if (!UIManager)
		{
			return false;
		}

		UIManager->EnsurePrimaryLayout(&PC);
		if (!IsValid(UIManager->GetPrimaryLayout()))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomPlayerController] %s: PrimaryLayout 未就位，拒绝%s"),
				LogPrefix,
				RejectActionLabel);
			return false;
		}

		if (DeactivateActiveGameMenuWidget(*UIManager))
		{
			UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
			if (!PC.IsGameMenuViewpointStageTransitionActive()
				&& !PC.IsGameMenuViewpointReturnArmed())
			{
				PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
			}
		}

		if (UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()))
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomPlayerController] %s: GameMenu 正在异步打开，忽略请求"),
				LogPrefix);
			return false;
		}

		OutUIManager = UIManager;
		return true;
	}

	bool PrepareOpenShopMenu(
		AWacomPlayerController& PC,
		FName ShopId,
		const TCHAR* LogPrefix,
		UWacomGameUIManagerSubsystem*& OutUIManager)
	{
		return PrepareOpenGameMenu(
			PC,
			LogPrefix,
			TEXT("打开商店"),
			[ShopId, LogPrefix]()
			{
				if (ShopId.IsNone())
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WacomPlayerController] %s: ShopId 为 None，拒绝"),
						LogPrefix);
					return false;
				}
				return true;
			},
			OutUIManager);
	}

	bool PrepareOpenRunEventMenu(
		AWacomPlayerController& PC,
		FName PersistentId,
		UWacomRunEventDefinition* EventDefinition,
		const TCHAR* LogPrefix,
		UWacomGameUIManagerSubsystem*& OutUIManager)
	{
		return PrepareOpenGameMenu(
			PC,
			LogPrefix,
			TEXT("打开事件"),
			[PersistentId, EventDefinition, LogPrefix]()
			{
				if (PersistentId.IsNone() || !EventDefinition)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[WacomPlayerController] %s: PersistentId 或 EventDefinition 无效，拒绝"),
						LogPrefix);
					return false;
				}
				return true;
			},
			OutUIManager);
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

	bool StageGameMenuThenOpen(
		AWacomPlayerController& PC,
		UWacomGameUIManagerSubsystem& UIManager,
		const FWacomFirstPersonViewStageRequest& StageRequest,
		FName DebugReason,
		const TCHAR* LogPrefix,
		FWacomOpenGameMenuAfterStage&& OpenMenuAfterStage)
	{
		AWacomPlayerCharacter* Pawn = PC.GetPawn<AWacomPlayerCharacter>();
		if (!Pawn)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomPlayerController] %s: 找不到 WacomPlayerCharacter，回退到普通 GameMenu 打开路径"),
				LogPrefix);
			OpenMenuAfterStage(PC, UIManager, /*bReturnToRunTunnelAfterClose*/false);
			return true;
		}

		PC.BeginGameMenuViewpointStageTransition(DebugReason);

		TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
		TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(&UIManager);
		TSharedRef<FWacomOpenGameMenuAfterStage> SharedOpenMenu =
			MakeShared<FWacomOpenGameMenuAfterStage>(MoveTemp(OpenMenuAfterStage));
		auto OpenAfterStage =
			[WeakPC, WeakUIManager, SharedOpenMenu]()
			{
				AWacomPlayerController* PC = WeakPC.Get();
				UWacomGameUIManagerSubsystem* UIManager = WeakUIManager.Get();
				if (!PC || !UIManager)
				{
					if (PC)
					{
						PC->ReturnFromGameMenuViewpointStageAfterFailedOpen();
					}
					return;
				}

				(*SharedOpenMenu)(
					*PC,
					*UIManager,
					/*bReturnToRunTunnelAfterClose*/true);
			};

		TFunction<void()> DeferredOpenAfterStage = OpenAfterStage;
		const bool bDeferred =
			FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
				*Pawn,
				PC,
				StageRequest,
				MoveTemp(DeferredOpenAfterStage));
		if (!bDeferred)
		{
			OpenAfterStage();
		}
		return true;
	}

	bool BeginShopVisitForAsyncPush(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		FName ShopId,
		TArray<FRunShopOfferInput> Offers,
		FGuid& OutVisitToken,
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
		OutVisitToken = RunSession->GetActiveShopVisitToken();
		if (!OutVisitToken.IsValid())
		{
			OutFailureReason = TEXT("MissingShopVisitToken");
			RunSession->EndShopVisit();
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

	void RollbackShopAsyncPush(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		FGuid VisitToken,
		FName /*FailureReason*/)
	{
		if (AWacomPlayerController* PC = WeakPC.Get())
		{
			if (URunSession* RunSession = PC->GetRunSession())
			{
				RunSession->EndShopVisitIfOwned(VisitToken);
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

	void PushShopScreenAsync(
		AWacomPlayerController& PC,
		UWacomGameUIManagerSubsystem& UIManager,
		FName ShopId,
		const TArray<FRunShopOfferInput>& Offers,
		bool bReturnToRunTunnelAfterClose)
	{
		TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
		TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(&UIManager);
		TSharedRef<FGuid> ShopVisitToken = MakeShared<FGuid>();
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
		Request.BeforePush = [WeakPC, ShopId, Offers, ShopVisitToken](FName& OutFailureReason)
		{
			return BeginShopVisitForAsyncPush(WeakPC, ShopId, Offers, *ShopVisitToken, OutFailureReason);
		};
		Request.AfterPush = [](UCommonActivatableWidget& PushedWidget, FName& OutFailureReason)
		{
			return RefreshShopAfterAsyncPush(PushedWidget, OutFailureReason);
		};
		Request.PrepareFailedPushedWidget = [](UCommonActivatableWidget& PushedWidget, FName FailureReason)
		{
			PrepareFailedShopAsyncPush(PushedWidget, FailureReason);
		};
		Request.Rollback = [WeakPC, ShopVisitToken](FName FailureReason)
		{
			RollbackShopAsyncPush(WeakPC, *ShopVisitToken, FailureReason);
		};
		Request.OnComplete = [WeakPC, ShopId, bReturnToRunTunnelAfterClose](const FWacomAsyncWidgetPushResult& Result)
		{
			if (AWacomPlayerController* PC = WeakPC.Get())
			{
				if (bReturnToRunTunnelAfterClose)
				{
					if (Result.bSucceeded)
					{
						PC->ArmGameMenuViewpointReturnForMenu(Cast<UWacomMenuWidgetBase>(Result.PushedWidget));
					}
					else
					{
						PC->ReturnFromGameMenuViewpointStageAfterFailedOpen();
					}
				}
				else
				{
					EndGameMenuTransitionSuppressionOnFailure(WeakPC, Result);
				}
			}
			LogShopAsyncPushResult(Result, ShopId);
		};
		UIManager.PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	}

	bool BeginRunEventForAsyncPush(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		FName PersistentId,
		TWeakObjectPtr<UWacomRunEventDefinition> WeakEventDefinition,
		FGuid& OutVisitToken,
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
		OutVisitToken = RunSession->GetActiveRunEventVisitToken();
		if (!OutVisitToken.IsValid())
		{
			OutFailureReason = TEXT("MissingRunEventVisitToken");
			RunSession->EndRunEvent();
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

	void RollbackRunEventAsyncPush(
		TWeakObjectPtr<AWacomPlayerController> WeakPC,
		FGuid VisitToken,
		FName /*FailureReason*/)
	{
		if (AWacomPlayerController* PC = WeakPC.Get())
		{
			if (URunSession* RunSession = PC->GetRunSession())
			{
				RunSession->EndRunEventIfOwned(VisitToken);
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

	void PushRunEventScreenAsync(
		AWacomPlayerController& PC,
		UWacomGameUIManagerSubsystem& UIManager,
		FName PersistentId,
		UWacomRunEventDefinition* EventDefinition,
		bool bReturnToRunTunnelAfterClose)
	{
		TWeakObjectPtr<AWacomPlayerController> WeakPC(&PC);
		TWeakObjectPtr<UWacomGameUIManagerSubsystem> WeakUIManager(&UIManager);
		TWeakObjectPtr<UWacomRunEventDefinition> WeakEventDefinition(EventDefinition);
		TSharedRef<FGuid> RunEventVisitToken = MakeShared<FGuid>();
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
		Request.BeforePush = [WeakPC, PersistentId, WeakEventDefinition, RunEventVisitToken](FName& OutFailureReason)
		{
			return BeginRunEventForAsyncPush(
				WeakPC,
				PersistentId,
				WeakEventDefinition,
				*RunEventVisitToken,
				OutFailureReason);
		};
		Request.AfterPush = [](UCommonActivatableWidget& PushedWidget, FName& OutFailureReason)
		{
			return RefreshRunEventAfterAsyncPush(PushedWidget, OutFailureReason);
		};
		Request.PrepareFailedPushedWidget = [](UCommonActivatableWidget& PushedWidget, FName FailureReason)
		{
			PrepareFailedRunEventAsyncPush(PushedWidget, FailureReason);
		};
		Request.Rollback = [WeakPC, RunEventVisitToken](FName FailureReason)
		{
			RollbackRunEventAsyncPush(WeakPC, *RunEventVisitToken, FailureReason);
		};
		Request.OnComplete = [WeakPC, PersistentId, bReturnToRunTunnelAfterClose](const FWacomAsyncWidgetPushResult& Result)
		{
			if (AWacomPlayerController* PC = WeakPC.Get())
			{
				if (bReturnToRunTunnelAfterClose)
				{
					if (Result.bSucceeded)
					{
						PC->ArmGameMenuViewpointReturnForMenu(Cast<UWacomMenuWidgetBase>(Result.PushedWidget));
					}
					else
					{
						PC->ReturnFromGameMenuViewpointStageAfterFailedOpen();
					}
				}
				else
				{
					EndGameMenuTransitionSuppressionOnFailure(WeakPC, Result);
				}
			}
			LogRunEventAsyncPushResult(Result, PersistentId);
		};
		UIManager.PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
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

	const bool bHadPendingGameMenu =
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
	UIManager->CancelPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag());
	if (PC.IsGameMenuViewpointStageTransitionActive() && bHadPendingGameMenu)
	{
		PC.ReturnFromGameMenuViewpointStageAfterFailedOpen();
		return;
	}
	if (!PC.IsGameMenuViewpointStageTransitionActive()
		&& !PC.IsGameMenuViewpointReturnArmed())
	{
		PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
	}
	DeactivateActiveGameMenuWidget(*UIManager);
}

bool FWacomExplorationScreenRouter::OpenShop(AWacomPlayerController& PC, FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	UWacomGameUIManagerSubsystem* UIManager = nullptr;
	if (!PrepareOpenShopMenu(PC, ShopId, TEXT("OpenShop"), UIManager))
	{
		return false;
	}

	PushShopScreenAsync(
		PC,
		*UIManager,
		ShopId,
		Offers,
		/*bReturnToRunTunnelAfterClose*/false);
	return true;
}

bool FWacomExplorationScreenRouter::OpenShop(
	AWacomPlayerController& PC,
	FName ShopId,
	const TArray<FRunShopOfferInput>& Offers,
	const FWacomFirstPersonViewStageRequest& StageRequest)
{
	if (!StageRequest.bHasViewTransform)
	{
		return OpenShop(PC, ShopId, Offers);
	}

	UWacomGameUIManagerSubsystem* UIManager = nullptr;
	if (!PrepareOpenShopMenu(PC, ShopId, TEXT("OpenShop.Staged"), UIManager))
	{
		return false;
	}

	return StageGameMenuThenOpen(
		PC,
		*UIManager,
		StageRequest,
		FName(TEXT("ShopEntry")),
		TEXT("OpenShop.Staged"),
		[ShopId, Offers](
			AWacomPlayerController& PC,
			UWacomGameUIManagerSubsystem& UIManager,
			bool bReturnToRunTunnelAfterClose)
		{
			PushShopScreenAsync(
				PC,
				UIManager,
				ShopId,
				Offers,
				bReturnToRunTunnelAfterClose);
		});
}

bool FWacomExplorationScreenRouter::OpenRunEvent(AWacomPlayerController& PC, FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	UWacomGameUIManagerSubsystem* UIManager = nullptr;
	if (!PrepareOpenRunEventMenu(PC, PersistentId, EventDefinition, TEXT("OpenRunEvent"), UIManager))
	{
		return false;
	}

	PushRunEventScreenAsync(
		PC,
		*UIManager,
		PersistentId,
		EventDefinition,
		/*bReturnToRunTunnelAfterClose*/false);
	return true;
}

bool FWacomExplorationScreenRouter::OpenRunEvent(
	AWacomPlayerController& PC,
	FName PersistentId,
	UWacomRunEventDefinition* EventDefinition,
	const FWacomFirstPersonViewStageRequest& StageRequest)
{
	if (!StageRequest.bHasViewTransform)
	{
		return OpenRunEvent(PC, PersistentId, EventDefinition);
	}

	UWacomGameUIManagerSubsystem* UIManager = nullptr;
	if (!PrepareOpenRunEventMenu(PC, PersistentId, EventDefinition, TEXT("OpenRunEvent.Staged"), UIManager))
	{
		return false;
	}

	TWeakObjectPtr<UWacomRunEventDefinition> WeakEventDefinition(EventDefinition);
	return StageGameMenuThenOpen(
		PC,
		*UIManager,
		StageRequest,
		FName(TEXT("RunEventEntry")),
		TEXT("OpenRunEvent.Staged"),
		[PersistentId, WeakEventDefinition](
			AWacomPlayerController& PC,
			UWacomGameUIManagerSubsystem& UIManager,
			bool bReturnToRunTunnelAfterClose)
		{
			UWacomRunEventDefinition* EventDefinition = WeakEventDefinition.Get();
			if (!EventDefinition)
			{
				if (bReturnToRunTunnelAfterClose)
				{
					PC.ReturnFromGameMenuViewpointStageAfterFailedOpen();
				}
				return;
			}

			PushRunEventScreenAsync(
				PC,
				UIManager,
				PersistentId,
				EventDefinition,
				bReturnToRunTunnelAfterClose);
		});
}
