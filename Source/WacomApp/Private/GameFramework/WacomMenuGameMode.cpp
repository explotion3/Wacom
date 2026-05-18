// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Menus/WacomMainMenuScreen.h"

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

	// 菜单模式输入配置：UI 优先 + 鼠标可见，不锁窗
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);

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

	if (!Pushed)
	{
		UE_LOG(LogTemp, Error, TEXT("[MenuGameMode] Push MainMenuScreen 失败"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] BeginPlay 完成，MainMenu 已 Push 到 GameMenu 层"));
}

// ================ 切关卡入口（由 MainMenuScreen 按钮回调调用） ================

void AWacomMenuGameMode::RequestStartNewGame()
{
	UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] RequestStartNewGame"));

	// 存档系统启用时清掉旧存档；暂停时跳过。
	if (AWacomGameMode::bSaveSystemEnabled)
	{
		if (UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Main, 0))
		{
			UGameplayStatics::DeleteGameInSlot(AWacomGameMode::SlotName_Main, 0);
		}
		if (UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Auto, 0))
		{
			UGameplayStatics::DeleteGameInSlot(AWacomGameMode::SlotName_Auto, 0);
		}
	}

	// TearDown UI 后 next-tick OpenLevel。
	// 注：TearDown 必须走到 DeactivateWidget，让 CommonUI Router 释放 UIInputConfig。
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWacomGameUIManagerSubsystem* UIManager = GI->GetSubsystem<UWacomGameUIManagerSubsystem>())
		{
			UIManager->TearDownPrimaryLayout();
			UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] PrimaryLayout torn down，准备 OpenLevel"));
		}
	}

	// 立即 OpenLevel（GameMode 生命周期比 Widget 稳定，不需要 next-tick）
	UGameplayStatics::OpenLevel(this, ExplorationLevelName);
}

void AWacomMenuGameMode::RequestContinueGame()
{
	UE_LOG(LogTemp, Display, TEXT("[MenuGameMode] RequestContinueGame"));

	// 存档系统暂停（Stage 0.1）：Continue 不可用。理论上 UI 已禁用按钮，
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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWacomGameUIManagerSubsystem* UIManager = GI->GetSubsystem<UWacomGameUIManagerSubsystem>())
		{
			UIManager->TearDownPrimaryLayout();
		}
	}

	UGameplayStatics::OpenLevel(this, ExplorationLevelName);
}
