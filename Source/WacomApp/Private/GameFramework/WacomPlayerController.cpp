// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Camera/WacomFirstPersonViewStageReturnFlow.h"
#include "Cards/CardDefinition.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomRunEncounterSceneBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "GameFramework/WacomBattleSceneInteractionRouter.h"
#include "GameFramework/WacomBattleEnemyPartInteractionQueryPolicy.h"
#include "GameFramework/WacomRunWorldInteractionRouter.h"
#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"
#include "GameFramework/WacomRunFloorSceneDescriptorResolver.h"
#include "GameFramework/WacomRunPathBranchSelectionController.h"
#include "GameFramework/WacomRunSceneBindingRegistry.h"
#include "Interaction/WacomRunWorldCardDropReceiver.h"
#include "Interaction/WacomInteractionTargetHitResolver.h"
#include "Interaction/WacomInteractionCollisionChannels.h"
#include "GameFramework/WacomExplorationScreenRouter.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "Characters/CharacterDefinition.h"
#include "Interaction/WacomWorldInteractableContractHelpers.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "RunStateTypes.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Run/WacomRunFirstPersonCardDetailController.h"
#include "UI/Run/WacomRunFirstPersonCardDropCoordinator.h"
#include "UI/Run/WacomRunFirstPersonCardDragController.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"
#include "UI/Run/WacomRunMenuWidgetBase.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/Map/WacomRunMapScreenFlow.h"
#include "UI/Map/WacomRunMapOpenGuard.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Framework/Application/SlateApplication.h"
#include "EngineUtils.h"

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

	FString GetDebugObjectName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("None");
	}

	bool ApplyRunExplorationCursorLookOverride(
		AWacomPlayerCharacter* Character,
		const FVector2D NormalizedCursor,
		const float Scale,
		const float InterpSpeedOverride)
	{
		if (!Character)
		{
			return false;
		}
		if (UWacomRunPathTraversalComponent* RunPath =
			Character->GetRunPathTraversalComponent())
		{
			const EWacomRunPathTraversalState State = RunPath->GetTraversalState();
			if (State == EWacomRunPathTraversalState::Anchored
				|| State == EWacomRunPathTraversalState::Traversing)
			{
				RunPath->SetCursorLookOverrideNormalized(
					NormalizedCursor, Scale, InterpSpeedOverride);
				return true;
			}
		}
		return false;
	}

	void ClearRunExplorationCursorLookOverride(AWacomPlayerCharacter* Character)
	{
		if (!Character)
		{
			return;
		}
		if (UWacomRunPathTraversalComponent* RunPath =
			Character->GetRunPathTraversalComponent())
		{
			RunPath->ClearCursorLookOverride();
		}
	}

}

AWacomPlayerController::AWacomPlayerController()
{
	RunFirstPersonCardSourceComponent =
		CreateDefaultSubobject<UWacomRunFirstPersonCardSourceComponent>(
			TEXT("RunFirstPersonCardSourceComponent"));
	if (!RunFirstPersonCardDetailPanelClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C")))
		{
			RunFirstPersonCardDetailPanelClass = Loaded;
		}
		else
		{
			RunFirstPersonCardDetailPanelClass = UWacomCardDetailPanel::StaticClass();
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
			UWacomJourneyDefinition* ResolvedJourney =
				GM->ResolveJourneyDefinitionForNewRun();
			FRunInitializationParams InitializationParams;
			InitializationParams.Character = GM->DefaultCharacter;
			InitializationParams.Journey = ResolvedJourney;
			const FRunInitializationResult Initialization =
				RunSession->Initialize(InitializationParams);
			if (!Initialization.IsOk())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[WacomPlayerController] RunSession 初始化失败：Character=%s Journey=%s Detail=%s"),
					*GetNameSafe(GM->DefaultCharacter),
					*GetNameSafe(ResolvedJourney),
					*Initialization.Status.Detail.ToString());
			}
		}

		// MVVM：把 RunSession 绑到 RunViewModelProvider Subsystem，
		// ViewModel 立刻同步当前 RunState 字段。即便 RunSession 初始化失败也调，
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
		RefreshRunExplorationPresentationBinding();

		if (RunFirstPersonCardSourceComponent)
		{
			PrepareExplorationRunFirstPersonCardLayer();
		}

		StartRunWorldTargetProbePreviewLoop();
	}
	else
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomPlayerController] 非探索 GameMode，跳过 IMC_Exploration / RunSession 初始化"));
	}
}

void AWacomPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TeardownRunExplorationPresentationBinding();
	ClearRunFirstPersonCardLayer();
	if (RunFirstPersonCardDetailController)
	{
		RunFirstPersonCardDetailController->UnbindCurrentBinding();
		RunFirstPersonCardDetailController->RemovePanelFromViewport();
	}
	else if (RunFirstPersonCardDetailPanel)
	{
		RunFirstPersonCardDetailPanel->RemoveFromParent();
		RunFirstPersonCardDetailPanel = nullptr;
	}
	if (RunFirstPersonCardDragController)
	{
		RunFirstPersonCardDragController->UnbindCurrentBinding();
	}
	ClearRunWorldTargetProbePreview();
	if (RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter->ClearHoverPrompt(TEXT("EndPlay"));
	}
	StopRunWorldTargetProbePreviewLoop();
	Super::EndPlay(EndPlayReason);
}

void AWacomPlayerController::SetPawn(APawn* InPawn)
{
	APawn* PreviousPawn = GetPawn();
	Super::SetPawn(InPawn);
	if (PreviousPawn == InPawn)
	{
		return;
	}

	HideRunFirstPersonCardDetailPanel();
	RefreshRunExplorationPresentationBinding();
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();

	if (!InPawn
		|| !RunFirstPersonCardSourceComponent
		|| !RunFirstPersonCardSourceComponent->IsRunFirstPersonCardLayerActive()
		|| !IsInExplorationFlow())
	{
		return;
	}

	RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
	RunFirstPersonCardSourceComponent->RefreshRunFirstPersonCardLayer();
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
}

void AWacomPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (RunPathBranchSelectionController)
	{
		const AWacomPlayerCharacter* WacomCharacter =
			Cast<AWacomPlayerCharacter>(GetPawn());
		const UWacomRunPathTraversalComponent* Traversal = WacomCharacter
			? WacomCharacter->GetRunPathTraversalComponent()
			: nullptr;
		RunPathBranchSelectionController->SetPresentationEnabled(
			CanRouteRunScenePointerInput()
			&& Traversal
			&& Traversal->GetTraversalState()
				== EWacomRunPathTraversalState::Anchored);
		RunPathBranchSelectionController->TickPointerHover();
	}
	if (RunFirstPersonCardDetailController)
	{
		RunFirstPersonCardDetailController->TickMotion(DeltaTime);
	}
	PumpFirstPersonCardActiveDragPointer();
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
	LazyLoadIA(IA_OpenMap,      TEXT("/Game/Wacom/Input/IA_OpenMap.IA_OpenMap"));
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
	if (IA_OpenMap)      { EIC->BindAction(IA_OpenMap,      ETriggerEvent::Started, this, &AWacomPlayerController::OnOpenMapPressed); }
	if (IA_Interact)     { EIC->BindAction(IA_Interact,     ETriggerEvent::Started, this, &AWacomPlayerController::OnInteractPressed); }
}

bool AWacomPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if ((Params.Key == EKeys::Escape || Params.Key == EKeys::Gamepad_FaceButton_Right)
		&& Params.Event == IE_Pressed)
	{
		if (TryCancelFirstPersonCardKeyboardShortcutDrag())
		{
			return true;
		}
		if (UBattleHUD* HUD = GetActiveBattleHUD())
		{
			if (HUD->IsInTargetSelect())
			{
				HUD->CancelTargetSelect();
				return true;
			}
			if (HUD->TryCloseEnemyInspection())
			{
				return true;
			}
		}
	}
	if (Params.Key == EKeys::RightMouseButton
		&& Params.Event == IE_Pressed
		&& TryCancelFirstPersonCardKeyboardShortcutDrag())
	{
		return true;
	}
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryReleaseFirstPersonCardActiveDragPointer())
	{
		return true;
	}
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryRouteBattleSceneTargetClick(/*bRequireTargetSelect*/true))
	{
		return true;
	}
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryRouteRunPathBranchClick())
	{
		return true;
	}
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryRouteRunWorldInteractableClick())
	{
		return true;
	}

	return Super::InputKey(Params);
}

// ================ 战斗状态切换转发 ================

void AWacomPlayerController::RequestExitBattle(EBattleOutcome Outcome)
{
	if (AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr)
	{
		GM->ExitBattle(Outcome);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WacomPlayerController] RequestExitBattle 时找不到 AWacomGameMode"));
	}
}

void AWacomPlayerController::SetRunFirstPersonCardLayerActive(bool bActive)
{
	if (!bActive)
	{
		HideRunFirstPersonCardDetailPanel();
	}
	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerActive(bActive);
	}
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	if (bActive)
	{
		PrewarmRunFirstPersonCardDetailPanel();
	}
}

bool AWacomPlayerController::RefreshRunFirstPersonCardLayer()
{
	if (!RunFirstPersonCardSourceComponent)
	{
		return false;
	}

	RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
	const bool bRefreshed = RunFirstPersonCardSourceComponent->RefreshRunFirstPersonCardLayer();
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	if (bRefreshed)
	{
		PrewarmRunFirstPersonCardDetailPanel();
	}
	return bRefreshed;
}

void AWacomPlayerController::ClearRunFirstPersonCardLayer()
{
	HideRunFirstPersonCardDetailPanel();
	ClearRunFirstPersonCardDragCameraLookOverride();
	ClearRunMenuDropTargetProbe();
	ClearRunWorldCardDropProbe();
	if (RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter->ClearHoverPrompt(TEXT("FirstPersonLayerCleared"));
	}
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerActive(false);
	}
	ActiveGameMenuWidgets.Reset();
	bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = false;
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
}

void AWacomPlayerController::PrepareExplorationRunFirstPersonCardLayer()
{
	HideRunFirstPersonCardDetailPanel();
	ClearRunMenuDropTargetProbe();
	ClearRunWorldCardDropProbe();
	ActiveGameMenuWidgets.Reset();
	bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = false;

	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
		const bool bWasActive = RunFirstPersonCardSourceComponent->IsRunFirstPersonCardLayerActive();
		RunFirstPersonCardSourceComponent->ResetRunFirstPersonCardLayerMenuContext();
		if (!bWasActive)
		{
			RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerActive(true);
		}
	}

	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	PrewarmRunFirstPersonCardDetailPanel();
}

void AWacomPlayerController::SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed)
{
	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerSuppressedByGameMenu(bSuppressed);
	}
	if (bSuppressed
		&& (!RunFirstPersonCardSourceComponent || !RunFirstPersonCardSourceComponent->HasActiveMenuLease()))
	{
		HideRunFirstPersonCardDetailPanel();
	}
	RefreshRunFirstPersonCardDetailBinding();
}

bool AWacomPlayerController::SetRunFirstPersonCardLayerMenuLeaseFromRunCards(
	const FWacomRunMenuCardLeaseRequest& Request,
	FWacomRunMenuCardLeaseResult& OutResult)
{
	if (!RunFirstPersonCardSourceComponent)
	{
		OutResult = FWacomRunMenuCardLeaseResult();
		OutResult.LeaseId = Request.LeaseId;
		OutResult.SourceId = Request.SourceId;
		OutResult.RejectReason = TEXT("MissingSourceComponent");
		OutResult.DebugSummary = FString::Printf(
			TEXT("RunMenuCardLeaseProvider{LeaseId=%s SourceId=%s LeaseSet=false Reject=MissingSourceComponent}"),
			*Request.LeaseId.ToString(),
			*Request.SourceId.ToString());
		RefreshRunFirstPersonMenuLeaseDragBinding();
		return false;
	}

	const bool bHadRequestedLease =
		RunFirstPersonCardSourceComponent->HasActiveMenuLease()
		&& RunFirstPersonCardSourceComponent->GetActiveMenuLeaseId() == Request.LeaseId;
	RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
	const bool bSet =
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerMenuLeaseFromRunCards(
			Request,
			OutResult);
	if (!bSet
		&& bHadRequestedLease
		&& (!RunFirstPersonCardSourceComponent->HasActiveMenuLease()
			|| RunFirstPersonCardSourceComponent->GetActiveMenuLeaseId() != Request.LeaseId))
	{
		ClearRunMenuDropTargetProbe();
		HideRunFirstPersonCardDetailPanel();
	}
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	return bSet;
}

bool AWacomPlayerController::ClearRunFirstPersonCardLayerMenuLease(FName LeaseId)
{
	const bool bCleared = RunFirstPersonCardSourceComponent
		? RunFirstPersonCardSourceComponent->ClearRunFirstPersonCardLayerMenuLease(LeaseId)
		: false;
	if (bCleared)
	{
		ClearRunMenuDropTargetProbe();
		HideRunFirstPersonCardDetailPanel();
	}
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	return bCleared;
}

URunSession* AWacomPlayerController::ResolveRunSessionForFirstPersonCardSource() const
{
	return RunSession;
}

void AWacomPlayerController::RegisterActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget)
{
	if (!MenuWidget)
	{
		return;
	}

	bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = false;
	CompactActiveGameMenuWidgets();

	if (!ActiveGameMenuWidgets.Contains(MenuWidget))
	{
		ActiveGameMenuWidgets.Add(MenuWidget);
	}
	if (RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter->ClearHoverPrompt(TEXT("GameMenuActive"));
	}
	RefreshRunFirstPersonCardLayerMenuSuppression();
}

void AWacomPlayerController::UnregisterActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget)
{
	const bool bTrackedGameMenuViewpointMenuClosing =
		bGameMenuViewpointReturnArmed
		&& (GameMenuViewpointReturnWidget.Get() == MenuWidget
			|| !GameMenuViewpointReturnWidget.IsValid());

	RemoveActiveGameMenuWidget(MenuWidget);

	if (bTrackedGameMenuViewpointMenuClosing)
	{
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = true;
		GameMenuViewpointReturnWidget.Reset();
	}

	RefreshRunFirstPersonCardLayerMenuSuppression();

	if (!bTrackedGameMenuViewpointMenuClosing || HasAnyActiveGameMenuWidget())
	{
		return;
	}

	bGameMenuViewpointReturnArmed = false;
	BeginGameMenuViewpointStageTransition(FName(TEXT("GameMenuReturn")));

	if (AWacomPlayerCharacter* WacomPawn = GetPawn<AWacomPlayerCharacter>())
	{
		const TWeakObjectPtr<AWacomPlayerController> WeakThis(this);
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*WacomPawn,
			*this,
			[WeakThis]()
			{
				if (AWacomPlayerController* StrongThis = WeakThis.Get())
				{
					StrongThis->FinishGameMenuViewpointStageTransition();
				}
			});
		return;
	}

	FinishGameMenuViewpointStageTransition();
}

void AWacomPlayerController::SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(bool bSuppressed)
{
	if (bRunFirstPersonCardLayerTransitionSuppressedByGameMenu == bSuppressed)
	{
		RefreshRunFirstPersonCardLayerMenuSuppression();
		return;
	}

	bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = bSuppressed;
	if (bSuppressed && RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter->ClearHoverPrompt(TEXT("GameMenuTransition"));
	}
	RefreshRunFirstPersonCardLayerMenuSuppression();
}

void AWacomPlayerController::RegisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget)
{
	GetRunFirstPersonCardDropCoordinator().RegisterRunMenuDropTarget(DropTarget);
}

void AWacomPlayerController::UnregisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget)
{
	GetRunFirstPersonCardDropCoordinator().UnregisterRunMenuDropTarget(DropTarget);
}

bool AWacomPlayerController::TryProbeRunMenuDropTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	return GetRunFirstPersonCardDropCoordinator()
		.TryProbeRunMenuDropTargetAtWidgetPosition(WidgetPosition, OutHandle);
}

void AWacomPlayerController::RefreshRunFirstPersonCardLayerMenuSuppression()
{
	CompactActiveGameMenuWidgets();

	const bool bShouldSuppress = HasActiveRunGameMenuOrTransitionSuppression();
	SetRunFirstPersonCardLayerSuppressedByGameMenu(bShouldSuppress);
	if (!bShouldSuppress)
	{
		ClearRunMenuDropTargetProbe();
	}
	else if (!RunFirstPersonCardSourceComponent || !RunFirstPersonCardSourceComponent->HasActiveMenuLease())
	{
		HideRunFirstPersonCardDetailPanel();
	}
	RefreshRunFirstPersonCardDetailBinding();
	RefreshRunFirstPersonMenuLeaseDragBinding();
}

void AWacomPlayerController::CompactActiveGameMenuWidgets()
{
	ActiveGameMenuWidgets.RemoveAll(
		[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Existing)
		{
			return !Existing.IsValid();
		});
}

void AWacomPlayerController::RemoveActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget)
{
	ActiveGameMenuWidgets.RemoveAll(
		[MenuWidget](const TWeakObjectPtr<UWacomMenuWidgetBase>& Existing)
		{
			return !Existing.IsValid() || Existing.Get() == MenuWidget;
		});
}

bool AWacomPlayerController::HasAnyActiveGameMenuWidget() const
{
	return ActiveGameMenuWidgets.ContainsByPredicate(
		[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Menu)
		{
			return Menu.IsValid();
		});
}

bool AWacomPlayerController::HasActiveRunGameMenuOrTransitionSuppression() const
{
	return bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| HasAnyActiveGameMenuWidget();
}

FWacomRunFirstPersonCardDetailController&
AWacomPlayerController::GetRunFirstPersonCardDetailController()
{
	if (!RunFirstPersonCardDetailController)
	{
		RunFirstPersonCardDetailController =
			MakeShared<FWacomRunFirstPersonCardDetailController>(*this);
	}
	return *RunFirstPersonCardDetailController;
}

const FWacomRunFirstPersonCardDetailController&
AWacomPlayerController::GetRunFirstPersonCardDetailController() const
{
	return const_cast<AWacomPlayerController*>(this)->GetRunFirstPersonCardDetailController();
}

FWacomRunFirstPersonCardDragController&
AWacomPlayerController::GetRunFirstPersonCardDragController()
{
	if (!RunFirstPersonCardDragController)
	{
		RunFirstPersonCardDragController =
			MakeShared<FWacomRunFirstPersonCardDragController>(*this);
	}
	return *RunFirstPersonCardDragController;
}

const FWacomRunFirstPersonCardDragController&
AWacomPlayerController::GetRunFirstPersonCardDragController() const
{
	return const_cast<AWacomPlayerController*>(this)->GetRunFirstPersonCardDragController();
}

FWacomRunFirstPersonCardDropCoordinator&
AWacomPlayerController::GetRunFirstPersonCardDropCoordinator()
{
	if (!RunFirstPersonCardDropCoordinator)
	{
		FWacomRunFirstPersonCardDropCoordinator::FContext DropContext;
		DropContext.PlayerController = this;
		DropContext.IsInExplorationFlowFunc =
			[this]()
			{
				return IsInExplorationFlow();
			};
		DropContext.HasActiveRunGameMenuOrTransitionSuppressionFunc =
			[this]()
			{
				return HasActiveRunGameMenuOrTransitionSuppression();
			};
		DropContext.IsRunWorldCardDropEnabledFunc =
			[this]()
			{
				return bEnableRunWorldCardDrop;
			};
		DropContext.ShouldLogRunWorldCardDropFunc =
			[this]()
			{
				return bLogRunWorldCardDrop;
			};
		DropContext.ResolveRunFirstPersonCardSourceFunc =
			[this]()
			{
				return RunFirstPersonCardSourceComponent;
			};
		DropContext.ResolveFirstPersonCardAnchorFunc =
			[this]()
			{
				return ResolveFirstPersonCardAnchorForRunMenuProbe();
			};
		DropContext.ResolveRunSessionFunc =
			[this]()
			{
				return ResolveRunSessionForFirstPersonCardSource();
			};
		DropContext.ResolveOwningMenuForLeaseFunc =
			[this](FName LeaseId)
			{
				return ResolveOwningMenuForActiveRunMenuLease(LeaseId);
			};
		DropContext.ResolveAppToastSubsystemFunc =
			[this]()
			{
				return ResolveAppToastSubsystem();
			};
		DropContext.RefreshRunFirstPersonCardLayerFunc =
			[this]()
			{
				RefreshRunFirstPersonCardLayer();
			};
		DropContext.TryProbeRunSceneInteractionTargetAtWidgetPositionFunc =
			[this](
				const FVector2D& WidgetPosition,
				FWacomInteractionTargetHandle& OutHandle)
			{
				return TryProbeRunSceneInteractionTargetAtWidgetPosition(
					WidgetPosition,
					OutHandle);
			};
		DropContext.ResolveRunWorldClickableInteractableFromHandleFunc =
			[this](
				const FWacomInteractionTargetHandle& Handle,
				AActor*& OutInteractableActor,
				UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
				FName& OutRejectReason)
			{
				return ResolveRunWorldClickableInteractableFromHandle(
					Handle,
					OutInteractableActor,
					OutBridge,
					OutRejectReason);
			};
		DropContext.ResolveRunWorldCardDropReceiverFromHandleFunc =
			[this](const FWacomInteractionTargetHandle& Handle)
			{
				return ResolveRunWorldCardDropReceiverFromHandle(Handle);
			};
		RunFirstPersonCardDropCoordinator =
			MakeShared<FWacomRunFirstPersonCardDropCoordinator>(MoveTemp(DropContext));
	}
	return *RunFirstPersonCardDropCoordinator;
}

const FWacomRunFirstPersonCardDropCoordinator&
AWacomPlayerController::GetRunFirstPersonCardDropCoordinator() const
{
	return const_cast<AWacomPlayerController*>(this)->GetRunFirstPersonCardDropCoordinator();
}

#if WITH_AUTOMATION_TESTS
FString AWacomPlayerController::GetRunMenuDropProbeDebugSummaryForTest() const
{
	return GetRunFirstPersonCardDropCoordinator().GetRunMenuDropProbeDebugSummary();
}

FString AWacomPlayerController::GetRunWorldCardDropDebugSummaryForTest() const
{
	return GetRunFirstPersonCardDropCoordinator().GetRunWorldCardDropDebugSummary();
}

bool AWacomPlayerController::ApplyRunMenuDropProbeFeedbackForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	return GetRunFirstPersonCardDropCoordinator()
		.ApplyRunMenuDropProbeFeedbackForTest(CardInstanceId, DragView, bReleased);
}

bool AWacomPlayerController::ApplyRunWorldCardDropProbeFeedbackForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	return GetRunFirstPersonCardDropCoordinator()
		.ApplyRunWorldCardDropProbeFeedbackForTest(CardInstanceId, DragView, bReleased);
}

FWacomRunMenuCardDropResolveResult
AWacomPlayerController::ResolveRunMenuCardDropIntentForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	return GetRunFirstPersonCardDropCoordinator()
		.ResolveRunMenuCardDropIntentForTest(CardInstanceId, DragView);
}

FRunWorldCardInteractionValidation
AWacomPlayerController::ResolveRunWorldCardDropIntentForTest(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	AActor*& OutTargetActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
	UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
	FString& OutDebugSummary) const
{
	return GetRunFirstPersonCardDropCoordinator()
		.ResolveRunWorldCardDropIntentForTest(
			CardInstanceId,
			DragView,
			OutTargetHandle,
			OutTargetActor,
			OutTargetBridge,
			OutReceiver,
			OutDebugSummary);
}
#endif

bool AWacomPlayerController::BuildRunFirstPersonCardDetailViewData(
	const FGuid& CardInstanceId,
	FWacomCardDetailViewData& OutDetailData) const
{
	URunSession* Run = ResolveRunSessionForFirstPersonCardSource();
	if (!Run || !CardInstanceId.IsValid())
	{
		return false;
	}

	FCardInstance Instance;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid ZoneOwnerInstanceId;
	if (!Run->FindInstance(CardInstanceId, Instance, Zone, ZoneOwnerInstanceId)
		|| !Instance.Definition)
	{
		return false;
	}

	OutDetailData = UWacomCardPresentationBuilder::BuildCardDetailViewData(Instance.Definition);
	return true;
}

void AWacomPlayerController::PrewarmRunFirstPersonCardDetailPanel()
{
	if (IsInExplorationFlow())
	{
		GetRunFirstPersonCardDetailController().PrewarmPanel();
	}
}

void AWacomPlayerController::RefreshRunFirstPersonCardDetailBinding()
{
	GetRunFirstPersonCardDetailController().RefreshBinding();
}

void AWacomPlayerController::HideRunFirstPersonCardDetailPanel()
{
	if (RunFirstPersonCardDetailController)
	{
		RunFirstPersonCardDetailController->ForceHideAll();
	}
	else if (RunFirstPersonCardDetailPanel)
	{
		RunFirstPersonCardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
		RunFirstPersonCardDetailPanel->SetRenderOpacity(0.0f);
		RunFirstPersonCardDetailPanel->SetRenderTransform(FWidgetTransform());
	}
}

#if WITH_AUTOMATION_TESTS
bool AWacomPlayerController::IsRunFirstPersonCardDetailPanelVisibleForTest() const
{
	return RunFirstPersonCardDetailController
		&& RunFirstPersonCardDetailController->IsVisible();
}

FText AWacomPlayerController::GetRunFirstPersonCardDetailPanelNameTextForTest() const
{
	return RunFirstPersonCardDetailController
		? RunFirstPersonCardDetailController->GetNameText()
		: FText::GetEmpty();
}

FVector2D AWacomPlayerController::GetRunFirstPersonCardDetailPanelPositionForTest() const
{
	return RunFirstPersonCardDetailController
		? RunFirstPersonCardDetailController->GetLastPanelPosition()
		: FVector2D::ZeroVector;
}

bool AWacomPlayerController::IsRunFirstPersonCardDetailPanelPrewarmedForTest() const
{
	return RunFirstPersonCardDetailController
		&& RunFirstPersonCardDetailController->IsPrewarmed();
}

bool AWacomPlayerController::IsRunFirstPersonCardDetailMotionPendingForTest() const
{
	return RunFirstPersonCardDetailController
		&& RunFirstPersonCardDetailController->IsPendingShowForTest();
}

float AWacomPlayerController::GetRunFirstPersonCardDetailPanelOpacityForTest() const
{
	return RunFirstPersonCardDetailController
		? RunFirstPersonCardDetailController->GetPanelOpacityForTest()
		: 0.0f;
}

int32 AWacomPlayerController::GetRunFirstPersonCardDetailDataApplyCountForTest() const
{
	return RunFirstPersonCardDetailController
		? RunFirstPersonCardDetailController->GetDetailDataApplyCountForTest()
		: 0;
}

void AWacomPlayerController::TickRunFirstPersonCardDetailForTest(float DeltaTime)
{
	if (RunFirstPersonCardDetailController)
	{
		RunFirstPersonCardDetailController->TickMotion(DeltaTime);
	}
}
#endif

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

FWacomBattleSceneInteractionRouter& AWacomPlayerController::GetBattleSceneInteractionRouter()
{
	if (!BattleSceneInteractionRouter)
	{
		BattleSceneInteractionRouter =
			MakeShared<FWacomBattleSceneInteractionRouter>(*this);
	}
	return *BattleSceneInteractionRouter;
}

const FWacomBattleSceneInteractionRouter& AWacomPlayerController::GetBattleSceneInteractionRouter() const
{
	return const_cast<AWacomPlayerController*>(this)->GetBattleSceneInteractionRouter();
}

FWacomRunWorldInteractionRouter& AWacomPlayerController::GetRunWorldInteractionRouter()
{
	if (!RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter =
			MakeShared<FWacomRunWorldInteractionRouter>(*this);
	}
	return *RunWorldInteractionRouter;
}

const FWacomRunWorldInteractionRouter& AWacomPlayerController::GetRunWorldInteractionRouter() const
{
	return const_cast<AWacomPlayerController*>(this)->GetRunWorldInteractionRouter();
}

bool AWacomPlayerController::TryRouteBattleSceneTargetClick(bool bRequireTargetSelect)
{
	return GetBattleSceneInteractionRouter().TryRouteTargetClick(bRequireTargetSelect);
}

bool AWacomPlayerController::TryProbeBattleSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const
{
	return GetBattleSceneInteractionRouter().TryProbeInteractionTarget(OutHandle);
}

bool AWacomPlayerController::TryProbeBattleSceneInteractionTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	return GetBattleSceneInteractionRouter().TryProbeInteractionTargetAtWidgetPosition(
		WidgetPosition,
		OutHandle);
}

bool AWacomPlayerController::TryProbeRunSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const
{
	return GetRunWorldInteractionRouter().TryProbeSceneInteractionTarget(OutHandle);
}

bool AWacomPlayerController::TryProbeRunSceneInteractionTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	return GetRunWorldInteractionRouter().TryProbeSceneInteractionTargetAtWidgetPosition(
		WidgetPosition,
		OutHandle);
}

bool AWacomPlayerController::TryRouteRunWorldInteractableClick()
{
	return CanRouteRunScenePointerInput()
		&& GetRunWorldInteractionRouter().TryRouteInteractableClick();
}

FString AWacomPlayerController::GetRunWorldInteractableHoverDebugSummary() const
{
	return GetRunWorldInteractionRouter().BuildHoverDebugSummary();
}

void AWacomPlayerController::LogRunWorldInteractableHoverDebugSummary() const
{
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunWorldInteractableHover] %s"),
		*GetRunWorldInteractableHoverDebugSummary());
}

bool AWacomPlayerController::BuildBattleSceneInteractionTargetHitResultAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FHitResult& OutHitResult) const
{
	const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	const FVector2D PixelPosition = WidgetPosition * ViewportScale;
	return BuildBattleSceneInteractionTargetHitResultAtScreenPosition(PixelPosition, OutHitResult);
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
	float PixelX = 0.0f;
	float PixelY = 0.0f;
	return GetMousePosition(PixelX, PixelY)
		&& BuildBattleSceneInteractionTargetHitResultAtScreenPosition(
			FVector2D(PixelX, PixelY),
			OutHitResult);
}

bool AWacomPlayerController::BuildBattleSceneInteractionTargetHitResultAtScreenPosition(
	const FVector2D& PixelPosition,
	FHitResult& OutHitResult) const
{
	OutHitResult = FHitResult();
	UWorld* World = GetWorld();
	UBattleHUD* HUD = GetActiveBattleHUD();
	if (!World || !HUD)
	{
		return false;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!DeprojectScreenPositionToWorld(
		PixelPosition.X,
		PixelPosition.Y,
		WorldOrigin,
		WorldDirection))
	{
		return false;
	}
	WorldDirection = WorldDirection.GetSafeNormal();
	if (WorldDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.0f;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WacomBattleEnemyPartInteraction), false);
	if (const APawn* ControlledPawn = GetPawn())
	{
		QueryParams.AddIgnoredActor(ControlledPawn);
	}

	auto ResolveCurrentRegistryPart = [HUD](
		const FHitResult& Hit,
		FWacomInteractionTargetHandle& OutHandle)
	{
		OutHandle = WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(Hit);
		return OutHandle.IsValid()
			&& OutHandle.TargetKind == EWacomInteractionTargetKind::World
			&& OutHandle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
			&& OutHandle.HasBattlePartSlotIdentity()
			&& HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(OutHandle);
	};

	FHitResult VisibilityOccluderHit;
	const bool bHasVisibilityOccluder = World->LineTraceSingleByChannel(
		VisibilityOccluderHit,
		WorldOrigin,
		TraceEnd,
		ECC_Visibility,
		QueryParams);
	const float OccluderDepth = bHasVisibilityOccluder
		? FVector::DotProduct(
			VisibilityOccluderHit.ImpactPoint - WorldOrigin,
			WorldDirection)
		: TNumericLimits<float>::Max();

	FHitResult StrictHit;
	const bool bHasStrictBlockingHit = World->LineTraceSingleByChannel(
		StrictHit,
		WorldOrigin,
		TraceEnd,
		Wacom::Interaction::BattleEnemyPartTraceChannel,
		QueryParams);
	FWacomInteractionTargetHandle StrictHandle;
	const float StrictDepth = bHasStrictBlockingHit
		? FVector::DotProduct(StrictHit.ImpactPoint - WorldOrigin, WorldDirection)
		: TNumericLimits<float>::Max();
	if (bHasStrictBlockingHit
		&& StrictDepth <= OccluderDepth + UE_KINDA_SMALL_NUMBER
		&& ResolveCurrentRegistryPart(StrictHit, StrictHandle))
	{
		OutHitResult = StrictHit;
		return true;
	}

	const float SweepRadius = FMath::Max(
		0.0f,
		BattleEnemyPartLenientSweepRadiusCentimeters);
	if (SweepRadius <= UE_SMALL_NUMBER)
	{
		return false;
	}
	TArray<FHitResult> SweepHits;
	FCollisionResponseParams LenientResponseParams;
	LenientResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	LenientResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Overlap);
	World->SweepMultiByChannel(
		SweepHits,
		WorldOrigin,
		TraceEnd,
		FQuat::Identity,
		Wacom::Interaction::BattleEnemyPartTraceChannel,
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams,
		LenientResponseParams);

	TMap<FString, FWacomBattleEnemyPartInteractionQueryCandidate> Candidates;
	for (const FHitResult& Hit : SweepHits)
	{
		FWacomInteractionTargetHandle Handle;
		if (!ResolveCurrentRegistryPart(Hit, Handle))
		{
			continue;
		}
		const float TraceDepth = FVector::DotProduct(
			Hit.ImpactPoint - WorldOrigin,
			WorldDirection);
		FVector2D HitScreenPosition = FVector2D::ZeroVector;
		if (!ProjectWorldLocationToScreen(Hit.ImpactPoint, HitScreenPosition))
		{
			continue;
		}
		const FString StableIdentity = FString::Printf(
			TEXT("%s|%s|%s|%s"),
			*Handle.EncounterId.ToString(),
			*Handle.EnemySlotId.ToString(),
			*Handle.PartSlotId.ToString(),
			*Handle.WorldTargetId.ToString());
		FWacomBattleEnemyPartInteractionQueryCandidate Candidate;
		Candidate.Hit = Hit;
		Candidate.StableIdentity = StableIdentity;
		Candidate.ScreenDistanceSquared = FVector2D::DistSquared(
			PixelPosition,
			HitScreenPosition);
		Candidate.TraceDepth = TraceDepth;
		Candidate.bInCurrentRegistry = true;
		if (!FWacomBattleEnemyPartInteractionQueryPolicy::IsEligible(
			Candidate,
			OccluderDepth))
		{
			continue;
		}
		FWacomBattleEnemyPartInteractionQueryCandidate* Existing =
			Candidates.Find(StableIdentity);
		if (!Existing
			|| FWacomBattleEnemyPartInteractionQueryPolicy::IsPreferred(
				Candidate,
				*Existing))
		{
			Candidates.Add(StableIdentity, MoveTemp(Candidate));
		}
	}

	TArray<FWacomBattleEnemyPartInteractionQueryCandidate> Ordered;
	Candidates.GenerateValueArray(Ordered);
	Ordered.Sort([](
		const FWacomBattleEnemyPartInteractionQueryCandidate& Left,
		const FWacomBattleEnemyPartInteractionQueryCandidate& Right)
	{
		return FWacomBattleEnemyPartInteractionQueryPolicy::IsPreferred(Left, Right);
	});
	if (Ordered.IsEmpty())
	{
		return false;
	}
	OutHitResult = Ordered[0].Hit;
	return true;
}

bool AWacomPlayerController::BuildRunSceneClickHitResult(FHitResult& OutHitResult) const
{
	return GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult);
}

bool AWacomPlayerController::BuildRunSceneInteractionTargetHitResultAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FHitResult& OutHitResult) const
{
	const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	const FVector2D PixelPosition = WidgetPosition * ViewportScale;
	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!DeprojectScreenPositionToWorld(PixelPosition.X, PixelPosition.Y, WorldOrigin, WorldDirection))
	{
		return false;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.0f;
	return GetWorld() && GetWorld()->LineTraceSingleByChannel(OutHitResult, WorldOrigin, TraceEnd, ECC_Visibility);
}

bool AWacomPlayerController::TryRouteRunPathBranchClick()
{
	if (!CanRouteRunScenePointerInput()
		|| !RunPathBranchSelectionController
		|| !RunPathBranchSelectionController->IsChoiceRequired())
	{
		return false;
	}

	FHitResult HitResult;
	if (!BuildRunPathBranchClickHitResult(HitResult))
	{
		RunPathBranchSelectionController->PulseAvailable();
		return true;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!Cast<AWacomRunPathBranchTargetActor>(HitActor)
		&& HitResult.GetComponent())
	{
		HitActor = HitResult.GetComponent()->GetOwner();
	}
	RunPathBranchSelectionController->TrySelectHitActor(HitActor);
	// ChoiceRequired 下场景点击始终由道路选择消费，避免穿透到节点内容 Actor。
	return true;
}

bool AWacomPlayerController::ApplyRunNodeActivityResolutionForPresentation(
	const FRunExplorationResolution& Resolution)
{
	if (RunExplorationPresentationCoordinator
		&& RunExplorationPresentationCoordinator->ApplyNodeActivityResolution(Resolution))
	{
		return true;
	}

	const FName FailureDetail = RunExplorationPresentationCoordinator
		? RunExplorationPresentationCoordinator->GetLastErrorDetail()
		: FName(TEXT("RunExplorationPresentationCoordinatorMissing"));
	UE_LOG(LogTemp, Error,
		TEXT("[WacomPlayerController] Run 节点活动结果表现应用失败，重建当前 Session 绑定。Detail=%s Before=%d After=%d"),
		*FailureDetail.ToString(),
		Resolution.VersionBefore,
		Resolution.VersionAfter);

	const bool bRecovered = RefreshRunExplorationPresentationBinding();
	UE_CLOG(!bRecovered, LogTemp, Error,
		TEXT("[WacomPlayerController] Run 探索表现绑定恢复失败"));
	return false;
}

bool AWacomPlayerController::RefreshRunExplorationPresentationBinding()
{
	const auto RejectRefresh = [this](const FName Detail)
	{
		RunExplorationSceneBindingLastFailureDetail = Detail;
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[WacomPlayerController] Run 探索场景绑定原子拒绝：%s"),
			*Detail.ToString());
		return false;
	};

#if WITH_DEV_AUTOMATION_TESTS
	const FName PreCommitFault =
		RunExplorationSceneBindingPreCommitFaultForAutomation;
	RunExplorationSceneBindingPreCommitFaultForAutomation = NAME_None;
#endif

	AWacomPlayerCharacter* WacomCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	UWacomRunPathTraversalComponent* Traversal = WacomCharacter
		? WacomCharacter->GetRunPathTraversalComponent()
		: nullptr;
	if (!RunSession || !Traversal || !GetWorld())
	{
		return RejectRefresh(TEXT("SceneBindingPrerequisiteMissing"));
	}
	if (RunExplorationPresentationCoordinator
		&& RunExplorationPresentationCoordinator->HasActiveTraversal())
	{
		return RejectRefresh(TEXT("SceneBindingRefreshDuringTraversal"));
	}

	const FRunExplorationSnapshot Snapshot = RunSession->BuildExplorationSnapshot();
	if (Snapshot.StateVersion <= 0 || !Snapshot.CurrentNode.IsValid())
	{
		return RejectRefresh(TEXT("SceneBindingSnapshotInvalid"));
	}

	const FWacomRunFloorSceneDescriptorResolveResult DescriptorResult =
		FWacomRunFloorSceneDescriptorResolver::Resolve(
			GetWorld(),
			Snapshot.CurrentNode.FloorId);
	if (!DescriptorResult.IsResolved() || !DescriptorResult.FloorDefinition)
	{
		return RejectRefresh(DescriptorResult.GetDetail());
	}

	TSharedPtr<FWacomRunSceneBindingRegistry> WorkingRegistry =
		MakeShared<FWacomRunSceneBindingRegistry>();
	WorkingRegistry->Reset(Snapshot.CurrentNode.FloorId);
	for (TActorIterator<AWacomRunPathSegmentActor> It(GetWorld()); It; ++It)
	{
		if (!WorkingRegistry->RegisterPath(**It))
		{
			return RejectRefresh(TEXT("ScenePathRegistrationFailed"));
		}
	}
	for (TActorIterator<AWacomRunMapNodeAnchorActor> It(GetWorld()); It; ++It)
	{
		if (!WorkingRegistry->RegisterNodeAnchor(**It))
		{
			return RejectRefresh(TEXT("SceneNodeAnchorRegistrationFailed"));
		}
		TInlineComponentArray<UWacomRunEncounterSceneBindingComponent*> EncounterBindings;
		It->GetComponents(EncounterBindings);
		for (UWacomRunEncounterSceneBindingComponent* Binding : EncounterBindings)
		{
			if (!Binding || !WorkingRegistry->RegisterEncounterBinding(*Binding))
			{
				return RejectRefresh(TEXT("SceneEncounterBindingRegistrationFailed"));
			}
		}
	}
	for (TActorIterator<AWacomRunPathBranchTargetActor> It(GetWorld()); It; ++It)
	{
		if (!WorkingRegistry->RegisterBranchTarget(**It))
		{
			return RejectRefresh(TEXT("SceneBranchTargetRegistrationFailed"));
		}
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UWacomRunMapNodeBindingComponent*> Bindings;
		It->GetComponents(Bindings);
		for (UWacomRunMapNodeBindingComponent* Binding : Bindings)
		{
			if (Binding && !WorkingRegistry->RegisterContentHost(
				Binding->NodeId,
				Binding->NodeType,
				**It))
			{
				return RejectRefresh(TEXT("SceneContentHostRegistrationFailed"));
			}
		}
	}

	const FWacomStatus Completeness = WorkingRegistry->ValidateComplete(
		*DescriptorResult.FloorDefinition);
	if (!Completeness.IsOk())
	{
		return RejectRefresh(Completeness.Detail);
	}

	FRunExplorationSnapshot CommitSnapshot =
		RunSession->BuildExplorationSnapshot();
#if WITH_DEV_AUTOMATION_TESTS
	if (PreCommitFault == TEXT("VersionDrift"))
	{
		++CommitSnapshot.StateVersion;
	}
	else if (PreCommitFault == TEXT("FloorDrift"))
	{
		CommitSnapshot.CurrentNode.FloorId = TEXT("Floor.AutomationDrift");
	}
#endif
	if (CommitSnapshot.StateVersion != Snapshot.StateVersion)
	{
		return RejectRefresh(TEXT("SceneBindingSnapshotVersionDrift"));
	}
	if (CommitSnapshot.CurrentNode.FloorId != Snapshot.CurrentNode.FloorId
		|| CommitSnapshot.CurrentNode.NodeId != Snapshot.CurrentNode.NodeId)
	{
		return RejectRefresh(TEXT("SceneBindingSnapshotFloorDrift"));
	}

	TSharedPtr<FWacomRunExplorationPresentationCoordinator> WorkingCoordinator =
		MakeShared<FWacomRunExplorationPresentationCoordinator>();
	if (!WorkingCoordinator->PrepareFromValidatedSnapshot(
		*RunSession,
		*Traversal,
		*WorkingRegistry,
		CommitSnapshot))
	{
		return RejectRefresh(WorkingCoordinator->GetLastErrorDetail());
	}

	const TArray<AWacomRunPathBranchTargetActor*> WorkingBranchTargets =
		WorkingRegistry->GetBranchTargets();
	TeardownRunExplorationPresentationBinding();
	RunExplorationSceneBindingRegistry = MoveTemp(WorkingRegistry);
	RunExplorationPresentationCoordinator = MoveTemp(WorkingCoordinator);
	RunExplorationPresentationCoordinator->CommitPreparedInitialization();

	for (AWacomRunPathBranchTargetActor* Target : WorkingBranchTargets)
	{
		Target->OnBranchRequestedNative().AddUObject(
			this,
			&AWacomPlayerController::HandleRunPathBranchRequested);
		BoundRunPathBranchTargets.Add(Target);
	}
	BoundRunPathTraversal = Traversal;
	Traversal->OnAnchoredForwardIntentNative().AddUObject(
		this,
		&AWacomPlayerController::HandleRunPathAnchoredForwardIntent);
	Traversal->OnAnchoredHorizontalIntentNative().AddUObject(
		this,
		&AWacomPlayerController::HandleRunPathAnchoredHorizontalIntent);
	RunExplorationPresentationCoordinator->OnRouteChoiceStateChangedNative().AddUObject(
		this,
		&AWacomPlayerController::HandleRunRouteChoiceStateChanged);
	RunExplorationPresentationCoordinator->OnNodeContentPresentationRequestedNative().AddUObject(
		this,
		&AWacomPlayerController::HandleRunNodeContentArrival);

	RunPathBranchSelectionController =
		MakeShared<FWacomRunPathBranchSelectionController>();
	RunPathBranchSelectionController->Initialize(*this, BoundRunPathBranchTargets);
	RunPathBranchSelectionController->ApplyRouteChoiceState(
		RunExplorationPresentationCoordinator->GetRouteChoiceState());
	RunPathBranchSelectionController->SetPresentationEnabled(
		CanRouteRunScenePointerInput());
	RunMapScreenFlow = MakeShared<FWacomRunMapScreenFlow>();
	RunMapScreenFlow->Initialize(*this, *RunExplorationPresentationCoordinator);
	++RunExplorationSceneBindingGeneration;
	RunExplorationSceneBindingLastFailureDetail = NAME_None;
	RefreshInteractToast();
	return true;
}

void AWacomPlayerController::TeardownRunExplorationPresentationBinding()
{
	if (RunMapScreenFlow)
	{
		RunMapScreenFlow->Shutdown();
		RunMapScreenFlow.Reset();
	}
	if (RunPathBranchSelectionController)
	{
		RunPathBranchSelectionController->Shutdown();
		RunPathBranchSelectionController.Reset();
	}
	UWacomRunPathTraversalComponent* const BoundTraversal =
		BoundRunPathTraversal.Get();
	if (BoundTraversal)
	{
		BoundTraversal->OnAnchoredForwardIntentNative().RemoveAll(this);
		BoundTraversal->OnAnchoredHorizontalIntentNative().RemoveAll(this);
	}
	for (const TWeakObjectPtr<AWacomRunPathBranchTargetActor>& Target : BoundRunPathBranchTargets)
	{
		if (AWacomRunPathBranchTargetActor* StrongTarget = Target.Get())
		{
			StrongTarget->OnBranchRequestedNative().RemoveAll(this);
		}
	}
	BoundRunPathBranchTargets.Reset();
	if (RunExplorationPresentationCoordinator)
	{
		RunExplorationPresentationCoordinator->OnRouteChoiceStateChangedNative().RemoveAll(this);
		RunExplorationPresentationCoordinator->OnNodeContentPresentationRequestedNative().RemoveAll(this);
		RunExplorationPresentationCoordinator->Shutdown();
		RunExplorationPresentationCoordinator.Reset();
	}
	if (BoundTraversal)
	{
		BoundTraversal->DeactivateTraversal();
	}
	BoundRunPathTraversal.Reset();
	RunExplorationSceneBindingRegistry.Reset();
}

bool AWacomPlayerController::CanPresentRunMapScreen(
	bool& bOutPreferRecommendedTarget,
	FName* OutRejectDetail) const
{
	const UWacomRunPathTraversalComponent* Traversal = BoundRunPathTraversal.Get();
	const FRunFloorMapSnapshot Snapshot = RunSession
		? RunSession->BuildCurrentFloorMapSnapshot()
		: FRunFloorMapSnapshot();

	FWacomRunMapOpenGuardFacts Facts;
	Facts.bExplorationFlow = IsInExplorationFlow();
	Facts.bHasSession = RunSession != nullptr;
	Facts.bHasCoordinator = RunExplorationPresentationCoordinator.IsValid();
	Facts.bHasFlow = RunMapScreenFlow.IsValid();
	Facts.bHasTraversal = Traversal != nullptr;
	Facts.bTraversalAnchored = Traversal
		&& Traversal->GetTraversalState() == EWacomRunPathTraversalState::Anchored;
	Facts.bCoordinatorTraversalActive = RunExplorationPresentationCoordinator
		&& RunExplorationPresentationCoordinator->HasActiveTraversal();
	Facts.bSnapshotValid = Snapshot.IsValid();
	Facts.bActiveActivity =
		Snapshot.ActiveActivityKind != ERunExplorationActivityKind::None;
	Facts.bVersionsMatch = RunExplorationPresentationCoordinator
		&& Snapshot.StateVersion
			== RunExplorationPresentationCoordinator->GetLastAppliedVersion();
	Facts.bDeadEnd = RunExplorationPresentationCoordinator
		&& RunExplorationPresentationCoordinator->GetRouteChoiceState().Mode
			== EWacomRunRouteChoiceMode::DeadEnd;

	const FWacomRunMapOpenGuardDecision Decision =
		FWacomRunMapOpenGuard::Evaluate(Facts);
	bOutPreferRecommendedTarget = Decision.bPreferRecommendedTarget;
	if (OutRejectDetail)
	{
		*OutRejectDetail = Decision.RejectDetail;
	}
	return Decision.bCanOpen;
}

int32 AWacomPlayerController::BeginRunMapScreenOpenRequest()
{
	bool bPreferRecommended = false;
	if (!CanPresentRunMapScreen(bPreferRecommended)
		|| RunMapScreenFlow->IsActive()
		|| RunMapScreenFlow->IsOpening())
	{
		return 0;
	}
	return RunMapScreenFlow->BeginOpenRequest();
}

bool AWacomPlayerController::IsRunMapScreenOpenRequestCurrent(
	const int32 RequestGeneration) const
{
	bool bPreferRecommended = false;
	return RunMapScreenFlow
		&& RunMapScreenFlow->IsOpenRequestCurrent(RequestGeneration)
		&& CanPresentRunMapScreen(bPreferRecommended);
}

bool AWacomPlayerController::AttachRunMapScreen(
	UWacomRunMapScreen& Screen,
	const int32 RequestGeneration)
{
	bool bPreferRecommended = false;
	if (!RunMapScreenFlow
		|| !RunSession
		|| !RunMapScreenFlow->IsOpenRequestCurrent(RequestGeneration)
		|| !CanPresentRunMapScreen(bPreferRecommended))
	{
		return false;
	}
	return RunMapScreenFlow->AttachScreen(
		*RunSession,
		Screen,
		bPreferRecommended,
		RequestGeneration);
}

void AWacomPlayerController::CancelRunMapScreenOpenRequest(
	const int32 RequestGeneration)
{
	if (RunMapScreenFlow)
	{
		RunMapScreenFlow->CancelOpenRequest(RequestGeneration);
	}
}

void AWacomPlayerController::HandleRunPathBranchRequested(const FName EdgeId)
{
	if (RunExplorationPresentationCoordinator
		&& RunExplorationPresentationCoordinator->HandleBranchIntent(EdgeId))
	{
		ClearCurrentEncounterRetry();
		return;
	}
	else
	{
		const FName Detail = RunExplorationPresentationCoordinator
			? RunExplorationPresentationCoordinator->GetLastErrorDetail()
			: FName(TEXT("CoordinatorUnavailable"));
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] Run Path 分支请求被拒绝：EdgeId=%s Detail=%s"),
			*EdgeId.ToString(), *Detail.ToString());
	}
}

void AWacomPlayerController::HandleRunPathAnchoredForwardIntent()
{
	if (!RunExplorationPresentationCoordinator || !CanRouteRunScenePointerInput())
	{
		return;
	}

	switch (RunExplorationPresentationCoordinator->HandleForwardIntent())
	{
	case EWacomRunForwardIntentResult::Started:
		ClearCurrentEncounterRetry();
		break;
	case EWacomRunForwardIntentResult::ChoiceRequired:
		if (RunPathBranchSelectionController)
		{
			RunPathBranchSelectionController->PulseAvailable();
		}
		break;
	case EWacomRunForwardIntentResult::DeadEnd:
		if (UWacomAppToastSubsystem* Toast = ResolveAppToastSubsystem())
		{
			Toast->ShowWarning(
				LOCTEXT("RunPathDeadEnd", "此路不通，按 M / 手柄 View 打开地图"));
		}
		break;
	case EWacomRunForwardIntentResult::Unavailable:
		if (UWacomAppToastSubsystem* Toast = ResolveAppToastSubsystem())
		{
			Toast->ShowWarning(
				LOCTEXT("RunPathUnavailable", "当前道路尚不可通行"));
		}
		break;
	case EWacomRunForwardIntentResult::Rejected:
	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] Run Path 前进意图被拒绝：%s"),
			*RunExplorationPresentationCoordinator->GetLastErrorDetail().ToString());
		break;
	}
}

void AWacomPlayerController::HandleRunPathAnchoredHorizontalIntent(
	const int32 Direction)
{
	if (RunPathBranchSelectionController && CanRouteRunScenePointerInput())
	{
		RunPathBranchSelectionController->ShiftFocus(Direction);
	}
}

void AWacomPlayerController::HandleRunRouteChoiceStateChanged(
	const FWacomRunRouteChoiceState& State)
{
	if (RunPathBranchSelectionController)
	{
		RunPathBranchSelectionController->ApplyRouteChoiceState(State);
	}
	RefreshInteractToast();
}

void AWacomPlayerController::HandleRunNodeContentArrival(
	const FWacomRunNodeContentArrivalRequest& Request)
{
	if (Request.NodeType != EWacomMapNodeType::Encounter)
	{
		return;
	}

	UWacomRunEncounterSceneBindingComponent* Binding =
		RunExplorationSceneBindingRegistry
			? RunExplorationSceneBindingRegistry->FindEncounterBinding(Request.Node.NodeId)
			: nullptr;
	if (Binding && TryRestoreEncounterRetryForArrival(Request.Node, *Binding))
	{
		return;
	}
	if (!Binding || !TryEnterCurrentEncounter(Request.Node, *Binding, false))
	{
		if (Binding)
		{
			ArmCurrentEncounterRetry(
				Request.Node,
				*Binding,
				TEXT("AutomaticBattleEntryFailed"));
		}
		if (UWacomAppToastSubsystem* Toast = ResolveAppToastSubsystem())
		{
			Toast->ShowWarning(
				LOCTEXT("AutomaticBattleEntryFailed",
					"战斗未能启动，按 E 重试"));
		}
	}
}

bool AWacomPlayerController::TryEnterCurrentEncounter(
	const FWacomMapNodeHandle& Node,
	UWacomRunEncounterSceneBindingComponent& Binding,
	const bool bFromRetry)
{
	const UEncounterDefinition* EncounterDefinition = ResolveEncounterDefinition(Node);
	AWacomGameMode* GameMode =
		GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	if (!EncounterDefinition || !GameMode)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] Encounter 开战请求缺少定义或 GameMode。Node=%s/%s Retry=%s"),
			*Node.FloorId.ToString(),
			*Node.NodeId.ToString(),
			bFromRetry ? TEXT("true") : TEXT("false"));
		return false;
	}

	const FWacomStatus Result =
		GameMode->TryEnterBattle(Node, *EncounterDefinition, Binding);
	if (!Result.IsOk())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomPlayerController] Encounter 开战被拒绝。Node=%s/%s Retry=%s Detail=%s"),
			*Node.FloorId.ToString(),
			*Node.NodeId.ToString(),
			bFromRetry ? TEXT("true") : TEXT("false"),
			*Result.Detail.ToString());
		return false;
	}

	EncounterNodesRequiringManualRetry.Remove(Node);
	ClearCurrentEncounterRetry();
	return true;
}

const UEncounterDefinition* AWacomPlayerController::ResolveEncounterDefinition(
	const FWacomMapNodeHandle& Node) const
{
	const UWacomJourneyDefinition* Journey = RunSession
		? RunSession->GetRunState().ExplorationState.JourneyDefinition
		: nullptr;
	const UWacomFloorMapDefinition* Floor =
		Journey ? Journey->FindFloor(Node.FloorId) : nullptr;
	const FWacomMapNodeDefinition* Definition =
		Floor ? Floor->FindNode(Node.NodeId) : nullptr;
	return Definition && Definition->NodeType == EWacomMapNodeType::Encounter
		? Definition->Content.Encounter.EncounterDefinition
		: nullptr;
}

bool AWacomPlayerController::TryRestoreEncounterRetryForArrival(
	const FWacomMapNodeHandle& Node,
	UWacomRunEncounterSceneBindingComponent& Binding)
{
	if (!RunSession || !EncounterNodesRequiringManualRetry.Contains(Node))
	{
		return false;
	}

	const FRunExplorationSnapshot Snapshot = RunSession->BuildExplorationSnapshot();
	if (Snapshot.Outcome != ERunOutcome::InProgress
		|| Snapshot.CurrentNode != Node
		|| Snapshot.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		return false;
	}

	PendingEncounterRetryNode = Node;
	PendingEncounterRetryBinding = &Binding;
	PendingEncounterRetryReason = TEXT("ManualRetryRequiredAfterArrival");
	RefreshInteractToast();
	return true;
}

bool AWacomPlayerController::HasCurrentEncounterRetry() const
{
	if (!RunSession
		|| !PendingEncounterRetryNode.IsValid()
		|| !PendingEncounterRetryBinding.IsValid())
	{
		return false;
	}
	const FRunExplorationSnapshot Snapshot = RunSession->BuildExplorationSnapshot();
	if (Snapshot.Outcome != ERunOutcome::InProgress
		|| Snapshot.CurrentNode != PendingEncounterRetryNode
		|| Snapshot.ActiveActivityKind != ERunExplorationActivityKind::None)
	{
		return false;
	}
	const FRunMapNodeSnapshot* Node = Snapshot.Nodes.FindByPredicate(
		[this](const FRunMapNodeSnapshot& Candidate)
		{
			return Candidate.Handle == PendingEncounterRetryNode;
		});
	return Node
		&& Node->NodeType == EWacomMapNodeType::Encounter
		&& Node->Lifecycle == ERunMapNodeLifecycle::Visited;
}

void AWacomPlayerController::ArmCurrentEncounterRetry(
	const FWacomMapNodeHandle& Node,
	UWacomRunEncounterSceneBindingComponent& Binding,
	const FName Reason)
{
	EncounterNodesRequiringManualRetry.Add(Node);
	PendingEncounterRetryNode = Node;
	PendingEncounterRetryBinding = &Binding;
	PendingEncounterRetryReason = Reason;
	RefreshInteractToast();
}

void AWacomPlayerController::ClearCurrentEncounterRetry()
{
	PendingEncounterRetryNode = {};
	PendingEncounterRetryBinding.Reset();
	PendingEncounterRetryReason = NAME_None;
	RefreshInteractToast();
}

bool AWacomPlayerController::BuildRunPathBranchClickHitResult(FHitResult& OutHitResult) const
{
	return GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult);
}

bool AWacomPlayerController::IsInExplorationFlow() const
{
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	return GM
		&& GM->GetGameFlowState() == EGameFlowState::Exploration
		&& !bGameMenuViewpointStageTransitionActive;
}

bool AWacomPlayerController::CanRouteRunScenePointerInput() const
{
	return IsInExplorationFlow()
		&& !HasActiveRunGameMenuOrTransitionSuppression();
}

void AWacomPlayerController::StartRunWorldTargetProbePreviewLoop()
{
	if (!bEnableRunWorldTargetProbePreview && !bEnableRunWorldInteractableHoverPrompt)
	{
		ClearRunWorldTargetProbePreview();
		if (RunWorldInteractionRouter)
		{
			RunWorldInteractionRouter->ClearHoverPrompt(TEXT("Disabled"));
		}
		StopRunWorldTargetProbePreviewLoop();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RunWorldTargetProbePreviewTimerHandle,
		this,
		&AWacomPlayerController::UpdateRunWorldTargetProbePreview,
		FMath::Max(0.01f, RunWorldTargetProbePreviewIntervalSeconds),
		true);
}

void AWacomPlayerController::StopRunWorldTargetProbePreviewLoop()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RunWorldTargetProbePreviewTimerHandle);
	}
	RunWorldTargetProbePreviewTimerHandle = FTimerHandle();
}

void AWacomPlayerController::UpdateRunWorldTargetProbePreview()
{
	GetRunWorldInteractionRouter().UpdateTargetProbePreview();
}

void AWacomPlayerController::ClearRunWorldTargetProbePreview()
{
	if (RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter->ClearTargetProbePreview();
	}
}

bool AWacomPlayerController::ResolveRunWorldClickableInteractableFromHandle(
	const FWacomInteractionTargetHandle& Handle,
	AActor*& OutInteractableActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
	FName& OutRejectReason) const
{
	return GetRunWorldInteractionRouter().ResolveClickableInteractableFromHandle(
		Handle,
		OutInteractableActor,
		OutBridge,
		OutRejectReason);
}

void AWacomPlayerController::ClearRunMenuDropTargetProbe()
{
	GetRunFirstPersonCardDropCoordinator().ClearRunMenuDropTargetProbe();
}

void AWacomPlayerController::RefreshRunFirstPersonMenuLeaseDragBinding()
{
	GetRunFirstPersonCardDragController().RefreshBinding();
}

UWacomFirstPersonCardAnchorComponent*
AWacomPlayerController::ResolveFirstPersonCardAnchorForRunMenuProbe() const
{
	const AWacomPlayerCharacter* WacomCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	return WacomCharacter ? WacomCharacter->GetFirstPersonCardAnchorComponent() : nullptr;
}

bool AWacomPlayerController::ShouldHandleRunFirstPersonMenuDropProbe() const
{
	return GetRunFirstPersonCardDropCoordinator().ShouldHandleRunFirstPersonMenuDropProbe();
}

bool AWacomPlayerController::ShouldHandleRunWorldCardDropProbe() const
{
	return GetRunFirstPersonCardDropCoordinator().ShouldHandleRunWorldCardDropProbe();
}

void AWacomPlayerController::PumpFirstPersonCardActiveDragPointer()
{
	GetRunFirstPersonCardDragController().PumpActiveDragPointer();
}

bool AWacomPlayerController::TryReleaseFirstPersonCardActiveDragPointer()
{
	return GetRunFirstPersonCardDragController().TryReleaseActiveDragPointer();
}

bool AWacomPlayerController::TryCancelFirstPersonCardKeyboardShortcutDrag()
{
	return GetRunFirstPersonCardDragController().TryCancelKeyboardShortcutActiveDrag();
}

bool AWacomPlayerController::TryCancelFirstPersonCardActiveGestureForTurnBoundaryShortcut()
{
	return GetRunFirstPersonCardDragController().TryCancelActiveGestureForTurnBoundaryShortcut();
}

bool AWacomPlayerController::TryGetMouseWidgetPosition(FVector2D& OutWidgetPosition)
{
	if (FSlateApplication::IsInitialized())
	{
		const FGeometry ViewportGeometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
		const FVector2D ViewportSize = ViewportGeometry.GetLocalSize();
		if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
		{
			const FVector2D MouseWidgetPosition =
				ViewportGeometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
			if (!MouseWidgetPosition.ContainsNaN())
			{
				OutWidgetPosition = MouseWidgetPosition;
				return true;
			}
		}
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	OutWidgetPosition = FVector2D(MouseX, MouseY) / ViewportScale;
	return true;
}

UWacomRunMenuWidgetBase* AWacomPlayerController::ResolveOwningMenuForActiveRunMenuLease(FName LeaseId) const
{
	if (LeaseId.IsNone())
	{
		return nullptr;
	}

	for (int32 Index = ActiveGameMenuWidgets.Num() - 1; Index >= 0; --Index)
	{
		UWacomRunMenuWidgetBase* Menu =
			Cast<UWacomRunMenuWidgetBase>(ActiveGameMenuWidgets[Index].Get());
		if (Menu && Menu->HasOwnedRunMenuCardLease(LeaseId))
		{
			return Menu;
		}
	}
	return nullptr;
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetRunFirstPersonCardDetailController().HandleCardHovered(CardInstanceId, SlotView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetRunFirstPersonCardDetailController().HandleCardUnhovered(CardInstanceId, SlotView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	GetRunFirstPersonCardDetailController().HandleHoveredCardLayoutUpdated(CardInstanceId, SlotView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerPointerMoved(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	ApplyRunFirstPersonCardPointerCameraLookOverride(PointerView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerPointerLeft()
{
	ClearRunFirstPersonCardPointerCameraLookOverride();
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	ClearRunFirstPersonCardPointerCameraLookOverride();
	ApplyRunFirstPersonCardDragCameraLookOverride(DragView);
	GetRunFirstPersonCardDragController().HandleDragStarted(CardInstanceId, DragView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	ApplyRunFirstPersonCardDragCameraLookOverride(DragView);
	GetRunFirstPersonCardDragController().HandleDragUpdated(CardInstanceId, DragView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	ClearRunFirstPersonCardDragCameraLookOverride();
	GetRunFirstPersonCardDragController().HandleDragReleased(CardInstanceId, DragView);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragCancelled(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	ClearRunFirstPersonCardDragCameraLookOverride();
	GetRunFirstPersonCardDragController().HandleDragCancelled(CardInstanceId, DragView);
}

void AWacomPlayerController::ApplyRunFirstPersonCardPointerCameraLookOverride(
	const FWacomFirstPersonCardPointerView& PointerView)
{
	if (!IsInExplorationFlow() || !PointerView.bHasPointerViewportPosition)
	{
		ClearRunFirstPersonCardPointerCameraLookOverride();
		return;
	}

	const UWacomFirstPersonCardAnchorComponent* Anchor =
		ResolveFirstPersonCardAnchorForRunMenuProbe();
	if (!Anchor
		|| !Anchor->bAllowCameraLookDuringCardPointer
		|| Anchor->CardPointerCameraLookScale <= 0.0f)
	{
		ClearRunFirstPersonCardPointerCameraLookOverride();
		return;
	}

	AWacomPlayerCharacter* PlayerCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	if (!ApplyRunExplorationCursorLookOverride(
		PlayerCharacter,
		PointerView.PointerNormalizedViewportPosition,
		Anchor->CardPointerCameraLookScale,
		Anchor->CardPointerCameraLookInterpSpeedOverride))
	{
		ClearRunExplorationCursorLookOverride(PlayerCharacter);
	}
}

void AWacomPlayerController::ClearRunFirstPersonCardPointerCameraLookOverride()
{
	AWacomPlayerCharacter* PlayerCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	ClearRunExplorationCursorLookOverride(PlayerCharacter);
}

void AWacomPlayerController::ApplyRunFirstPersonCardDragCameraLookOverride(
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!IsInExplorationFlow() || !DragView.bHasPointerViewportPosition)
	{
		ClearRunFirstPersonCardDragCameraLookOverride();
		return;
	}

	const UWacomFirstPersonCardAnchorComponent* Anchor =
		ResolveFirstPersonCardAnchorForRunMenuProbe();
	if (!Anchor
		|| !Anchor->bAllowCameraLookDuringCardDrag
		|| Anchor->CardDragCameraLookScale <= 0.0f)
	{
		ClearRunFirstPersonCardDragCameraLookOverride();
		return;
	}

	AWacomPlayerCharacter* PlayerCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	if (!ApplyRunExplorationCursorLookOverride(
		PlayerCharacter,
		DragView.PointerNormalizedViewportPosition,
		Anchor->CardDragCameraLookScale,
		Anchor->CardDragCameraLookInterpSpeedOverride))
	{
		ClearRunExplorationCursorLookOverride(PlayerCharacter);
	}
}

void AWacomPlayerController::ClearRunFirstPersonCardDragCameraLookOverride()
{
	ClearRunFirstPersonCardPointerCameraLookOverride();
}

UWacomAppToastSubsystem* AWacomPlayerController::ResolveAppToastSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance
		? GameInstance->GetSubsystem<UWacomAppToastSubsystem>()
		: nullptr;
}

UWacomRunWorldCardDropReceiverComponent*
AWacomPlayerController::ResolveRunWorldCardDropReceiverFromHandle(
	const FWacomInteractionTargetHandle& Handle) const
{
	AActor* SourceActor = GetRunWorldInteractionRouter().ResolveSourceActorFromHandle(Handle);
	if (!SourceActor)
	{
		return nullptr;
	}
	return SourceActor->FindComponentByClass<UWacomRunWorldCardDropReceiverComponent>();
}

void AWacomPlayerController::ClearRunWorldCardDropProbe()
{
	GetRunFirstPersonCardDropCoordinator().ClearRunWorldCardDropProbe();
}

void AWacomPlayerController::RouteHandIndex(int32 OneBasedIndex)
{
	UBattleHUD* HUD = GetActiveBattleHUD();
	if (!HUD) { return; }

	TOptional<FVector2D> PointerWidgetPosition;
	FVector2D MouseWidgetPosition = FVector2D::ZeroVector;
	if (TryGetMouseWidgetPosition(MouseWidgetPosition))
	{
		PointerWidgetPosition = MouseWidgetPosition;
	}
	HUD->TryStartFirstPersonBattleHandDragByIndex(OneBasedIndex, PointerWidgetPosition);
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
	if (TryCancelFirstPersonCardActiveGestureForTurnBoundaryShortcut())
	{
		return;
	}

	if (UBattleHUD* HUD = GetActiveBattleHUD()) { HUD->OnWaitRequested(); }
}

void AWacomPlayerController::OnEndTurnPressed()
{
	if (TryCancelFirstPersonCardActiveGestureForTurnBoundaryShortcut())
	{
		return;
	}

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

void AWacomPlayerController::OnOpenMapPressed()
{
	TryToggleRunMapFromConsole();
}

// ================ 候选世界交互对象（use-key 模型）================

void AWacomPlayerController::RegisterCandidateInteractable(AActor* InteractableActor)
{
	if (!WacomWorldInteractableContractHelpers::IsWorldInteractableActor(InteractableActor))
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
		if (!WacomWorldInteractableContractHelpers::IsWorldInteractableActor(Candidate))
		{
			continue;
		}
		if (!WacomWorldInteractableContractHelpers::CanInteractWithActor(
			Candidate,
			const_cast<AWacomPlayerController*>(this)))
		{
			continue;
		}

		const FVector InteractLoc = WacomWorldInteractableContractHelpers::GetInteractLocationFromActor(
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
	if (HasCurrentEncounterRetry())
	{
		return LOCTEXT("EncounterRetryPrompt", "按 E 重新挑战");
	}
	if (RunPathBranchSelectionController)
	{
		const FText RouteChoicePrompt =
			RunPathBranchSelectionController->BuildInteractionPrompt();
		if (!RouteChoicePrompt.IsEmpty())
		{
			return RouteChoicePrompt;
		}
	}

	const FText HoverPrompt = RunWorldInteractionRouter
		? RunWorldInteractionRouter->GetHoverPrompt()
		: FText::GetEmpty();
	if (!HoverPrompt.IsEmpty())
	{
		return HoverPrompt;
	}

	AActor* Best = PickClosestInteractable();
	if (!Best)
	{
		return FText::GetEmpty();
	}
	return WacomWorldInteractableContractHelpers::GetInteractPromptTextFromActor(
		Best,
		const_cast<AWacomPlayerController*>(this));
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
	if (!CanRouteRunScenePointerInput())
	{
		return;
	}
	if (HasCurrentEncounterRetry())
	{
		UWacomRunEncounterSceneBindingComponent* Binding =
			PendingEncounterRetryBinding.Get();
		if (Binding
			&& !TryEnterCurrentEncounter(
				PendingEncounterRetryNode,
				*Binding,
				true))
		{
			if (UWacomAppToastSubsystem* Toast = ResolveAppToastSubsystem())
			{
				Toast->ShowWarning(
					LOCTEXT("EncounterRetryFailed",
						"战斗未能启动，按 E 重试"));
			}
		}
		return;
	}
	if (RunPathBranchSelectionController
		&& RunPathBranchSelectionController->IsChoiceRequired())
	{
		RunPathBranchSelectionController->ConfirmFocused();
		return;
	}

	AActor* Best = PickClosestInteractable();
	if (!Best)
	{
		UE_LOG(LogTemp, Display, TEXT("[WacomPlayerController] Interact: 没有候选交互对象"));
		return;
	}
	WacomWorldInteractableContractHelpers::TryInteractWithActor(Best, this);
}

bool AWacomPlayerController::RequestOpenShop(FName ShopId, const TArray<FRunShopOfferInput>& Offers)
{
	return FWacomExplorationScreenRouter::OpenShop(*this, ShopId, Offers);
}

bool AWacomPlayerController::RequestOpenShop(
	FName ShopId,
	const TArray<FRunShopOfferInput>& Offers,
	const FWacomFirstPersonViewStageRequest& StageRequest)
{
	return FWacomExplorationScreenRouter::OpenShop(*this, ShopId, Offers, StageRequest);
}

bool AWacomPlayerController::RequestOpenShop(const FRunShopVisitRequest& Request)
{
	return FWacomExplorationScreenRouter::OpenShop(*this, Request);
}

bool AWacomPlayerController::RequestOpenShop(
	const FRunShopVisitRequest& Request,
	const FWacomFirstPersonViewStageRequest& StageRequest)
{
	return FWacomExplorationScreenRouter::OpenShop(*this, Request, StageRequest);
}

void AWacomPlayerController::BeginGameMenuViewpointStageTransition(FName DebugReason)
{
	bGameMenuViewpointStageTransitionActive = true;
	ClearRunMenuDropTargetProbe();
	ClearRunWorldCardDropProbe();
	const FName HoverClearReason = DebugReason.IsNone()
		? FName(TEXT("GameMenuViewpointStage"))
		: FName(*FString::Printf(TEXT("%sViewpointStage"), *DebugReason.ToString()));
	if (RunWorldInteractionRouter)
	{
		RunWorldInteractionRouter->ClearHoverPrompt(HoverClearReason);
	}
	SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(true);
	if (AWacomPlayerCharacter* WacomPawn = GetPawn<AWacomPlayerCharacter>())
	{
		WacomPawn->SetExplorationInputEnabled(false);
	}
	RefreshInteractToast();
}

void AWacomPlayerController::ArmGameMenuViewpointReturnForMenu(UWacomMenuWidgetBase* MenuWidget)
{
	bGameMenuViewpointStageTransitionActive = false;
	bGameMenuViewpointReturnArmed = MenuWidget != nullptr;
	GameMenuViewpointReturnWidget = MenuWidget;
	RefreshInteractToast();
}

void AWacomPlayerController::ReturnFromGameMenuViewpointStageAfterFailedOpen()
{
	bGameMenuViewpointReturnArmed = false;
	GameMenuViewpointReturnWidget.Reset();
	BeginGameMenuViewpointStageTransition(FName(TEXT("GameMenuReturn")));

	if (AWacomPlayerCharacter* WacomPawn = GetPawn<AWacomPlayerCharacter>())
	{
		const TWeakObjectPtr<AWacomPlayerController> WeakThis(this);
		FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
			*WacomPawn,
			*this,
			[WeakThis]()
			{
				if (AWacomPlayerController* StrongThis = WeakThis.Get())
				{
					StrongThis->FinishGameMenuViewpointStageTransition();
				}
			});
		return;
	}

	FinishGameMenuViewpointStageTransition();
}

void AWacomPlayerController::FinishGameMenuViewpointStageTransition()
{
	bGameMenuViewpointStageTransitionActive = false;
	SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
	RefreshRunFirstPersonCardLayer();
	RefreshInteractToast();
}

bool AWacomPlayerController::RequestOpenRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition)
{
	return FWacomExplorationScreenRouter::OpenRunEvent(*this, PersistentId, EventDefinition);
}

bool AWacomPlayerController::RequestOpenRunEvent(
	FName PersistentId,
	UWacomRunEventDefinition* EventDefinition,
	const FWacomFirstPersonViewStageRequest& StageRequest)
{
	return FWacomExplorationScreenRouter::OpenRunEvent(*this, PersistentId, EventDefinition, StageRequest);
}

void AWacomPlayerController::TryOpenBackpackFromConsole()
{
	FWacomExplorationScreenRouter::OpenBackpack(*this);
}

void AWacomPlayerController::TryToggleRunMapFromConsole()
{
	FWacomExplorationScreenRouter::ToggleMap(*this);
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

static FAutoConsoleCommandWithWorld GWacomToggleRunMapCmd(
	TEXT("Wacom.ToggleRunMap"),
	TEXT("打开或关闭当前 Floor 地图（探索 Anchored 状态才生效）。"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AWacomPlayerController* WPC = FindLocalWacomPC(World))
		{
			WPC->TryToggleRunMapFromConsole();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Wacom.ToggleRunMap] 找不到 AWacomPlayerController"));
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

