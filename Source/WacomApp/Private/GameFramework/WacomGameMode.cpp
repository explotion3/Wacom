// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomGameMode.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Session/BattleSession.h"

#include "Actors/BattleTriggerActor.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"

AWacomGameMode::AWacomGameMode()
{
	PlayerControllerClass = AWacomPlayerController::StaticClass();
	DefaultPawnClass      = AWacomPlayerCharacter::StaticClass();
}

void AWacomGameMode::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EGameFlowState::Exploration;

	// 默认 CharacterDefinition / PrimaryLayout / HUD 类的懒加载。
	// 避免 CDO ConstructorHelpers 阶段资产可能还没就绪。
	if (!DefaultCharacter)
	{
		DefaultCharacter = LoadObject<UCharacterDefinition>(nullptr,
			TEXT("/Game/Wacom/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
	}
	if (!PrimaryLayoutClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(nullptr,
			TEXT("/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C")))
		{
			PrimaryLayoutClass = Loaded;
		}
	}
	if (!BattleHUDClass)
	{
		// 默认用 C++ 类 UBattleHUD：RebuildWidget 会自动构造一套完整布局。
		// 有美术 WBP 后，在 GameMode Details 面板覆盖即可。
		BattleHUDClass = UBattleHUD::StaticClass();
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomGameMode] BeginPlay, State=Exploration, DefaultCharacter=%s, PrimaryLayout=%s, BattleHUD=%s"),
		*GetNameSafe(DefaultCharacter),
		*GetNameSafe(PrimaryLayoutClass.Get()),
		*GetNameSafe(BattleHUDClass.Get()));
}

void AWacomGameMode::EnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger)
{
	if (CurrentState == EGameFlowState::Battle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] EnterBattle 被重复调用，忽略"));
		return;
	}
	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] EnterBattle 缺少 EnemyDef，忽略"));
		return;
	}
	if (!DefaultCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] EnterBattle: DefaultCharacter 未配置"));
		return;
	}
	if (!PrimaryLayoutClass || !BattleHUDClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] EnterBattle: PrimaryLayoutClass/BattleHUDClass 未配置"));
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] EnterBattle: 找不到 PlayerController"));
		return;
	}

	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	URunSession* Run = WacomPC ? WacomPC->GetRunSession() : nullptr;

	// 1) 创建 BattleSession + Initialize
	ActiveSession = NewObject<UBattleSession>(this);
	{
		FBattleInitParams Params;

		// 优先从 RunSession 构造（含角色 / 种子）；若 Run 未就绪则回退到 GameMode 字段。
		if (!Run || !Run->BuildInitParamsForBattle(EnemyDef, Params))
		{
			Params.Character  = DefaultCharacter;
			Params.Enemy      = EnemyDef;
			Params.RandomSeed = DefaultRandomSeed;
		}

		const FWacomStatus Status = ActiveSession->Initialize(Params);
		if (!Status.IsOk())
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] Session Initialize 失败 Code=%d"),
				(int32)Status.Code);
			ActiveSession = nullptr;
			return;
		}
	}

	// 2) 创建 PrimaryLayout + Push BattleHUD 到 Game Layer
	PrimaryLayout = CreateWidget<UWacomPrimaryGameLayout>(PC, PrimaryLayoutClass);
	if (!PrimaryLayout)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] 创建 PrimaryLayout 失败"));
		ActiveSession = nullptr;
		return;
	}
	PrimaryLayout->AddToViewport();

	UCommonActivatableWidget* Pushed = PrimaryLayout->PushWidgetToLayer(
		WacomUITags::UI_Layer_Game.GetTag(), BattleHUDClass);
	BattleHUD = Cast<UBattleHUD>(Pushed);
	if (!BattleHUD)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] Push BattleHUD 失败"));
		PrimaryLayout->RemoveFromParent();
		PrimaryLayout = nullptr;
		ActiveSession = nullptr;
		return;
	}

	BattleHUD->SetSession(ActiveSession);

	// 订阅战斗结束广播
	BattleEndedHandle = BattleHUD->OnBattleEndedNative.AddUObject(
		this, &AWacomGameMode::HandleBattleEnded);

	// 3) 禁用探索输入 + 切 IMC
	if (WacomPC)
	{
		if (WacomPC->ExplorationMappingContext)
		{
			WacomPC->PopMappingContext(WacomPC->ExplorationMappingContext);
		}
		if (WacomPC->BattleMappingContext)
		{
			WacomPC->PushMappingContext(WacomPC->BattleMappingContext, /*Priority*/1);
		}
	}

	if (AWacomPlayerCharacter* Pawn = PC->GetPawn<AWacomPlayerCharacter>())
	{
		Pawn->SetExplorationInputEnabled(false);
	}

	// 4) 记录状态
	CurrentState        = EGameFlowState::Battle;
	PendingTrigger      = Trigger;
	PendingEnemyDefForRun = EnemyDef;

	// 让 HUD 立即刷出初始 Snapshot
	BattleHUD->RefreshFromSnapshot(ActiveSession->BuildSnapshot());

	UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] EnterBattle 完成：EnemyDef=%s"),
		*GetNameSafe(EnemyDef));
}

void AWacomGameMode::HandleBattleEnded(EBattleOutcome Outcome)
{
	// HUD 仍处于 viewport，延迟到 ExitBattle 再清理。
	ExitBattle(Outcome);
}

void AWacomGameMode::ExitBattle(EBattleOutcome Outcome)
{
	if (CurrentState != EGameFlowState::Battle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] ExitBattle 在非战斗状态下被调用，忽略"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] ExitBattle: Outcome=%d"),
		static_cast<int32>(Outcome));

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;

	// 1) 反订阅 HUD 委托 + 销毁 UI
	if (BattleHUD && BattleEndedHandle.IsValid())
	{
		BattleHUD->OnBattleEndedNative.Remove(BattleEndedHandle);
	}
	BattleEndedHandle.Reset();

	if (PrimaryLayout)
	{
		PrimaryLayout->RemoveFromParent();
	}
	BattleHUD     = nullptr;
	PrimaryLayout = nullptr;
	ActiveSession = nullptr;

	// 2) 恢复探索输入 + 切 IMC
	if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
	{
		if (WacomPC->BattleMappingContext)
		{
			WacomPC->PopMappingContext(WacomPC->BattleMappingContext);
		}
		if (WacomPC->ExplorationMappingContext)
		{
			WacomPC->PushMappingContext(WacomPC->ExplorationMappingContext, /*Priority*/0);
		}

		// 战斗中 HUD 申请了 EMouseCaptureMode::NoCapture；恢复成 GameOnly 锁鼠标。
		WacomPC->bShowMouseCursor = false;
		WacomPC->SetInputMode(FInputModeGameOnly{});
	}
	if (PC)
	{
		if (AWacomPlayerCharacter* Pawn = PC->GetPawn<AWacomPlayerCharacter>())
		{
			Pawn->SetExplorationInputEnabled(true);
		}
	}

	// 3) 销毁触发器
	if (PendingTrigger)
	{
		PendingTrigger->Destroy();
		PendingTrigger = nullptr;
	}

	// 4) 通知 RunSession 战斗结束，让它更新击败列表 / run active 状态
	if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
	{
		if (URunSession* Run = WacomPC->GetRunSession())
		{
			Run->OnBattleFinished(Outcome, PendingEnemyDefForRun);
		}
	}
	PendingEnemyDefForRun = nullptr;

	// 5) 状态复位
	CurrentState = EGameFlowState::Exploration;
}
