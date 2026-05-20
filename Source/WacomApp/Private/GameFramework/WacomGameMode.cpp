// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomGameMode.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "EngineUtils.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Session/BattleSession.h"

#include "Actors/BattleTriggerActor.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

const FString AWacomGameMode::SlotName_Main = TEXT("Main");
const FString AWacomGameMode::SlotName_Auto = TEXT("Auto");

AWacomGameMode::AWacomGameMode()
{
	PlayerControllerClass = AWacomPlayerController::StaticClass();
	DefaultPawnClass      = AWacomPlayerCharacter::StaticClass();
}

void AWacomGameMode::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EGameFlowState::Exploration;

	// 默认 CharacterDefinition / HUD 类的懒加载。
	// 避免 CDO ConstructorHelpers 阶段资产可能还没就绪。
	if (!DefaultCharacter)
	{
		DefaultCharacter = LoadObject<UCharacterDefinition>(nullptr,
			TEXT("/Game/Wacom/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
	}
	if (!BattleHUDClass)
	{
		// 默认用 C++ 类 UBattleHUD：RebuildWidget 会自动构造一套完整布局。
		// 有美术 WBP 后，在 GameMode Details 面板覆盖即可。
		BattleHUDClass = UBattleHUD::StaticClass();
	}

	// 探索 HUD：默认 C++ 父类，蓝图子类可在 GameMode Details 面板覆盖。
	if (!ExplorationHUDClass)
	{
		ExplorationHUDClass = UWacomExplorationHUD::StaticClass();
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomGameMode] BeginPlay, State=Exploration, DefaultCharacter=%s, BattleHUD=%s"),
		*GetNameSafe(DefaultCharacter),
		*GetNameSafe(BattleHUDClass.Get()));

	// 进入探索关卡时的 UI 重置：
	// 1. EnsurePrimaryLayout（若跨关卡而来，stale 检测会重建）
	// 2. ClearAllLayers 清上一关残留
	// 3. Push ExplorationHUD 作为 Game 层底层 widget——声明 Game input config，
	//    让 CommonUIActionRouter 离开上一关的 Menu config 状态。这条尤其关键：
	//    否则从主菜单切过来时 Router 会卡在 MainMenuScreen 的 Menu config，WASD 失效。
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWacomGameUIManagerSubsystem* UIManager = GI->GetSubsystem<UWacomGameUIManagerSubsystem>())
		{
			if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
			{
				UIManager->EnsurePrimaryLayout(PC);
			}
			UIManager->ClearAllLayers();
			UIManager->PushContentToLayer(
				WacomUITags::UI_Layer_Game.GetTag(),
				ExplorationHUDClass);
		}
	}

	// 从主菜单 OpenLevel 过来时，PC 是复用的（PIE 里 OpenLevel 不销毁 PC），
	// PC::BeginPlay 不会再次被调用，IMC_Exploration 从未被 Push。
	// 这里主动确保 IMC_Exploration 就位。
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			if (WacomPC->ExplorationMappingContext)
			{
				WacomPC->PushMappingContext(WacomPC->ExplorationMappingContext, /*Priority*/0);
			}
		}
	}

	// 存档引导延后一帧：此时 PlayerController 可能尚未 BeginPlay，
	// RunSession 还没创建。下一帧所有 actor 都已 BeginPlay 完毕。
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &AWacomGameMode::BootstrapRunFromSave));
	}
}

void AWacomGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 只对正常退出做存档。关卡切换 / 崩溃 / Editor 停止 PIE 都归属"游戏结束"。
	// 如果当前在战斗中（CurrentState == Battle），按规则丢弃进度：不存档。
	if (CurrentState == EGameFlowState::Exploration)
	{
		// World 可能已进入 TearingDown，PlayerController 已被销毁。
		// 此时 SaveRunToSlot 会找不到 Run——用 bQuiet=true 避免 Warning 噪音。
		// 上一次 ExitBattle 的 SaveToSlot 已经写入，此处丢一次无功能损失。
		SaveRunToSlot(SlotName_Main, /*bQuiet*/true);
	}
	else
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomGameMode] EndPlay 期间仍在战斗中，按规则丢弃进度，不写存档"));
	}

	Super::EndPlay(EndPlayReason);
}

// ================ 存档引导 ================

void AWacomGameMode::BootstrapRunFromSave()
{
	// 存档系统暂停（Stage 0.1）：直接走新 Run，不读盘。
	if (!bSaveSystemEnabled)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomGameMode] Bootstrap: 存档系统暂停，保持新 Run"));
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	URunSession* Run = WacomPC ? WacomPC->GetRunSession() : nullptr;
	if (!Run)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomGameMode] BootstrapRunFromSave: RunSession 未就位，保持新 Run"));
		return;
	}

	bool bLoaded = false;
	if (Run->LoadFromSlot(SlotName_Main))
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] Bootstrap: 从 Main 读档成功"));
		bLoaded = true;
	}
	else if (Run->LoadFromSlot(SlotName_Auto))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] Bootstrap: Main 读档失败，已回退到 Auto"));
		bLoaded = true;
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] Bootstrap: 无有效存档，保持新 Run"));
	}

	// 读档成功：先清理已销毁的触发器，再传送玩家。
	// 顺序关键：teleport 用 TeleportPhysics 会触发 Overlap；如果已销毁的 Trigger
	// 还在场景里，teleport 到原来位置会立刻再次触发同一场战斗。
	if (bLoaded)
	{
		// 1. 销毁存档中已标记为"已销毁"的触发器。
		// Trigger::BeginPlay 做过一次自销毁检查，但那时 Bootstrap 尚未加载存档，
		// RunSession 是空的，所以还得在这里补一刀。
		for (TActorIterator<ABattleTriggerActor> It(GetWorld()); It; ++It)
		{
			ABattleTriggerActor* Trigger = *It;
			if (Trigger
				&& !Trigger->PersistentId.IsNone()
				&& Run->IsTriggerDestroyed(Trigger->PersistentId))
			{
				UE_LOG(LogTemp, Display,
					TEXT("[WacomGameMode] Bootstrap: 清理存档中已销毁的触发器 %s (id=%s)"),
					*Trigger->GetName(), *Trigger->PersistentId.ToString());
				Trigger->Destroy();
			}
		}

		// 2. 传送玩家到存档位置。
		const FRunState& State = Run->GetRunState();
		if (State.bHasPlayerTransform)
		{
			if (APawn* Pawn = PC ? PC->GetPawn() : nullptr)
			{
				Pawn->SetActorLocationAndRotation(
					State.PlayerTransform.GetLocation(),
					State.PlayerTransform.Rotator(),
					/*bSweep*/false,
					nullptr,
					ETeleportType::TeleportPhysics);

				// 同时把 Controller 视角对齐玩家朝向，避免瞬移后视角错乱
				if (PC)
				{
					PC->SetControlRotation(State.PlayerTransform.Rotator());
				}

				UE_LOG(LogTemp, Display,
					TEXT("[WacomGameMode] Bootstrap: 恢复玩家 Transform 到 %s"),
					*State.PlayerTransform.GetLocation().ToString());
			}
		}
	}
}

bool AWacomGameMode::SaveRunToSlot(const FString& SlotName, bool bQuiet) const
{
	// 存档系统暂停（Stage 0.1）：静默 no-op。不影响调用方流程。
	if (!bSaveSystemEnabled)
	{
		return false;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	URunSession* Run = WacomPC ? WacomPC->GetRunSession() : nullptr;
	if (!Run)
	{
		if (!bQuiet)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomGameMode] SaveRunToSlot(%s): RunSession 未就位"), *SlotName);
		}
		return false;
	}

	// 存档前：把玩家当前 Transform 记下。
	// 只在探索状态这么做；战斗中 Pawn 位置不会变，但 Save 不该在战斗中被调用。
	if (CurrentState == EGameFlowState::Exploration)
	{
		if (APawn* Pawn = PC ? PC->GetPawn() : nullptr)
		{
			Run->SetPlayerTransform(Pawn->GetActorTransform());
		}
	}

	return Run->SaveToSlot(SlotName);
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
	if (!BattleHUDClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] EnterBattle: BattleHUDClass 未配置"));
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

	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] EnterBattle: 找不到 UWacomGameUIManagerSubsystem"));
		return;
	}

	// 1) 创建 BattleSession + Initialize
	ActiveSession = NewObject<UBattleSession>(this);
	{
		FBattleInitParams Params;

		// 优先从 RunSession 构造（含角色 / 种子 / 撤离持久化的破坏部位）；
		// 若 Run 未就绪则回退到 GameMode 字段。
		const FName TriggerPersistentId = Trigger ? Trigger->PersistentId : NAME_None;
		if (!Run || !Run->BuildInitParamsForBattle(EnemyDef, TriggerPersistentId, Params))
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

	// 2) 确保 PrimaryLayout 就位（跨场景长存，Subsystem 负责生命周期）
	UIManager->EnsurePrimaryLayout(PC);

	// 3) Push BattleHUD 到 Game Layer
	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_Game.GetTag(), BattleHUDClass);
	BattleHUD = Cast<UBattleHUD>(Pushed);
	if (!BattleHUD)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] Push BattleHUD 失败"));
		ActiveSession = nullptr;
		return;
	}

	BattleHUD->SetSession(ActiveSession);

	// 订阅战斗结束广播
	BattleEndedHandle = BattleHUD->OnBattleEndedNative.AddUObject(
		this, &AWacomGameMode::HandleBattleEnded);

	// 4) 禁用探索输入 + 切 IMC
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

	// 5) 记录状态
	CurrentState        = EGameFlowState::Battle;
	PendingTrigger      = Trigger;
	PendingEnemyDefForRun = EnemyDef;

	// 战斗期间 Toast 应隐藏（即便候选列表非空）。
	if (AWacomPlayerController* WPC = Cast<AWacomPlayerController>(PC))
	{
		WPC->RefreshInteractToast();
	}

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

	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;

	// 1) 反订阅 HUD 委托。
	// Session 这里还活着，先 build packet 再清理。
	if (BattleHUD && BattleEndedHandle.IsValid())
	{
		BattleHUD->OnBattleEndedNative.Remove(BattleEndedHandle);
	}
	BattleEndedHandle.Reset();

	// 1.5) 在 Session 释放前组装战后包，传给 Run 层。
	FBattleResultPacket Packet;
	if (ActiveSession)
	{
		Packet = ActiveSession->BuildResultPacket();
	}
	else
	{
		// 异常路径（理论上不会发生）：手工填 Outcome，其他 flag 默认 false。
		Packet.Outcome = Outcome;
	}

	// 2) Pop HUD + 清理 Session
	if (UIManager && BattleHUD)
	{
		UIManager->PopContentFromLayer(BattleHUD);
	}
	BattleHUD     = nullptr;
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

	// 3) 标记触发器为已销毁（在 Destroy 前读 PersistentId）+ Destroy 本身
	const FName TriggerPersistentId = PendingTrigger ? PendingTrigger->PersistentId : NAME_None;
	if (PendingTrigger)
	{
		AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
		URunSession* Run = WacomPC ? WacomPC->GetRunSession() : nullptr;

		// 真胜利（非撤离）才标记已销毁 + Destroy。
		// 若异常路径产生"撤离但所有部位都已毁"，也按胜利清理，避免留下空血敌人反复重入。
		// 正常规则层会在最后一个存活部位被击倒时禁用撤离。
		// 失败 / 未定场景也不销毁。
		const bool bAllPartsDestroyed = PendingEnemyDefForRun
			&& Packet.DestroyedPartIds.Num() >= PendingEnemyDefForRun->Parts.Num();
		const bool bRealVictory = (Packet.Outcome == EBattleOutcome::Victory)
			&& (!Packet.bWithdrawn || bAllPartsDestroyed);

		if (bRealVictory && Run && !TriggerPersistentId.IsNone())
		{
			Run->MarkTriggerDestroyed(TriggerPersistentId);
		}

		if (bRealVictory)
		{
			PendingTrigger->Destroy();
		}
		PendingTrigger = nullptr;
	}

	// 4) 通知 RunSession 战斗结束，让它更新击败列表 / run active 状态 / 战外结算压力
	//    + 撤离时持久化破坏部位、真胜利时清理（GDD §10.5）
	if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
	{
		if (URunSession* Run = WacomPC->GetRunSession())
		{
			Run->OnBattleFinishedFromTrigger(Packet, PendingEnemyDefForRun, TriggerPersistentId);

			// 战斗结束统一结算 1 节点（GDD §10.5：胜利 / 失败 / 撤离都消耗）
			if (Packet.Outcome != EBattleOutcome::Undetermined)
			{
				Run->ConsumeNode(1);
			}
		}
	}
	PendingEnemyDefForRun = nullptr;

	// 5) 状态复位
	CurrentState = EGameFlowState::Exploration;

	// 撤离回探索时玩家仍在 Sphere 内（不会再发 BeginOverlap），
	// 但候选列表里 Trigger 还在；显式刷一次 Toast 让 ExplorationHUD 显示"按 E 战斗"。
	// 真胜利时 Trigger 已在前面 Destroy，EndPlay 会反注册候选，Toast 自然隐藏。
	if (AWacomPlayerController* WPC = Cast<AWacomPlayerController>(PC))
	{
		WPC->RefreshInteractToast();
	}

	// 6) 存档：先写 Auto 备份，再覆盖 Main。
	// 顺序很重要——如果程序在这两次 Save 之间崩掉，Auto 至少是新的，Main 还是上次的旧档，
	// 下次启动会先试 Main 失败（或读到旧数据）再回退 Auto。
	SaveRunToSlot(SlotName_Auto);
	SaveRunToSlot(SlotName_Main);
}
