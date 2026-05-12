// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "GameFramework/WacomGameMode.h"
#include "RunSession.h"
#include "Characters/CharacterDefinition.h"
#include "Types/WacomEnums.h"

#include "UI/Battle/BattleHUD.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	// LoadObject 一个 IA 辅助，减少 BeginPlay 里的样板。
	void LazyLoadIA(TObjectPtr<UInputAction>& Slot, const TCHAR* Path)
	{
		if (!Slot)
		{
			Slot = LoadObject<UInputAction>(nullptr, Path);
		}
	}
}

void AWacomPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] BeginPlay"));

	// ---- IMC 懒加载（两个场景都可能用到）----
	if (!ExplorationMappingContext)
	{
		ExplorationMappingContext = LoadObject<UInputMappingContext>(nullptr,
			TEXT("/Game/Wacom/Input/IMC_Exploration.IMC_Exploration"));
	}
	if (!BattleMappingContext)
	{
		BattleMappingContext = LoadObject<UInputMappingContext>(nullptr,
			TEXT("/Game/Wacom/Input/IMC_Battle.IMC_Battle"));
	}

	// 探索 GameMode（AWacomGameMode）才走：锁鼠标 + Push IMC_Exploration + 建 RunSession
	// 菜单 GameMode（AWacomMenuGameMode）由它自己的 BeginPlay 管鼠标 / 输入模式
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	if (GM)
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly{});

		if (ExplorationMappingContext)
		{
			PushMappingContext(ExplorationMappingContext, /*Priority*/0);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomPlayerController] ExplorationMappingContext 未配置，请先运行 WacomCreateInputAssets"));
		}

		if (!RunSession)
		{
			RunSession = NewObject<URunSession>(this);
			if (!RunSession->Initialize(GM->DefaultCharacter))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[WacomPlayerController] RunSession 初始化失败：DefaultCharacter 为空"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomPlayerController] 非探索 GameMode，跳过 IMC_Exploration / RunSession 初始化"));
	}
}

void AWacomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] InputComponent 不是 UEnhancedInputComponent"));
		return;
	}

	// IA 资产懒加载（SetupInputComponent 早于 BeginPlay）
	LazyLoadIA(IA_PlayCard1,  TEXT("/Game/Wacom/Input/IA_PlayCard1.IA_PlayCard1"));
	LazyLoadIA(IA_PlayCard2,  TEXT("/Game/Wacom/Input/IA_PlayCard2.IA_PlayCard2"));
	LazyLoadIA(IA_PlayCard3,  TEXT("/Game/Wacom/Input/IA_PlayCard3.IA_PlayCard3"));
	LazyLoadIA(IA_PlayCard4,  TEXT("/Game/Wacom/Input/IA_PlayCard4.IA_PlayCard4"));
	LazyLoadIA(IA_PlayCard5,  TEXT("/Game/Wacom/Input/IA_PlayCard5.IA_PlayCard5"));
	LazyLoadIA(IA_PlayCard6,  TEXT("/Game/Wacom/Input/IA_PlayCard6.IA_PlayCard6"));
	LazyLoadIA(IA_PlayCard7,  TEXT("/Game/Wacom/Input/IA_PlayCard7.IA_PlayCard7"));
	LazyLoadIA(IA_Wait,       TEXT("/Game/Wacom/Input/IA_Wait.IA_Wait"));
	LazyLoadIA(IA_EndTurn,    TEXT("/Game/Wacom/Input/IA_EndTurn.IA_EndTurn"));
	LazyLoadIA(IA_Restart,    TEXT("/Game/Wacom/Input/IA_Restart.IA_Restart"));
	LazyLoadIA(IA_RefreshHUD, TEXT("/Game/Wacom/Input/IA_RefreshHUD.IA_RefreshHUD"));
	LazyLoadIA(IA_OpenMenu,   TEXT("/Game/Wacom/Input/IA_OpenMenu.IA_OpenMenu"));

	if (IA_PlayCard1) { EIC->BindAction(IA_PlayCard1, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard1); }
	if (IA_PlayCard2) { EIC->BindAction(IA_PlayCard2, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard2); }
	if (IA_PlayCard3) { EIC->BindAction(IA_PlayCard3, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard3); }
	if (IA_PlayCard4) { EIC->BindAction(IA_PlayCard4, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard4); }
	if (IA_PlayCard5) { EIC->BindAction(IA_PlayCard5, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard5); }
	if (IA_PlayCard6) { EIC->BindAction(IA_PlayCard6, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard6); }
	if (IA_PlayCard7) { EIC->BindAction(IA_PlayCard7, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard7); }

	if (IA_Wait)       { EIC->BindAction(IA_Wait,       ETriggerEvent::Started, this, &AWacomPlayerController::OnWaitPressed); }
	if (IA_EndTurn)    { EIC->BindAction(IA_EndTurn,    ETriggerEvent::Started, this, &AWacomPlayerController::OnEndTurnPressed); }
	if (IA_Restart)    { EIC->BindAction(IA_Restart,    ETriggerEvent::Started, this, &AWacomPlayerController::OnRestartPressed); }
	if (IA_RefreshHUD) { EIC->BindAction(IA_RefreshHUD, ETriggerEvent::Started, this, &AWacomPlayerController::OnRefreshHUDPressed); }
	if (IA_OpenMenu)   { EIC->BindAction(IA_OpenMenu,   ETriggerEvent::Started, this, &AWacomPlayerController::OnOpenMenuPressed); }
}

// ================ 战斗状态切换转发 ================

void AWacomPlayerController::RequestEnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger)
{
	if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
	{
		GM->EnterBattle(EnemyDef, Trigger);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] RequestEnterBattle 时找不到 AWacomGameMode"));
	}
}

void AWacomPlayerController::RequestExitBattle(uint8 Outcome)
{
	if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
	{
		GM->ExitBattle(static_cast<EBattleOutcome>(Outcome));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] RequestExitBattle 时找不到 AWacomGameMode"));
	}
}

// ================ IMC 切换 ================

void AWacomPlayerController::PushMappingContext(UInputMappingContext* IMC, int32 Priority)
{
	if (!IMC) { return; }
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(IMC, Priority);
	}
}

void AWacomPlayerController::PopMappingContext(UInputMappingContext* IMC)
{
	if (!IMC) { return; }
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(IMC);
	}
}

// ================ 战斗 IA 回调 ================

UBattleHUD* AWacomPlayerController::GetActiveBattleHUD() const
{
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	// GameMode 没暴露 HUD getter，这里依赖它最终存活于 Game Layer。
	// 为简洁起见：让 GameMode 提供。
	return GM ? GM->GetActiveBattleHUD() : nullptr;
}

void AWacomPlayerController::RouteHandIndex(int32 OneBasedIndex)
{
	UBattleHUD* HUD = GetActiveBattleHUD();
	if (!HUD) { return; }

	UBattleSession* S = HUD->GetSession();
	if (!S) { return; }

	const FBattleSnapshot Snap = S->BuildSnapshot();
	const int32 Idx = OneBasedIndex - 1;
	if (!Snap.Hand.Cards.IsValidIndex(Idx)) { return; }

	const FGuid CardId = Snap.Hand.Cards[Idx].InstanceId;
	HUD->OnCardClickedByUser(CardId);
}

void AWacomPlayerController::OnPlayCard1() { RouteHandIndex(1); }
void AWacomPlayerController::OnPlayCard2() { RouteHandIndex(2); }
void AWacomPlayerController::OnPlayCard3() { RouteHandIndex(3); }
void AWacomPlayerController::OnPlayCard4() { RouteHandIndex(4); }
void AWacomPlayerController::OnPlayCard5() { RouteHandIndex(5); }
void AWacomPlayerController::OnPlayCard6() { RouteHandIndex(6); }
void AWacomPlayerController::OnPlayCard7() { RouteHandIndex(7); }

void AWacomPlayerController::OnWaitPressed()
{
	if (UBattleHUD* HUD = GetActiveBattleHUD()) { HUD->OnWaitRequested(); }
}

void AWacomPlayerController::OnEndTurnPressed()
{
	if (UBattleHUD* HUD = GetActiveBattleHUD()) { HUD->OnEndTurnRequested(); }
}

void AWacomPlayerController::OnRestartPressed()
{
	// R / Restart：第一阶段占位——重启战斗由 GameMode ExitBattle 之后新建 Run 处理。
	// 当前不做任何事，保留按键绑定避免 IMC 里的 R 映射空触发警告。
	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] Restart 按键暂不支持"));
}

void AWacomPlayerController::OnRefreshHUDPressed()
{
	if (UBattleHUD* HUD = GetActiveBattleHUD())
	{
		if (UBattleSession* S = HUD->GetSession())
		{
			HUD->RefreshFromSnapshot(S->BuildSnapshot());
		}
	}
}

void AWacomPlayerController::OnOpenMenuPressed()
{
	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] OnOpenMenuPressed: UIManager 未就位"));
		return;
	}

	// 如果 GameMenu 层已有内容（暂停菜单已打开），ESC = 关闭（Resume）
	UWacomPrimaryGameLayout* Layout = UIManager->GetPrimaryLayout();
	if (Layout)
	{
		UCommonActivatableWidgetStack* MenuStack = Layout->GetLayerStack(
			WacomUITags::UI_Layer_GameMenu.GetTag());
		if (MenuStack && MenuStack->GetActiveWidget())
		{
			// 已有菜单打开 → Pop 它（等同于 Resume）
			MenuStack->GetActiveWidget()->DeactivateWidget();
			UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] ESC: 关闭暂停菜单"));
			return;
		}
	}

	// 没有菜单打开 → Push 暂停菜单
	UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		UWacomPauseMenuScreen::StaticClass());
	UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] ESC: 打开暂停菜单"));
}
