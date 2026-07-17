// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomGameMode.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Characters/CharacterDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Session/BattleSession.h"

#include "Actors/BattleTriggerActor.h"
#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewStageReturnFlow.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "GameFramework/WacomResolvedEncounterSceneRetirementPolicy.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "RunSession.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomJourneySummaryScreen.h"

const FString AWacomGameMode::SlotName_Main = TEXT("Main");
const FString AWacomGameMode::SlotName_Auto = TEXT("Auto");

namespace
{
	const FName JourneySummaryMainMenuPackagePath(TEXT("/Game/Wacom/Maps/L_MainMenu"));

	struct FExitBattlePostRunBarrierState
	{
		explicit FExitBattlePostRunBarrierState(TFunction<void()>&& InOnReady)
			: OnReady(MoveTemp(InOnReady))
		{
		}

		void MarkReturnCompleted()
		{
			bReturnCompleted = true;
			TryComplete();
		}

		void MarkExitBattlePostRunReady()
		{
			bExitBattlePostRunReady = true;
			TryComplete();
		}

		void SetResolvedEncounterTrigger(ABattleTriggerActor* InTrigger)
		{
			WeakResolvedEncounterTrigger = InTrigger;
		}

	private:
		void TryComplete()
		{
			if (!bReturnCompleted || !bExitBattlePostRunReady || bCompleted)
			{
				return;
			}

			bCompleted = true;
			if (ABattleTriggerActor* Trigger = WeakResolvedEncounterTrigger.Get())
			{
				Trigger->CompleteResolvedEncounterSceneRetirement();
			}
			if (OnReady)
			{
				OnReady();
			}
		}

		TFunction<void()> OnReady;
		TWeakObjectPtr<ABattleTriggerActor> WeakResolvedEncounterTrigger;
		bool bReturnCompleted = false;
		bool bExitBattlePostRunReady = false;
		bool bCompleted = false;
	};
}

AWacomGameMode::AWacomGameMode()
{
	PlayerControllerClass = AWacomPlayerController::StaticClass();
	DefaultPawnClass      = AWacomPlayerCharacter::StaticClass();
}

FName AWacomGameMode::GetJourneySummaryMainMenuLevelPackagePathForTravel()
{
	return JourneySummaryMainMenuPackagePath;
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
			TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
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
	if (!JourneySummaryScreenClass)
	{
		JourneySummaryScreenClass = UWacomJourneySummaryScreen::StaticClass();
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

	// 从主菜单 OpenLevel 过来时，PC 可能复用；这里主动恢复探索输入 profile。
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			if (ULocalPlayer* LP = WacomPC->GetLocalPlayer())
			{
				if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
					LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>())
				{
					InputCoordinator->InitializeForPlayerController(WacomPC);
					InputCoordinator->SetMappingContexts(WacomPC->ExplorationMappingContext, WacomPC->BattleMappingContext);
					InputCoordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
				}
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
	UnbindJourneySummaryScreen();

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
			TEXT("[WacomGameMode] EndPlay 期间不处于活动探索，按规则不写活动 Run 存档。State=%d"),
			static_cast<int32>(CurrentState));
	}

	Super::EndPlay(EndPlayReason);
}

// ================ 存档引导 ================

void AWacomGameMode::BootstrapRunFromSave()
{
	// 存档系统关闭时直接走新 Run，不读盘。
	if (!bSaveSystemEnabled)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomGameMode] Bootstrap: 存档系统暂停，保持新 Run"));
		if (AWacomPlayerController* WacomPC =
			Cast<AWacomPlayerController>(GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr))
		{
			WacomPC->PrepareExplorationRunFirstPersonCardLayer();
		}
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
		// 1. 退役存档中已标记为完成的 Encounter 场景，再销毁对应触发器。
		// Trigger::BeginPlay 做过一次自销毁检查，但那时 Bootstrap 尚未加载存档，
		// RunSession 是空的，所以还得在这里补一刀。先退役 Host，避免读档后留下 Idle 敌人。
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
				Trigger->CompleteResolvedEncounterSceneRetirement();
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

	if (WacomPC)
	{
		WacomPC->PrepareExplorationRunFirstPersonCardLayer();
	}
}

bool AWacomGameMode::SaveRunToSlot(const FString& SlotName, bool bQuiet) const
{
	// 存档系统关闭时静默 no-op，不影响调用方流程。
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

void AWacomGameMode::EnterBattle(ABattleTriggerActor* Trigger)
{
	if (CurrentState == EGameFlowState::Battle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] EnterBattle 被重复调用，忽略"));
		return;
	}
	if (!Trigger)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] EnterBattle 缺少 BattleTrigger，忽略"));
		return;
	}

	TArray<FBattleEnemySlotInit> EncounterEnemySlots;
	Trigger->BuildBattleEnemySlots(EncounterEnemySlots);
	if (EncounterEnemySlots.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomGameMode] EnterBattle 缺少有效 EncounterDefinition，忽略"));
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
	if (!Run)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] EnterBattle: RunSession 未就位，拒绝脱离 RunState 的战斗"));
		return;
	}

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
	int32 EnterBattleEnemySlotCount = 0;
	const UEnemyDefinition* FirstEnemySlotDefinition = nullptr;
	FBattleInitializationResult BattleInitialization;
	{
		FBattleInitParams Params;

		FirstEnemySlotDefinition = EncounterEnemySlots[0].Enemy;

		// RunSession 是进入战斗的唯一规则入口：角色、种子、备战卡组和撤离重入进度都来自 RunState。
		const FName TriggerPersistentId = Trigger ? Trigger->PersistentId : NAME_None;
		if (!Run->BuildInitParamsForBattle(TriggerPersistentId, Params))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomGameMode] EnterBattle: RunSession 无法构造战斗参数，拒绝 fallback 战斗"));
			return;
		}

		// 敌人侧由 App/Trigger 层负责：EncounterDefinition -> EnemySlots。
		Params.EnemySlots = MoveTemp(EncounterEnemySlots);
		EnterBattleEnemySlotCount = Params.EnemySlots.Num() > 0 ? Params.EnemySlots.Num() : 1;

		ActiveSession = NewObject<UBattleSession>(this);
		BattleInitialization = ActiveSession->Initialize(Params);
		if (!BattleInitialization.IsOk())
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] Session Initialize 失败 Code=%d"),
				(int32)BattleInitialization.Status.Code);
			ActiveSession = nullptr;
			return;
		}
	}

	// Battle 表现启动前先取得 Encounter 活动所有权。规则层只预留 1 AP；
	// Victory 在结果提交时消费，Withdraw/Defeat/启动失败会释放预留。
	const FRunExplorationResolution EncounterBegin =
		Run->BeginCurrentNodeActivity(ERunNodeActivityKind::Encounter);
	if (!EncounterBegin.IsOk() || !EncounterBegin.NodeActivityTicket.IsSet())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] EnterBattle: 当前逻辑节点无法开始 Encounter，拒绝场景战斗。Detail=%s"),
			*EncounterBegin.Status.Detail.ToString());
		ActiveSession = nullptr;
		return;
	}
	if (!WacomPC->ApplyRunNodeActivityResolutionForPresentation(EncounterBegin))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] EnterBattle: Encounter Begin 未能同步到 Run 表现，取消活动并拒绝进入战斗"));
		const FRunExplorationResolution Cancellation =
			Run->CancelNodeActivity(EncounterBegin.NodeActivityTicket.GetValue());
		if (Cancellation.IsOk())
		{
			WacomPC->ApplyRunNodeActivityResolutionForPresentation(Cancellation);
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomGameMode] EnterBattle: Encounter Begin 回滚失败。Detail=%s"),
				*Cancellation.Status.Detail.ToString());
		}
		ActiveSession = nullptr;
		return;
	}
	PendingEncounterActivity = EncounterBegin.NodeActivityTicket.GetValue();

	// 2) 确保 PrimaryLayout 就位（跨场景长存，Subsystem 负责生命周期）
	UIManager->EnsurePrimaryLayout(PC);

	// 3) Push BattleHUD 到 Game Layer
	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_Game.GetTag(), BattleHUDClass);
	BattleHUD = Cast<UBattleHUD>(Pushed);
	if (!BattleHUD)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomGameMode] Push BattleHUD 失败"));
		const FRunExplorationResolution Cancellation =
			Run->CancelNodeActivity(PendingEncounterActivity.GetValue());
		if (Cancellation.IsOk())
		{
			WacomPC->ApplyRunNodeActivityResolutionForPresentation(Cancellation);
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomGameMode] Push BattleHUD 失败后的 Encounter 取消失败。Detail=%s"),
				*Cancellation.Status.Detail.ToString());
		}
		PendingEncounterActivity.Reset();
		ActiveSession = nullptr;
		return;
	}

	if (WacomPC)
	{
		WacomPC->ClearRunFirstPersonCardLayer();
	}

	BattleHUD->BeginBattleEntryPresentation();
	BattleHUD->AttachInitializedBattleSession(ActiveSession, MoveTemp(BattleInitialization));
	TArray<AWacomBattleEnemyActor*> SceneEnemyHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneEnemyHosts);
	BattleHUD->SetBattleSceneEnemyHosts(SceneEnemyHosts);

	// 订阅战斗结束广播
	BattleEndedHandle = BattleHUD->OnBattleEndedNative.AddUObject(
		this, &AWacomGameMode::HandleBattleEnded);

	// 4) 禁用探索输入 + 切输入上下文
	if (WacomPC)
	{
		if (ULocalPlayer* LP = WacomPC->GetLocalPlayer())
		{
			if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
				LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>())
			{
				InputCoordinator->InitializeForPlayerController(WacomPC);
				InputCoordinator->SetMappingContexts(WacomPC->ExplorationMappingContext, WacomPC->BattleMappingContext);
				InputCoordinator->SetFlowContext(EWacomInputFlowContext::Battle);
			}
		}
	}

	bool bBattleCameraActivationDeferred = false;
	if (AWacomPlayerCharacter* Pawn = PC->GetPawn<AWacomPlayerCharacter>())
	{
		Pawn->SetExplorationInputEnabled(false);
		FWacomFirstPersonViewStageRequest BattleEntryStageRequest;
		Trigger->TryBuildBattleEntryViewStageRequest(BattleEntryStageRequest);

		const TWeakObjectPtr<AWacomGameMode> WeakGameMode(this);
		const TWeakObjectPtr<AWacomPlayerCharacter> WeakPawn(Pawn);
		bBattleCameraActivationDeferred =
			FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
				*Pawn,
				*PC,
				BattleEntryStageRequest,
				[WeakGameMode, WeakPawn]()
				{
					AWacomGameMode* GameMode = WeakGameMode.Get();
					if (!GameMode
						|| !WeakPawn.Get()
						|| GameMode->CurrentState != EGameFlowState::Battle)
					{
						return;
					}
					if (GameMode->BattleHUD)
					{
						GameMode->BattleHUD->ReleaseBattleEntryPresentation();
					}
				});
	}

	if (!bBattleCameraActivationDeferred)
	{
		BattleHUD->ReleaseBattleEntryPresentation();
	}

	// 5) 记录状态
	CurrentState        = EGameFlowState::Battle;
	PendingTrigger      = Trigger;

	// 战斗期间 Toast 应隐藏（即便候选列表非空）。
	if (AWacomPlayerController* WPC = Cast<AWacomPlayerController>(PC))
	{
		WPC->RefreshInteractToast();
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomGameMode] EnterBattle 完成：FirstEnemySlotDefinition=%s EncounterSlots=%d"),
		*GetNameSafe(FirstEnemySlotDefinition),
		EnterBattleEnemySlotCount);
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
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	URunSession* Run = WacomPC ? WacomPC->GetRunSession() : nullptr;
	PendingJourneySummaryViewData.Reset();
	bJourneySummarySuccessEventConsumed = false;
	bool bJourneySucceeded = false;

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

	// 在销毁场景 Trigger 或恢复探索表现前，先以 Begin 时票据原子提交战果。
	// 无效/未定结果只取消预留，绝不回退到旧 no-token 结算或手工扣点。
	bool bEncounterSettlementSucceeded = false;
	if (Run && PendingEncounterActivity.IsSet())
	{
		FRunExplorationResolution EncounterResult;
		if (Packet.Outcome == EBattleOutcome::Undetermined)
		{
			EncounterResult = Run->CancelNodeActivity(PendingEncounterActivity.GetValue());
		}
		else
		{
			EncounterResult = Run->SettleEncounterNodeActivity(
				PendingEncounterActivity.GetValue(),
				Packet);
			bEncounterSettlementSucceeded = EncounterResult.IsOk();
			if (!EncounterResult.IsOk())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[WacomGameMode] Encounter 战果提交失败，取消活动以解除探索锁。Detail=%s"),
					*EncounterResult.Status.Detail.ToString());
				EncounterResult = Run->CancelNodeActivity(
					PendingEncounterActivity.GetValue());
			}
		}
		if (EncounterResult.IsOk())
		{
			bJourneySucceeded = ConsumeJourneySucceededEvent(EncounterResult, *Run);
			if (!WacomPC->ApplyRunNodeActivityResolutionForPresentation(EncounterResult))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[WacomGameMode] ExitBattle: Encounter 结果未按序应用到 Run 表现；已尝试恢复当前 Session 绑定"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomGameMode] Encounter 结算与取消均失败，探索活动仍可能被锁定。Detail=%s"),
				*EncounterResult.Status.Detail.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] ExitBattle 缺少 RunSession 或 Encounter 票据，战果不会结算"));
	}
	PendingEncounterActivity.Reset();

	TWeakObjectPtr<AWacomGameMode> WeakGameMode(this);
	TWeakObjectPtr<AWacomPlayerController> WeakWacomPC(WacomPC);
	const TSharedRef<FExitBattlePostRunBarrierState> PostRunBarrier =
		MakeShared<FExitBattlePostRunBarrierState>(
			[WeakGameMode, WeakWacomPC, bJourneySucceeded]()
			{
				if (AWacomGameMode* GameMode = WeakGameMode.Get())
				{
					GameMode->HandleExitBattlePostRunBarrier(
						bJourneySucceeded,
						WeakWacomPC.Get());
				}
			});

	// Map Node 结算成功才是 Encounter 完成真相。SaveGame 的 Trigger Id 只是兼容投影；
	// 先禁用 Trigger，Host 继续保留 Downed 终态，等返回探索双 barrier 完成后统一退役。
	const bool bShouldRetireResolvedEncounterScene =
		WacomResolvedEncounterSceneRetirementPolicy::ShouldRetire(
			bEncounterSettlementSucceeded,
			Packet.Outcome,
			Packet.bWithdrawn);
	if (bShouldRetireResolvedEncounterScene && PendingTrigger)
	{
		if (Run && !PendingTrigger->PersistentId.IsNone())
		{
			Run->MarkTriggerDestroyed(PendingTrigger->PersistentId);
		}
		PendingTrigger->BeginResolvedEncounterSceneRetirement();
		PostRunBarrier->SetResolvedEncounterTrigger(PendingTrigger);
	}
	PendingTrigger = nullptr;

	// 2) Pop HUD + 清理 Session
	if (UIManager && BattleHUD)
	{
		BattleHUD->SetBattleSceneEnemyHosts({});
		UIManager->PopContentFromLayer(BattleHUD);
	}
	BattleHUD     = nullptr;
	ActiveSession = nullptr;

	// 2) 切回探索输入上下文；探索输入会在镜头回到 Run Path 后恢复。
	if (WacomPC)
	{
		// 回 Run Path staging 期间清空探索手牌，避免卡牌跟着相机从战斗 Viewpoint 平移回样条。
		WacomPC->ClearRunFirstPersonCardLayer();
		if (ULocalPlayer* LP = WacomPC->GetLocalPlayer())
		{
			if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
				LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>())
			{
				InputCoordinator->InitializeForPlayerController(WacomPC);
				InputCoordinator->SetMappingContexts(WacomPC->ExplorationMappingContext, WacomPC->BattleMappingContext);
				InputCoordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
			}
		}
	}
	bool bRunReturnFlowStarted = false;
	if (PC)
	{
		if (AWacomPlayerCharacter* Pawn = PC->GetPawn<AWacomPlayerCharacter>())
		{
			if (UWacomBattleCameraLookComponent* BattleCameraLook = Pawn->GetBattleCameraLookComponent())
			{
				BattleCameraLook->DeactivateBattleCameraLookPreservingView();
			}

			bRunReturnFlowStarted = true;
			FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
				*Pawn,
				*PC,
				[PostRunBarrier]()
				{
					PostRunBarrier->MarkReturnCompleted();
				});
		}
	}
	if (!bRunReturnFlowStarted)
	{
		PostRunBarrier->MarkReturnCompleted();
	}

	// 3) Run 规则结果已在上方通过 Encounter ticket 一次性提交。

	// 4) 状态复位
	CurrentState = bJourneySucceeded
		? EGameFlowState::JourneySummary
		: EGameFlowState::Exploration;

	// 撤离回探索时玩家仍在 Sphere 内（不会再发 BeginOverlap），但候选列表里 Trigger 还在。
	// Run 手牌和 Toast 等 return staging 完成后再恢复，避免退出战斗时从 Viewpoint 平移回样条。
	PostRunBarrier->MarkExitBattlePostRunReady();

	// 5) 存档：先写 Auto 备份，再覆盖 Main。
	// 顺序很重要——如果程序在这两次 Save 之间崩掉，Auto 至少是新的，Main 还是上次的旧档，
	// 下次启动会先试 Main 失败（或读到旧数据）再回退 Auto。
	SaveRunToSlot(SlotName_Auto);
	SaveRunToSlot(SlotName_Main);
}

bool AWacomGameMode::ConsumeJourneySucceededEvent(
	const FRunExplorationResolution& Resolution,
	const URunSession& RunSession)
{
	const FRunExplorationEvent* SuccessEvent = Resolution.Events.FindByPredicate(
		[](const FRunExplorationEvent& Event)
		{
			return Event.Type == ERunExplorationEventType::JourneySucceeded;
		});
	if (!SuccessEvent)
	{
		return false;
	}

	bJourneySummarySuccessEventConsumed = true;
	const FRunExplorationSnapshot& Snapshot = Resolution.PostSnapshot;
	const FRunCompletionSummary& Summary = Snapshot.CompletionSummary;
	if (Snapshot.Outcome != ERunOutcome::Succeeded
		|| !Snapshot.bHasCompletionSummary
		|| !Summary.IsValid()
		|| SuccessEvent->Detail != Summary.JourneyId
		|| SuccessEvent->Node != Summary.TerminalNode)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] JourneySucceeded event 缺少匹配的合法 PostSnapshot summary；跳过 Screen 并走安全主菜单交接"));
		PendingJourneySummaryViewData.Reset();
		return true;
	}

	FWacomJourneySummaryViewData ViewData;
	ViewData.StatusTitle = NSLOCTEXT("WacomJourneySummary", "SuccessStatus", "Journey 成功");
	const UWacomJourneyDefinition* Journey =
		RunSession.GetRunState().ExplorationState.JourneyDefinition;
	ViewData.JourneyTitle = Journey && !Journey->DisplayName.IsEmpty()
		? Journey->DisplayName
		: FText::FromName(SuccessEvent->Detail);
	ViewData.CompletionDay = Summary.CompletionDay;
	ViewData.EnteredFloorCount = Summary.EnteredFloorCount;
	ViewData.TotalFloorCount = Summary.TotalFloorCount;
	ViewData.ResolvedNodeCount = Summary.ResolvedNodeCount;
	ViewData.TotalNodeCount = Summary.TotalNodeCount;
	ViewData.FinalPressure = Summary.FinalPressure;
	PendingJourneySummaryViewData = MoveTemp(ViewData);
	return true;
}

void AWacomGameMode::HandleExitBattlePostRunBarrier(
	bool bJourneySucceeded,
	AWacomPlayerController* WacomPC)
{
	bJourneySummaryBarrierCompleted = true;
	if (bJourneySucceeded)
	{
		// 终局不恢复 Run 手牌或交互 Toast；镜头 staging 完成后直接进入总结。
		ShowJourneySummaryOrTravel();
		return;
	}

	bJourneySummaryRunPresentationRestoreRequested = true;
	if (WacomPC)
	{
		WacomPC->PrepareExplorationRunFirstPersonCardLayer();
		WacomPC->RefreshInteractToast();
	}
}

void AWacomGameMode::ShowJourneySummaryOrTravel()
{
	if (CurrentState != EGameFlowState::JourneySummary)
	{
		return;
	}

	bJourneySummaryPushAttempted = true;
	if (!PendingJourneySummaryViewData.IsSet() || !JourneySummaryScreenClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] Journey Summary 缺少 ViewData 或 Screen class，执行安全主菜单交接"));
		RequestJourneySummaryMainMenuHandoff();
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!PC || !UIManager)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] Journey Summary 缺少 PlayerController/UIManager，执行安全主菜单交接"));
		RequestJourneySummaryMainMenuHandoff();
		return;
	}

	UIManager->EnsurePrimaryLayout(PC);
	UWacomJourneySummaryScreen* Screen = Cast<UWacomJourneySummaryScreen>(
		UIManager->PushContentToLayer(
			WacomUITags::UI_Layer_GameMenu.GetTag(),
			JourneySummaryScreenClass));
	if (!Screen)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomGameMode] Push Journey Summary Screen 失败，执行安全主菜单交接"));
		RequestJourneySummaryMainMenuHandoff();
		return;
	}

	bJourneySummaryPushSucceeded = true;
	BindJourneySummaryScreen(*Screen);
	Screen->ApplyViewData(PendingJourneySummaryViewData.GetValue());
}

void AWacomGameMode::BindJourneySummaryScreen(UWacomJourneySummaryScreen& Screen)
{
	UnbindJourneySummaryScreen();
	ActiveJourneySummaryScreen = &Screen;
	Screen.OnContinueRequestedNative.RemoveAll(this);
	Screen.OnContinueRequestedNative.AddUObject(
		this,
		&AWacomGameMode::HandleJourneySummaryContinueRequested);
}

void AWacomGameMode::UnbindJourneySummaryScreen()
{
	if (UWacomJourneySummaryScreen* Screen = ActiveJourneySummaryScreen.Get())
	{
		Screen->OnContinueRequestedNative.RemoveAll(this);
	}
	ActiveJourneySummaryScreen.Reset();
}

void AWacomGameMode::HandleJourneySummaryContinueRequested()
{
	RequestJourneySummaryMainMenuHandoff();
}

void AWacomGameMode::RequestJourneySummaryMainMenuHandoff()
{
	if (bJourneySummaryHandoffRequested)
	{
		return;
	}
	bJourneySummaryHandoffRequested = true;
	++JourneySummaryHandoffRequestCount;
	UnbindJourneySummaryScreen();

	bJourneySummaryPrimaryLayoutTeardownRequested = true;
	JourneySummaryTeardownOrder = ++JourneySummaryTravelOrderCounter;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWacomGameUIManagerSubsystem* UIManager =
			GI->GetSubsystem<UWacomGameUIManagerSubsystem>())
		{
			UIManager->TearDownPrimaryLayout();
			bJourneySummaryPrimaryLayoutTeardownCompleted = true;
		}
	}

	PendingJourneySummaryTravelLevelName = JourneySummaryMainMenuPackagePath;
	bJourneySummaryTravelScheduled = true;
	JourneySummaryScheduleOrder = ++JourneySummaryTravelOrderCounter;

	UE_LOG(LogTemp, Display,
		TEXT("[WacomGameMode] Schedule JourneySummary travel Target=%s Teardown=%s"),
		*PendingJourneySummaryTravelLevelName.ToString(),
		bJourneySummaryPrimaryLayoutTeardownCompleted ? TEXT("true") : TEXT("false"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this,
				&AWacomGameMode::ExecuteJourneySummaryMainMenuTravel));
	}
}

void AWacomGameMode::ExecuteJourneySummaryMainMenuTravel()
{
	if (PendingJourneySummaryTravelLevelName.IsNone())
	{
		return;
	}

	const FName LevelName = PendingJourneySummaryTravelLevelName;
	PendingJourneySummaryTravelLevelName = NAME_None;
	bJourneySummaryTravelExecuted = true;
	JourneySummaryExecuteOrder = ++JourneySummaryTravelOrderCounter;

	UE_LOG(LogTemp, Display,
		TEXT("[WacomGameMode] Execute JourneySummary travel Target=%s SuppressedForAutomation=%s"),
		*LevelName.ToString(),
		bSuppressJourneySummaryTravelForAutomation ? TEXT("true") : TEXT("false"));

	if (!bSuppressJourneySummaryTravelForAutomation)
	{
		UGameplayStatics::OpenLevel(this, LevelName);
	}
}

#if WITH_AUTOMATION_TESTS
FWacomJourneySummaryHandoffAutomationTestView
AWacomGameMode::GetJourneySummaryHandoffAutomationTestView() const
{
	FWacomJourneySummaryHandoffAutomationTestView View;
	View.bSuccessEventConsumed = bJourneySummarySuccessEventConsumed;
	View.bBarrierCompleted = bJourneySummaryBarrierCompleted;
	View.bRunPresentationRestoreRequested = bJourneySummaryRunPresentationRestoreRequested;
	View.bSummaryPushAttempted = bJourneySummaryPushAttempted;
	View.bSummaryPushSucceeded = bJourneySummaryPushSucceeded;
	View.bHandoffRequested = bJourneySummaryHandoffRequested;
	View.bPrimaryLayoutTeardownRequested = bJourneySummaryPrimaryLayoutTeardownRequested;
	View.bPrimaryLayoutTeardownCompleted = bJourneySummaryPrimaryLayoutTeardownCompleted;
	View.bTravelScheduled = bJourneySummaryTravelScheduled;
	View.bTravelExecuted = bJourneySummaryTravelExecuted;
	View.bActualTravelSuppressed = bSuppressJourneySummaryTravelForAutomation;
	View.HandoffRequestCount = JourneySummaryHandoffRequestCount;
	View.TeardownOrder = JourneySummaryTeardownOrder;
	View.ScheduleOrder = JourneySummaryScheduleOrder;
	View.ExecuteOrder = JourneySummaryExecuteOrder;
	View.TravelLevelName = bJourneySummaryTravelExecuted
		? JourneySummaryMainMenuPackagePath
		: PendingJourneySummaryTravelLevelName;
	if (PendingJourneySummaryViewData.IsSet())
	{
		View.ViewData = PendingJourneySummaryViewData.GetValue();
	}
	return View;
}
#endif
