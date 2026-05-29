// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomRunTunnelBranchTargetActor.h"
#include "Actors/WacomRunTunnelBranchTargetActor.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Interaction/WacomInteractionTargetProvider.h"
#include "GameFramework/WacomExplorationScreenRouter.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "Characters/CharacterDefinition.h"
#include "Interaction/WacomWorldInteractable.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UI/Battle/BattleHUD.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#define LOCTEXT_NAMESPACE "WacomPlayerController"

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

	bool IsWorldInteractableActor(const AActor* Actor)
	{
		return Actor && Actor->GetClass()->ImplementsInterface(UWacomWorldInteractable::StaticClass());
	}

	FText GetInteractPromptTextFromActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetInteractPromptText_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			? IWacomWorldInteractable::Execute_GetInteractPromptText(Actor, PC)
			: FText::GetEmpty();
	}

	FVector GetInteractLocationFromActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetInteractLocation_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			? IWacomWorldInteractable::Execute_GetInteractLocation(Actor, PC)
			: FVector::ZeroVector;
	}

	bool CanInteractWithActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->CanInteract_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			&& IWacomWorldInteractable::Execute_CanInteract(Actor, PC);
	}

	bool TryInteractWithActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomWorldInteractable* Native = Cast<IWacomWorldInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->TryInteract_Implementation(PC);
			}
		}
		return IsWorldInteractableActor(Actor)
			&& IWacomWorldInteractable::Execute_TryInteract(Actor, PC);
	}

	FString GetDebugObjectName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("None");
	}

	/**
	 * 从命中 Actor 的组件中查找首个实现了 IWacomInteractionTargetProvider 的，
	 * 构建统一的交互目标 handle。
	 */
	FWacomInteractionTargetHandle BuildInteractionTargetHandleFromHit(const FHitResult& HitResult)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
		{
			return FWacomInteractionTargetHandle();
		}

		TArray<UActorComponent*> Components;
		HitActor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (IWacomInteractionTargetProvider* Provider = Cast<IWacomInteractionTargetProvider>(Component))
			{
				FWacomInteractionTargetHandle Handle = Provider->BuildWorldTargetHandle();
				if (Handle.IsValid())
				{
					if (HitResult.HasValidHitObjectHandle() || HitResult.Location != FVector::ZeroVector)
					{
						Handle.WorldLocation = HitResult.Location;
					}
					return Handle;
				}
			}
		}

		return FWacomInteractionTargetHandle();
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

	// 探索 GameMode（AWacomGameMode）才走：应用 Exploration 输入上下文 + 建 RunSession。
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	if (GM)
	{
		if (!ExplorationMappingContext)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomPlayerController] ExplorationMappingContext 未配置，请先运行 WacomCreateInputAssets"));
		}
		if (ULocalPlayer* LP = GetLocalPlayer())
		{
			if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
				LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>())
			{
				InputCoordinator->InitializeForPlayerController(this);
				InputCoordinator->SetMappingContexts(ExplorationMappingContext, BattleMappingContext);
				InputCoordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
			}
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

		// MVVM：把 RunSession 绑到 RunViewModelProvider Subsystem，
		// ViewModel 立刻同步当前 RunState 字段。即便 RunSession::Initialize 失败也调，
		// Provider 内部会安全处理（找不到 RunSession 就跳过）。
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UWacomRunViewModelProvider* Provider = GI->GetSubsystem<UWacomRunViewModelProvider>())
			{
				Provider->BindToPlayerController(this);
			}
			if (UWacomAppToastSubsystem* ToastSubsystem = GI->GetSubsystem<UWacomAppToastSubsystem>())
			{
				ToastSubsystem->EnsureAppToastReady();
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
	LazyLoadIA(IA_OpenMenu,   TEXT("/Game/Wacom/Input/IA_OpenMenu.IA_OpenMenu"));
	LazyLoadIA(IA_OpenBackpack, TEXT("/Game/Wacom/Input/IA_OpenBackpack.IA_OpenBackpack"));
	LazyLoadIA(IA_Interact,     TEXT("/Game/Wacom/Input/IA_Interact.IA_Interact"));

	if (IA_PlayCard1) { EIC->BindAction(IA_PlayCard1, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard1); }
	if (IA_PlayCard2) { EIC->BindAction(IA_PlayCard2, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard2); }
	if (IA_PlayCard3) { EIC->BindAction(IA_PlayCard3, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard3); }
	if (IA_PlayCard4) { EIC->BindAction(IA_PlayCard4, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard4); }
	if (IA_PlayCard5) { EIC->BindAction(IA_PlayCard5, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard5); }
	if (IA_PlayCard6) { EIC->BindAction(IA_PlayCard6, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard6); }
	if (IA_PlayCard7) { EIC->BindAction(IA_PlayCard7, ETriggerEvent::Started, this, &AWacomPlayerController::OnPlayCard7); }

	if (IA_Wait)       { EIC->BindAction(IA_Wait,       ETriggerEvent::Started, this, &AWacomPlayerController::OnWaitPressed); }
	if (IA_EndTurn)    { EIC->BindAction(IA_EndTurn,    ETriggerEvent::Started, this, &AWacomPlayerController::OnEndTurnPressed); }
	if (IA_OpenMenu)   { EIC->BindAction(IA_OpenMenu,   ETriggerEvent::Started, this, &AWacomPlayerController::OnOpenMenuPressed); }
	if (IA_OpenBackpack) { EIC->BindAction(IA_OpenBackpack, ETriggerEvent::Started, this, &AWacomPlayerController::OnOpenBackpackPressed); }
	if (IA_Interact)     { EIC->BindAction(IA_Interact,     ETriggerEvent::Started, this, &AWacomPlayerController::OnInteractPressed); }
}

bool AWacomPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryRouteBattleSceneTargetClick(/*bRequireTargetSelect*/true))
	{
		return true;
	}
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryRouteRunTunnelBranchClick())
	{
		return true;
	}

	return Super::InputKey(Params);
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

bool AWacomPlayerController::TryRouteBattleSceneTargetClick(bool bRequireTargetSelect)
{
	UBattleHUD* HUD = nullptr;
	if (!CanRouteBattleSceneTargetClick(HUD))
	{
		if (bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute reason=CannotRouteBattleSceneTargetClick requireTargetSelect=%s"),
				bRequireTargetSelect ? TEXT("true") : TEXT("false"));
		}
		return false;
	}
	if (bRequireTargetSelect && (!HUD || !HUD->IsInTargetSelect()))
	{
		if (bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute reason=NotInTargetSelect hud=%s requireTargetSelect=%s"),
				*GetDebugObjectName(HUD),
				bRequireTargetSelect ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	FHitResult HitResult;
	if (!BuildBattleSceneClickHitResult(HitResult))
	{
		if (bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute reason=NoVisibilityHit hud=%s inTargetSelect=%s"),
				*GetDebugObjectName(HUD),
				HUD && HUD->IsInTargetSelect() ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	// 通过 IWacomInteractionTargetProvider 接口构建统一 handle，用于 World 目标点击路由。
	{
		const FWacomInteractionTargetHandle Handle = BuildInteractionTargetHandleFromHit(HitResult);
		if (Handle.IsValid() && Handle.TargetKind == EWacomInteractionTargetKind::World
			&& Handle.TargetTag == WacomTags::Interaction_Target_Battle_EnemyPart
			&& Handle.WorldTargetId.IsValid())
		{
			HUD->OnEnemyPartClickedByUser(Handle.WorldTargetId);
			if (bLogBattleSceneTargetClickRouting)
			{
				UE_LOG(LogTemp, Display,
					TEXT("[WacomBattleSceneClickRouter] RouteViaProvider handle=%s inTargetSelect=%s"),
					*Handle.ToString(),
					HUD && HUD->IsInTargetSelect() ? TEXT("true") : TEXT("false"));
			}
			return true;
		}

		if (bLogBattleSceneTargetClickRouting)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomBattleSceneClickRouter] NoRoute handleValid=%s hitActor=%s"),
				Handle.IsValid() ? TEXT("true") : TEXT("false"),
				*GetDebugObjectName(HitResult.GetActor()));
		}
		return false;
	}
}

bool AWacomPlayerController::CanRouteBattleSceneTargetClick(UBattleHUD*& OutHUD) const
{
	OutHUD = nullptr;
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	if (!GM || GM->GetGameFlowState() != EGameFlowState::Battle)
	{
		return false;
	}

	OutHUD = GM->GetActiveBattleHUD();
	return OutHUD != nullptr;
}

bool AWacomPlayerController::BuildBattleSceneClickHitResult(FHitResult& OutHitResult) const
{
	return GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult);
}

bool AWacomPlayerController::TryRouteRunTunnelBranchClick()
{
	AWacomPlayerCharacter* WacomCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	UWacomRunTunnelMovementComponent* TunnelComponent =
		WacomCharacter ? WacomCharacter->GetRunTunnelMovementComponent() : nullptr;
	if (!TunnelComponent
		|| !TunnelComponent->IsRunTunnelActive()
		|| TunnelComponent->IsRunTunnelSuspended())
	{
		return false;
	}

	FHitResult HitResult;
	if (!BuildRunTunnelBranchClickHitResult(HitResult))
	{
		return false;
	}

	AWacomRunTunnelBranchTargetActor* BranchTarget = Cast<AWacomRunTunnelBranchTargetActor>(HitResult.GetActor());
	if (!BranchTarget && HitResult.GetComponent())
	{
		BranchTarget = Cast<AWacomRunTunnelBranchTargetActor>(HitResult.GetComponent()->GetOwner());
	}
	return BranchTarget && BranchTarget->RequestBranch(TunnelComponent);
}

bool AWacomPlayerController::BuildRunTunnelBranchClickHitResult(FHitResult& OutHitResult) const
{
	return GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult);
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

void AWacomPlayerController::OnOpenMenuPressed()
{
	FWacomExplorationScreenRouter::TogglePauseMenu(*this);
}

// ================ 背包入口 ================

namespace
{
	/**
	 * 找到第一个本地 AWacomPlayerController。
	 * 用于 console command 等无 PC 上下文的调试入口。
	 */
	AWacomPlayerController* FindLocalWacomPC(UWorld* World)
	{
		if (!World) { return nullptr; }
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AWacomPlayerController* WPC = Cast<AWacomPlayerController>(It->Get()))
			{
				return WPC;
			}
		}
		return nullptr;
	}
}

void AWacomPlayerController::OnOpenBackpackPressed()
{
	TryOpenBackpackFromConsole();
}

// ================ 候选世界交互对象（use-key 模型）================

void AWacomPlayerController::RegisterCandidateInteractable(AActor* InteractableActor)
{
	if (!IsWorldInteractableActor(InteractableActor))
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& Weak : CandidateInteractables)
	{
		if (Weak.Get() == InteractableActor) { return; }
	}
	CandidateInteractables.Add(InteractableActor);
	RefreshInteractToast();
}

void AWacomPlayerController::UnregisterCandidateInteractable(AActor* InteractableActor)
{
	const int32 NumRemoved = CandidateInteractables.RemoveAllSwap(
		[InteractableActor](const TWeakObjectPtr<AActor>& Weak)
		{
			return !Weak.IsValid() || Weak.Get() == InteractableActor;
		});
	if (NumRemoved > 0)
	{
		RefreshInteractToast();
	}
}

void AWacomPlayerController::RegisterCandidateTrigger(ABattleTriggerActor* Trigger)
{
	RegisterCandidateInteractable(Trigger);
}

void AWacomPlayerController::UnregisterCandidateTrigger(ABattleTriggerActor* Trigger)
{
	UnregisterCandidateInteractable(Trigger);
}

AActor* AWacomPlayerController::PickClosestInteractable() const
{
	APawn* OwnedPawn = GetPawn();
	if (!OwnedPawn) { return nullptr; }
	const FVector PlayerLoc = OwnedPawn->GetActorLocation();

	AActor* Best = nullptr;
	float BestSqDist = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AActor>& Weak : CandidateInteractables)
	{
		AActor* Candidate = Weak.Get();
		if (!IsWorldInteractableActor(Candidate))
		{
			continue;
		}
		if (!CanInteractWithActor(Candidate, const_cast<AWacomPlayerController*>(this)))
		{
			continue;
		}

		const FVector InteractLoc = GetInteractLocationFromActor(
			Candidate,
			const_cast<AWacomPlayerController*>(this));
		const float SqDist = FVector::DistSquared(InteractLoc, PlayerLoc);
		if (SqDist < BestSqDist)
		{
			BestSqDist = SqDist;
			Best       = Candidate;
		}
	}
	return Best;
}

FText AWacomPlayerController::BuildCurrentInteractPrompt() const
{
	AActor* Best = PickClosestInteractable();
	if (!Best)
	{
		return FText::GetEmpty();
	}
	return GetInteractPromptTextFromActor(Best, const_cast<AWacomPlayerController*>(this));
}

void AWacomPlayerController::RefreshInteractToast()
{
	// 只在探索状态显示 Toast；战斗中即便候选列表非空也不显示。
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	const bool bExploration = GM && GM->GetGameFlowState() == EGameFlowState::Exploration;

	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	UWacomPrimaryGameLayout* Layout = UIManager ? UIManager->GetPrimaryLayout() : nullptr;
	if (!Layout) { return; }

	UCommonActivatableWidgetStack* GameStack = Layout->GetLayerStack(
		WacomUITags::UI_Layer_Game.GetTag());
	UWacomExplorationHUD* HUD = GameStack
		? Cast<UWacomExplorationHUD>(GameStack->GetActiveWidget())
		: nullptr;
	if (!HUD) { return; }

	const FText Prompt = BuildCurrentInteractPrompt();

	HUD->SetInteractToastVisible(
		bExploration && !Prompt.IsEmpty(),
		Prompt);
}

void AWacomPlayerController::OnInteractPressed()
{
	TryInteractFromConsole();
}

void AWacomPlayerController::TryInteractFromConsole()
{
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	if (!GM || GM->GetGameFlowState() != EGameFlowState::Exploration)
	{
		return;
	}

	AActor* Best = PickClosestInteractable();
	if (!Best)
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] Interact: 没有候选交互对象"));
		return;
	}
	TryInteractWithActor(Best, this);
}

bool AWacomPlayerController::RequestOpenShop(FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	return FWacomExplorationScreenRouter::OpenShop(*this, ShopId, Offers);
}

bool AWacomPlayerController::RequestOpenRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	return FWacomExplorationScreenRouter::OpenRunEvent(*this, PersistentId, EventDefinition);
}

void AWacomPlayerController::TryOpenBackpackFromConsole()
{
	FWacomExplorationScreenRouter::OpenBackpack(*this);
}

// ---- Console Commands（调试入口）----

static FAutoConsoleCommandWithWorld GWacomOpenBackpackCmd(
	TEXT("Wacom.OpenBackpack"),
	TEXT("打开背包界面（探索 GameMode 才生效）。"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AWacomPlayerController* WPC = FindLocalWacomPC(World))
		{
			WPC->TryOpenBackpackFromConsole();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Wacom.OpenBackpack] 找不到 AWacomPlayerController"));
		}
	}));

static FAutoConsoleCommandWithWorld GWacomCloseBackpackCmd(
	TEXT("Wacom.CloseBackpack"),
	TEXT("关闭 GameMenu 层最顶上的界面。"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AWacomPlayerController* WPC = FindLocalWacomPC(World))
		{
			FWacomExplorationScreenRouter::CloseTopGameMenu(*WPC);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Wacom.CloseBackpack] 找不到 AWacomPlayerController"));
		}
	}));

static FAutoConsoleCommandWithWorld GWacomInteractCmd(
	TEXT("Wacom.Interact"),
	TEXT("在世界交互对象范围内时触发最近对象。"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AWacomPlayerController* WPC = FindLocalWacomPC(World))
		{
			WPC->TryInteractFromConsole();
		}
	}));

#undef LOCTEXT_NAMESPACE

