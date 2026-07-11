// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

#include "Engine/World.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomConfirmDialog.h"
#include "UI/Menus/WacomMainMenuScreen.h"

#define LOCTEXT_NAMESPACE "WacomMenuGameMode"

AWacomMenuGameMode::AWacomMenuGameMode()
{
	// 菜单关不生成可移动 Pawn。
	DefaultPawnClass      = nullptr;
	PlayerControllerClass = AWacomPlayerController::StaticClass();
}

void AWacomMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 默认类懒加载
	if (!MainMenuScreenClass)
	{
		MainMenuScreenClass = UWacomMainMenuScreen::StaticClass();
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[MenuGameMode] BeginPlay 找不到 PlayerController"));
		return;
	}

	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
			LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>())
		{
			InputCoordinator->InitializeForPlayerController(PC);
			if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
			{
				InputCoordinator->SetMappingContexts(WacomPC->ExplorationMappingContext, WacomPC->BattleMappingContext);
			}
			InputCoordinator->SetFlowContext(EWacomInputFlowContext::MainMenu);
		}
	}

	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[MenuGameMode] 找不到 UWacomGameUIManagerSubsystem"));
		return;
	}

	UIManager->EnsurePrimaryLayout(PC);

	// 清掉任何残留 Widget（比如切回主菜单时，Battle HUD 残留）
	UIManager->ClearAllLayers();

	// 切关卡时保留 Subsystem 但 Widget Tree 会被清空
	// （World 销毁时 Activatable Widget 跟随消失）。重新 Push MainMenu。
	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(), MainMenuScreenClass);

	UWacomMainMenuScreen* MainMenuScreen = Cast<UWacomMainMenuScreen>(Pushed);
	if (!MainMenuScreen)
	{
		UE_LOG(LogTemp, Error, TEXT("[MenuGameMode] Push MainMenuScreen 失败"));
		return;
	}

	BindMainMenuScreen(MainMenuScreen);

	UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] BeginPlay 完成，MainMenu 已 Push 到 GameMenu 层"));
}

void AWacomMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMainMenuScreen();
	Super::EndPlay(EndPlayReason);
}

FWacomMainMenuViewData AWacomMenuGameMode::BuildMainMenuViewData() const
{
	FWacomMainMenuViewData ViewData;

	// 当前正式存档流程仍暂停。关闭时不访问磁盘，主菜单只开放新旅程和退出。
	if (AWacomGameMode::bSaveSystemEnabled)
	{
		ViewData.bHasActiveJourney = UGameplayStatics::DoesSaveGameExist(
			AWacomGameMode::SlotName_Main, 0);
		ViewData.bCanContinueJourney = ViewData.bHasActiveJourney;
		if (ViewData.bHasActiveJourney)
		{
			ViewData.ActiveJourneyTitle = LOCTEXT("LegacyActiveJourneyTitle", "当前旅程");
			ViewData.ActiveJourneySummary = LOCTEXT(
				"LegacyActiveJourneySummary",
				"继续上次安全保存的旅程。");
		}
	}

	// 对应页面尚未落地。本轮只建立合同，不向玩家展示死入口。
	ViewData.bShowJourneyHistory = false;
	ViewData.bShowSettings = false;
	ViewData.bShowCredits = false;
	return ViewData;
}

void AWacomMenuGameMode::BindMainMenuScreen(UWacomMainMenuScreen* Screen)
{
	UnbindMainMenuScreen();
	if (!Screen)
	{
		return;
	}

	ActiveMainMenuScreen = Screen;
	Screen->OnActionRequestedNative.AddUObject(this, &AWacomMenuGameMode::HandleMainMenuAction);
	Screen->ApplyViewData(BuildMainMenuViewData());
}

void AWacomMenuGameMode::UnbindMainMenuScreen()
{
	if (UWacomMainMenuScreen* Screen = ActiveMainMenuScreen.Get())
	{
		Screen->OnActionRequestedNative.RemoveAll(this);
	}
	ActiveMainMenuScreen.Reset();
}

void AWacomMenuGameMode::HandleMainMenuAction(EWacomMainMenuAction Action)
{
	switch (Action)
	{
	case EWacomMainMenuAction::ContinueJourney:
	{
		const FWacomMainMenuViewData ViewData = BuildMainMenuViewData();
		if (!ViewData.bHasActiveJourney || !ViewData.bCanContinueJourney)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MenuGameMode] Continue action rejected by current ViewData"));
			if (UWacomMainMenuScreen* Screen = ActiveMainMenuScreen.Get())
			{
				Screen->ApplyViewData(ViewData);
			}
			return;
		}
		RequestContinueGame();
		return;
	}

	case EWacomMainMenuAction::StartNewJourney:
	{
		const FWacomMainMenuViewData ViewData = BuildMainMenuViewData();
		if (!ViewData.bHasActiveJourney)
		{
			RequestStartNewGame();
			return;
		}

		TWeakObjectPtr<AWacomMenuGameMode> WeakThis(this);
		UWacomConfirmDialog::Show(
			ActiveMainMenuScreen.IsValid()
				? static_cast<UObject*>(ActiveMainMenuScreen.Get())
				: static_cast<UObject*>(this),
			LOCTEXT("NewJourneyConfirmTitle", "开始新旅程"),
			LOCTEXT("NewJourneyConfirmMessage", "开始新的旅程将结束当前旅程。确定继续？"),
			[WeakThis]()
			{
				if (AWacomMenuGameMode* GameMode = WeakThis.Get())
				{
					GameMode->RequestStartNewGame();
				}
			});
		return;
	}

	case EWacomMainMenuAction::Quit:
		RequestQuitGame();
		return;

	case EWacomMainMenuAction::JourneyHistory:
	case EWacomMainMenuAction::Settings:
	case EWacomMainMenuAction::Credits:
	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[MenuGameMode] Reject unavailable main menu action: %d"),
			static_cast<int32>(Action));
		return;
	}
}

void AWacomMenuGameMode::RequestQuitGame()
{
	TWeakObjectPtr<AWacomMenuGameMode> WeakThis(this);
	UWacomConfirmDialog::Show(
		ActiveMainMenuScreen.IsValid()
			? static_cast<UObject*>(ActiveMainMenuScreen.Get())
			: static_cast<UObject*>(this),
		LOCTEXT("QuitConfirmTitle", "退出游戏"),
		LOCTEXT("QuitConfirmMessage", "确定要退出游戏吗？"),
		[WeakThis]()
		{
			AWacomMenuGameMode* GameMode = WeakThis.Get();
			if (!GameMode)
			{
				return;
			}

			UKismetSystemLibrary::QuitGame(
				GameMode,
				GameMode->GetWorld() ? GameMode->GetWorld()->GetFirstPlayerController() : nullptr,
				EQuitPreference::Quit,
				false);
		});
}

// ================ 切关卡入口（由 MainMenuScreen 按钮回调调用） ================

void AWacomMenuGameMode::RequestStartNewGame()
{
	UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] RequestStartNewGame"));

	bool bSaveCleanupAttempted = false;

	// 存档系统启用时清掉旧存档；暂停时跳过。
	if (AWacomGameMode::bSaveSystemEnabled)
	{
		bSaveCleanupAttempted = true;
		if (UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Main, 0))
		{
			UGameplayStatics::DeleteGameInSlot(AWacomGameMode::SlotName_Main, 0);
		}
		if (UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Auto, 0))
		{
			UGameplayStatics::DeleteGameInSlot(AWacomGameMode::SlotName_Auto, 0);
		}
	}

	RequestTravelToLevel(ExplorationLevelName, TEXT("StartNewGame"));
	LastMenuTravelDebugView.bStartNewGameSaveCleanupAttempted = bSaveCleanupAttempted;
}

void AWacomMenuGameMode::RequestContinueGame()
{
	UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] RequestContinueGame"));

	// 存档系统关闭时 Continue 不可用。理论上 UI 已禁用按钮，
	// 这里再防一次以防其他入口（命令行 / 蓝图）调用。
	if (!AWacomGameMode::bSaveSystemEnabled)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MenuGameMode] RequestContinueGame 被调用但存档系统已暂停，忽略"));
		return;
	}

	if (!UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Main, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuGameMode] Continue 被请求但无存档"));
		return;
	}

	RequestTravelToLevel(ExplorationLevelName, TEXT("ContinueGame"));
}

FName AWacomMenuGameMode::NormalizeLevelPackagePath(FName LevelName)
{
	const FString Value = LevelName.ToString();
	FString PackageName;
	FString ObjectName;
	if (Value.Split(TEXT("."), &PackageName, &ObjectName)
		&& !PackageName.IsEmpty()
		&& ObjectName == FPackageName::GetShortName(PackageName))
	{
		return FName(*PackageName);
	}
	return LevelName;
}

bool AWacomMenuGameMode::IsObjectPathLevelName(FName LevelName)
{
	const FString Value = LevelName.ToString();
	FString PackageName;
	FString ObjectName;
	return Value.Split(TEXT("."), &PackageName, &ObjectName)
		&& !PackageName.IsEmpty()
		&& !ObjectName.IsEmpty();
}

bool AWacomMenuGameMode::IsPackagePathLevelName(FName LevelName)
{
	const FString Value = LevelName.ToString();
	return Value.StartsWith(TEXT("/Game/")) && !IsObjectPathLevelName(LevelName);
}

void AWacomMenuGameMode::RequestTravelToLevel(FName LevelName, FName Reason)
{
	const FName PackageLevelName = NormalizeLevelPackagePath(LevelName);
	UWorld* World = GetWorld();

	LastMenuTravelDebugView = FWacomMenuTravelDebugView();
	LastMenuTravelDebugView.RequestedLevelName = LevelName;
	LastMenuTravelDebugView.TravelLevelName = PackageLevelName;
	LastMenuTravelDebugView.Reason = Reason;
	LastMenuTravelDebugView.WorldName = GetNameSafe(World);
	LastMenuTravelDebugView.bIsPIEWorld = World && World->WorldType == EWorldType::PIE;
	LastMenuTravelDebugView.bRequestedObjectPath = IsObjectPathLevelName(LevelName);
	LastMenuTravelDebugView.bTravelTargetUsesPackagePath = IsPackagePathLevelName(PackageLevelName);
	LastMenuTravelDebugView.bActualTravelSuppressedForAutomation = bSuppressActualTravelForAutomation;

	if (LastMenuTravelDebugView.bRequestedObjectPath)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MenuGameMode] Travel target 使用了 ObjectPath，将规范化为 package path: %s -> %s"),
			*LevelName.ToString(),
			*PackageLevelName.ToString());
	}

	UnbindMainMenuScreen();

	// 注：TearDown 必须走到 DeactivateWidget，让 CommonUI Router 释放 UIInputConfig。
	LastMenuTravelDebugView.bPrimaryLayoutTeardownRequested = true;
	LastMenuTravelDebugView.TeardownOrder = ++LastMenuTravelOrderCounter;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWacomGameUIManagerSubsystem* UIManager = GI->GetSubsystem<UWacomGameUIManagerSubsystem>())
		{
			UIManager->TearDownPrimaryLayout();
			LastMenuTravelDebugView.bPrimaryLayoutTeardownCompleted = true;
		}
	}

	PendingTravelLevelName = PackageLevelName;
	PendingTravelReason = Reason;
	LastMenuTravelDebugView.bTravelScheduledForNextTick = true;
	LastMenuTravelDebugView.ScheduleOrder = ++LastMenuTravelOrderCounter;

	UE_LOG(LogTemp, Display,
		TEXT("[MenuGameMode] ScheduleTravel Reason=%s Target=%s Requested=%s World=%s PIE=%s Teardown=%s PackagePath=%s"),
		*Reason.ToString(),
		*PackageLevelName.ToString(),
		*LevelName.ToString(),
		*LastMenuTravelDebugView.WorldName,
		LastMenuTravelDebugView.bIsPIEWorld ? TEXT("true") : TEXT("false"),
		LastMenuTravelDebugView.bPrimaryLayoutTeardownCompleted ? TEXT("true") : TEXT("false"),
		LastMenuTravelDebugView.bTravelTargetUsesPackagePath ? TEXT("true") : TEXT("false"));

	if (World)
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &AWacomMenuGameMode::ExecutePendingTravel));
	}
}

void AWacomMenuGameMode::ExecutePendingTravel()
{
	if (PendingTravelLevelName.IsNone())
	{
		return;
	}

	const FName LevelName = PendingTravelLevelName;
	const FName Reason = PendingTravelReason;
	PendingTravelLevelName = NAME_None;
	PendingTravelReason = NAME_None;

	LastMenuTravelDebugView.bTravelExecuted = true;
	LastMenuTravelDebugView.ExecuteOrder = ++LastMenuTravelOrderCounter;
	LastMenuTravelDebugView.bActualTravelSuppressedForAutomation = bSuppressActualTravelForAutomation;

	UE_LOG(LogTemp, Display,
		TEXT("[MenuGameMode] ExecuteTravel Reason=%s Target=%s World=%s SuppressedForAutomation=%s"),
		*Reason.ToString(),
		*LevelName.ToString(),
		*GetNameSafe(GetWorld()),
		bSuppressActualTravelForAutomation ? TEXT("true") : TEXT("false"));

	if (bSuppressActualTravelForAutomation)
	{
		return;
	}

	UGameplayStatics::OpenLevel(this, LevelName);
}

FString AWacomMenuGameMode::GetMenuTravelDebugSummary() const
{
	const FWacomMenuTravelDebugView& View = LastMenuTravelDebugView;
	return FString::Printf(
		TEXT("Reason=%s Requested=%s Target=%s PackagePath=%d ObjectPath=%d World=%s PIE=%d TeardownRequested=%d TeardownCompleted=%d Scheduled=%d Executed=%d Suppressed=%d Order(TearDown=%d Schedule=%d Execute=%d)"),
		*View.Reason.ToString(),
		*View.RequestedLevelName.ToString(),
		*View.TravelLevelName.ToString(),
		View.bTravelTargetUsesPackagePath ? 1 : 0,
		View.bRequestedObjectPath ? 1 : 0,
		*View.WorldName,
		View.bIsPIEWorld ? 1 : 0,
		View.bPrimaryLayoutTeardownRequested ? 1 : 0,
		View.bPrimaryLayoutTeardownCompleted ? 1 : 0,
		View.bTravelScheduledForNextTick ? 1 : 0,
		View.bTravelExecuted ? 1 : 0,
		View.bActualTravelSuppressedForAutomation ? 1 : 0,
		View.TeardownOrder,
		View.ScheduleOrder,
		View.ExecuteOrder);
}

#undef LOCTEXT_NAMESPACE
