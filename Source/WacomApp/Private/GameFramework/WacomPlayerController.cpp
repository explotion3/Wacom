// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include "Actors/BattleTriggerActor.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Actors/WacomRunTunnelBranchTargetActor.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Interaction/WacomInteractionTargetProvider.h"
#include "Interaction/WacomRunWorldCardDropReceiver.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"
#include "GameFramework/WacomExplorationScreenRouter.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "Characters/CharacterDefinition.h"
#include "Interaction/WacomWorldInteractable.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "RunStateTypes.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UI/Battle/BattleHUD.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomExplorationHUD.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"
#include "UI/Run/WacomRunMenuCardLeaseTestMenu.h"
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

	bool IsRunWorldClickableInteractableActor(const AActor* Actor)
	{
		return Actor
			&& Actor->GetClass()->ImplementsInterface(
				UWacomRunWorldClickableInteractable::StaticClass());
	}

	FText GetRunWorldClickHoverPromptFromActor(AActor* Actor, AWacomPlayerController* PC)
	{
		if (IWacomRunWorldClickableInteractable* Native =
			Cast<IWacomRunWorldClickableInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetRunWorldClickHoverPrompt_Implementation(PC);
			}
		}
		return IsRunWorldClickableInteractableActor(Actor)
			? IWacomRunWorldClickableInteractable::Execute_GetRunWorldClickHoverPrompt(Actor, PC)
			: FText::GetEmpty();
	}

	FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugViewFromActor(
		AActor* Actor,
		AWacomPlayerController* PC)
	{
		if (IWacomRunWorldClickableInteractable* Native =
			Cast<IWacomRunWorldClickableInteractable>(Actor))
		{
			if (Actor->GetClass()->IsNative())
			{
				return Native->GetRunWorldClickableDebugView_Implementation(PC);
			}
		}
	return IsRunWorldClickableInteractableActor(Actor)
		? IWacomRunWorldClickableInteractable::Execute_GetRunWorldClickableDebugView(Actor, PC)
		: FWacomRunWorldClickableInteractableDebugView();
	}

	FName BuildRunWorldClickableHoverReason(
		const FWacomRunWorldClickableInteractableDebugView& TriggerDebug)
	{
		if (TriggerDebug.bIsCompleted)
		{
			return TEXT("Completed");
		}
		if (TriggerDebug.bCanInteract)
		{
			return TEXT("Ok");
		}
		return TriggerDebug.RejectReason.IsNone()
			? TriggerDebug.LastDebugResult
			: TriggerDebug.RejectReason;
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

	const TCHAR* ToRunMenuCardDropIntentString(EWacomRunMenuCardDropIntentKind Kind)
	{
		switch (Kind)
		{
		case EWacomRunMenuCardDropIntentKind::ProbeZoneTarget:
			return TEXT("ProbeZoneTarget");
		case EWacomRunMenuCardDropIntentKind::SubmitZoneTarget:
			return TEXT("SubmitZoneTarget");
		case EWacomRunMenuCardDropIntentKind::Reject:
			return TEXT("Reject");
		case EWacomRunMenuCardDropIntentKind::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropRejectString(EWacomRunMenuCardDropRejectReason Reason)
	{
		switch (Reason)
		{
		case EWacomRunMenuCardDropRejectReason::NotInExploration:
			return TEXT("NotInExploration");
		case EWacomRunMenuCardDropRejectReason::MissingGameMenu:
			return TEXT("MissingGameMenu");
		case EWacomRunMenuCardDropRejectReason::MissingMenuLease:
			return TEXT("MissingMenuLease");
		case EWacomRunMenuCardDropRejectReason::MissingSession:
			return TEXT("MissingSession");
		case EWacomRunMenuCardDropRejectReason::InvalidSourceCard:
			return TEXT("InvalidSourceCard");
		case EWacomRunMenuCardDropRejectReason::MissingZoneTarget:
			return TEXT("MissingZoneTarget");
		case EWacomRunMenuCardDropRejectReason::UnsupportedTargetKind:
			return TEXT("UnsupportedTargetKind");
		case EWacomRunMenuCardDropRejectReason::MenuNotFound:
			return TEXT("MenuNotFound");
		case EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept:
			return TEXT("MenuDoesNotAccept");
		case EWacomRunMenuCardDropRejectReason::CardNotOwned:
			return TEXT("CardNotOwned");
		case EWacomRunMenuCardDropRejectReason::RunValidationFailed:
			return TEXT("RunValidationFailed");
		case EWacomRunMenuCardDropRejectReason::SubmitFailed:
			return TEXT("SubmitFailed");
		case EWacomRunMenuCardDropRejectReason::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropSubmitPolicyString(EWacomRunMenuCardDropSubmitPolicy Policy)
	{
		switch (Policy)
		{
		case EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard:
			return TEXT("ControllerDestroyOwnedCard");
		case EWacomRunMenuCardDropSubmitPolicy::MenuHandled:
			return TEXT("MenuHandled");
		case EWacomRunMenuCardDropSubmitPolicy::None:
		default:
			return TEXT("None");
		}
	}

	void FinalizeRunMenuCardDropDebug(
		FWacomRunMenuCardDropResolveResult& Result,
		const FVector2D& PointerPosition,
		bool bReleased)
	{
		Result.DebugSummary = FString::Printf(
			TEXT("RunMenuCardDropIntent{CardId=%s LeaseId=%s LeaseSource=%s Intent=%s Reject=%s SubmitPolicy=%s SubmitReason=%s ZoneId=%s RunValidation=%s CanSubmit=%s Submitted=%s Pointer=%s Released=%s Target=%s}"),
			*Result.SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.LeaseId.ToString(),
			*Result.LeaseSourceId.ToString(),
			ToRunMenuCardDropIntentString(Result.IntentKind),
			ToRunMenuCardDropRejectString(Result.RejectReason),
			ToRunMenuCardDropSubmitPolicyString(Result.SubmitPolicy),
			*Result.SubmitReason.ToString(),
			*Result.ZoneId.ToString(),
			*Result.RunValidationReason.ToString(),
			Result.bCanSubmit ? TEXT("true") : TEXT("false"),
			Result.bSubmitted ? TEXT("true") : TEXT("false"),
			*PointerPosition.ToString(),
			bReleased ? TEXT("true") : TEXT("false"),
			*Result.TargetHandle.ToString());
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

	void FinalizeRunWorldCardDropDebugSummary(
		FString& OutDebugSummary,
		const FVector2D& PointerPosition,
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& TargetHandle,
		const FRunWorldCardInteractionValidation& Validation,
		const AActor* TargetActor,
		const UWacomRunWorldCardDropReceiverComponent* Receiver,
		FName ResolveReason,
		bool bReleased,
		bool bSubmitted,
		FName ToastSource = NAME_None,
		const FText& ToastText = FText::GetEmpty())
	{
		const TCHAR* Phase = bReleased ? TEXT("Release") : TEXT("Preview");
		OutDebugSummary = FString::Printf(
			TEXT("RunWorldCardDrop{Phase=%s Card=%s TargetActor=%s Receiver=%s StableId=%s CanSubmit=%s Reason=%s Resolve=%s Submitted=%s Released=%s ToastSource=%s FailureToast=%s Pointer=%s Target=%s Validation=%s}"),
			Phase,
			*CardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*GetDebugObjectName(TargetActor),
			*GetDebugObjectName(Receiver),
			*TargetHandle.StableTargetId.ToString(),
			Validation.bCanSubmit ? TEXT("true") : TEXT("false"),
			*Validation.DisabledReason.ToString(),
			*ResolveReason.ToString(),
			bSubmitted ? TEXT("true") : TEXT("false"),
			bReleased ? TEXT("true") : TEXT("false"),
			ToastSource.IsNone() ? TEXT("None") : *ToastSource.ToString(),
			ToastText.IsEmpty() ? TEXT("None") : *ToastText.ToString(),
			*PointerPosition.ToString(),
			*TargetHandle.ToString(),
			*Validation.DebugSummary);
	}

	FText BuildRunWorldCardDropConfigWarningToast(FName Reason)
	{
		if (Reason.IsNone())
		{
			return LOCTEXT("RunWorldCardDropConfigWarning", "场景交互配置异常");
		}
		return FText::Format(
			LOCTEXT("RunWorldCardDropConfigWarningWithReason", "场景交互配置异常：{0}"),
			FText::FromName(Reason));
	}
}

AWacomPlayerController::AWacomPlayerController()
{
	RunFirstPersonCardSourceComponent =
		CreateDefaultSubobject<UWacomRunFirstPersonCardSourceComponent>(
			TEXT("RunFirstPersonCardSourceComponent"));
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

		if (RunFirstPersonCardSourceComponent)
		{
			RunFirstPersonCardSourceComponent->BindRunSession(RunSession);
			RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerActive(true);
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
	ClearRunFirstPersonCardLayer();
	ClearRunWorldTargetProbePreview();
	ClearRunWorldInteractableHoverPrompt(TEXT("EndPlay"));
	StopRunWorldTargetProbePreviewLoop();
	Super::EndPlay(EndPlayReason);
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
	if (Params.Key == EKeys::LeftMouseButton
		&& Params.Event == IE_Released
		&& TryRouteRunWorldInteractableClick())
	{
		return true;
	}

	return Super::InputKey(Params);
}

// ================ 战斗状态切换转发 ================

void AWacomPlayerController::RequestEnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger)
{
	ClearRunMenuDropTargetProbe();
	ClearRunFirstPersonCardLayer();
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

void AWacomPlayerController::SetRunFirstPersonCardLayerActive(bool bActive)
{
	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerActive(bActive);
	}
	RefreshRunFirstPersonMenuLeaseDragBinding();
}

bool AWacomPlayerController::RefreshRunFirstPersonCardLayer()
{
	if (!RunFirstPersonCardSourceComponent)
	{
		return false;
	}

	RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
	const bool bRefreshed = RunFirstPersonCardSourceComponent->RefreshRunFirstPersonCardLayer();
	RefreshRunFirstPersonMenuLeaseDragBinding();
	return bRefreshed;
}

void AWacomPlayerController::ClearRunFirstPersonCardLayer()
{
	ClearRunMenuDropTargetProbe();
	ClearRunWorldCardDropProbe();
	ClearRunWorldInteractableHoverPrompt(TEXT("FirstPersonLayerCleared"));
	RefreshRunFirstPersonMenuLeaseDragBinding();
	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerActive(false);
	}
	ActiveGameMenuWidgets.Reset();
	bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = false;
	RefreshRunFirstPersonMenuLeaseDragBinding();
}

void AWacomPlayerController::SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed)
{
	if (RunFirstPersonCardSourceComponent)
	{
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerSuppressedByGameMenu(bSuppressed);
	}
}

bool AWacomPlayerController::SetRunFirstPersonCardLayerMenuLease(
	FName LeaseId,
	FName SourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& Entries)
{
	if (!RunFirstPersonCardSourceComponent)
	{
		return false;
	}

	RunFirstPersonCardSourceComponent->BindRunSession(ResolveRunSessionForFirstPersonCardSource());
	const bool bSet =
		RunFirstPersonCardSourceComponent->SetRunFirstPersonCardLayerMenuLease(LeaseId, SourceId, Entries);
	RefreshRunFirstPersonMenuLeaseDragBinding();
	return bSet;
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
	}
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
	}
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
	ActiveGameMenuWidgets.RemoveAll(
		[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Existing)
		{
			return !Existing.IsValid();
		});

	if (!ActiveGameMenuWidgets.Contains(MenuWidget))
	{
		ActiveGameMenuWidgets.Add(MenuWidget);
	}
	ClearRunWorldInteractableHoverPrompt(TEXT("GameMenuActive"));
	RefreshRunFirstPersonCardLayerMenuSuppression();
}

void AWacomPlayerController::UnregisterActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget)
{
	ActiveGameMenuWidgets.RemoveAll(
		[MenuWidget](const TWeakObjectPtr<UWacomMenuWidgetBase>& Existing)
		{
			return !Existing.IsValid() || Existing.Get() == MenuWidget;
		});
	RefreshRunFirstPersonCardLayerMenuSuppression();
}

void AWacomPlayerController::SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(bool bSuppressed)
{
	if (bRunFirstPersonCardLayerTransitionSuppressedByGameMenu == bSuppressed)
	{
		RefreshRunFirstPersonCardLayerMenuSuppression();
		return;
	}

	bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = bSuppressed;
	if (bSuppressed)
	{
		ClearRunWorldInteractableHoverPrompt(TEXT("GameMenuTransition"));
	}
	RefreshRunFirstPersonCardLayerMenuSuppression();
}

void AWacomPlayerController::RegisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget)
{
	if (!DropTarget)
	{
		return;
	}

	RunMenuDropTargets.RemoveAll(
		[](const TWeakObjectPtr<UWacomRunMenuDropTargetWidget>& Existing)
		{
			return !Existing.IsValid();
		});
	if (!RunMenuDropTargets.ContainsByPredicate(
		[DropTarget](const TWeakObjectPtr<UWacomRunMenuDropTargetWidget>& Existing)
		{
			return Existing.Get() == DropTarget;
		}))
	{
		RunMenuDropTargets.Add(DropTarget);
	}
}

void AWacomPlayerController::UnregisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget)
{
	if (PreviewedRunMenuDropTarget.Get() == DropTarget)
	{
		ClearRunMenuDropTargetProbe();
	}

	RunMenuDropTargets.RemoveAll(
		[DropTarget](const TWeakObjectPtr<UWacomRunMenuDropTargetWidget>& Existing)
		{
			return !Existing.IsValid() || Existing.Get() == DropTarget;
		});
}

bool AWacomPlayerController::TryProbeRunMenuDropTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	for (int32 Index = RunMenuDropTargets.Num() - 1; Index >= 0; --Index)
	{
		UWacomRunMenuDropTargetWidget* Target = RunMenuDropTargets[Index].Get();
		if (!Target || !Target->CanProbeRunMenuDropTarget())
		{
			continue;
		}

		if (Target->ContainsWidgetPosition(WidgetPosition))
		{
			OutHandle = Target->BuildZoneTargetHandle(WidgetPosition);
			return OutHandle.IsValid()
				&& OutHandle.TargetKind == EWacomInteractionTargetKind::Zone
				&& !OutHandle.ZoneId.IsNone();
		}
	}

	return false;
}

void AWacomPlayerController::RefreshRunFirstPersonCardLayerMenuSuppression()
{
	ActiveGameMenuWidgets.RemoveAll(
		[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Existing)
		{
			return !Existing.IsValid();
		});

	const bool bShouldSuppress =
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| ActiveGameMenuWidgets.Num() > 0;
	SetRunFirstPersonCardLayerSuppressedByGameMenu(bShouldSuppress);
	if (!bShouldSuppress)
	{
		ClearRunMenuDropTargetProbe();
	}
	RefreshRunFirstPersonMenuLeaseDragBinding();
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

bool AWacomPlayerController::TryProbeBattleSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	UBattleHUD* HUD = nullptr;
	if (!CanRouteBattleSceneTargetClick(HUD))
	{
		return false;
	}

	FHitResult HitResult;
	if (!BuildBattleSceneClickHitResult(HitResult))
	{
		return false;
	}

	OutHandle = BuildInteractionTargetHandleFromHit(HitResult);
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		if (ProjectWorldLocationToScreen(HitResult.Location, ScreenPosition))
		{
			const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
			OutHandle.ScreenPosition = ScreenPosition / ViewportScale;
		}
	}
	return OutHandle.IsValid();
}

bool AWacomPlayerController::TryProbeBattleSceneInteractionTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	UBattleHUD* HUD = nullptr;
	if (!CanRouteBattleSceneTargetClick(HUD))
	{
		return false;
	}

	FHitResult HitResult;
	if (!BuildBattleSceneInteractionTargetHitResultAtWidgetPosition(WidgetPosition, HitResult))
	{
		return false;
	}

	OutHandle = BuildInteractionTargetHandleFromHit(HitResult);
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		OutHandle.ScreenPosition = WidgetPosition;
	}
	return OutHandle.IsValid();
}

bool AWacomPlayerController::TryProbeRunSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	if (!IsInExplorationFlow())
	{
		return false;
	}

	FHitResult HitResult;
	if (!BuildRunSceneClickHitResult(HitResult))
	{
		return false;
	}

	OutHandle = BuildInteractionTargetHandleFromHit(HitResult);
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		if (ProjectWorldLocationToScreen(HitResult.Location, ScreenPosition))
		{
			const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
			OutHandle.ScreenPosition = ScreenPosition / ViewportScale;
		}
	}

	const bool bAccepted = OutHandle.IsValid()
		&& OutHandle.TargetKind == EWacomInteractionTargetKind::World
		&& OutHandle.TargetTag == WacomTags::Interaction_Target_Run_Object
		&& OutHandle.WorldTargetId.IsValid();
	if (!bAccepted)
	{
		OutHandle = FWacomInteractionTargetHandle();
	}
	return bAccepted;
}

bool AWacomPlayerController::TryProbeRunSceneInteractionTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	if (!IsInExplorationFlow())
	{
		return false;
	}

	FHitResult HitResult;
	if (!BuildRunSceneInteractionTargetHitResultAtWidgetPosition(WidgetPosition, HitResult))
	{
		return false;
	}

	OutHandle = BuildInteractionTargetHandleFromHit(HitResult);
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		OutHandle.ScreenPosition = WidgetPosition;
	}

	const bool bAccepted = OutHandle.IsValid()
		&& OutHandle.TargetKind == EWacomInteractionTargetKind::World
		&& OutHandle.TargetTag == WacomTags::Interaction_Target_Run_Object
		&& OutHandle.WorldTargetId.IsValid();
	if (!bAccepted)
	{
		OutHandle = FWacomInteractionTargetHandle();
	}
	return bAccepted;
}

bool AWacomPlayerController::TryRouteRunWorldInteractableClick()
{
	if (!bEnableRunWorldInteractableClick)
	{
		return false;
	}
	if (!IsInExplorationFlow())
	{
		return false;
	}
	const bool bHasActiveGameMenu =
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| ActiveGameMenuWidgets.ContainsByPredicate(
			[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Menu)
			{
				return Menu.IsValid();
			});
	if (bHasActiveGameMenu || ShouldHandleRunFirstPersonMenuDropProbe())
	{
		if (bLogRunWorldInteractableClick)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableClick] NoRoute reason=BlockedByMenuOrDrag"));
		}
		return false;
	}

	FWacomInteractionTargetHandle Handle;
	if (!TryProbeRunSceneInteractionTarget(Handle))
	{
		if (bLogRunWorldInteractableClick)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableClick] NoRoute reason=NoRunWorldTarget"));
		}
		return false;
	}

	AActor* TargetActor = nullptr;
	UWacomRunWorldInteractionTargetBridgeComponent* TargetBridge = nullptr;
	FName RejectReason = NAME_None;
	if (!ResolveRunWorldClickableInteractableFromHandle(
		Handle,
		TargetActor,
		TargetBridge,
		RejectReason))
	{
		if (bLogRunWorldInteractableClick)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableClick] NoRoute reason=%s actor=%s bridge=%s handle=%s"),
				*RejectReason.ToString(),
				*GetDebugObjectName(TargetActor),
				*GetDebugObjectName(TargetBridge),
				*Handle.ToString());
		}
		return false;
	}

	const bool bRouted = TryInteractWithActor(TargetActor, this);
	if (bLogRunWorldInteractableClick)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomRunWorldInteractableClick] Route actor=%s handle=%s result=%s"),
			*GetDebugObjectName(TargetActor),
			*Handle.ToString(),
			bRouted ? TEXT("true") : TEXT("false"));
	}
	return bRouted;
}

FString AWacomPlayerController::GetRunWorldInteractableHoverDebugSummary() const
{
	AActor* HoverActor = HoveredRunWorldInteractableActor.Get();
	FWacomRunWorldClickableInteractableDebugView TriggerDebug;
	if (IsRunWorldClickableInteractableActor(HoverActor))
	{
		TriggerDebug =
			GetRunWorldClickableDebugViewFromActor(
				HoverActor,
				const_cast<AWacomPlayerController*>(this));
	}
	else
	{
		TriggerDebug.ActorName = GetDebugObjectName(HoverActor);
		TriggerDebug.StableId = HoveredRunWorldInteractableHandle.StableTargetId;
		TriggerDebug.bHasStableId = !TriggerDebug.StableId.IsNone();
		TriggerDebug.bImplementsWorldInteractable = IsWorldInteractableActor(HoverActor);
		TriggerDebug.bImplementsClickableContract = IsRunWorldClickableInteractableActor(HoverActor);
		TriggerDebug.HoverPrompt = HoveredRunWorldInteractablePrompt.ToString();
		TriggerDebug.RejectReason = LastRunWorldInteractableHoverReason;
		TriggerDebug.LastDebugResult = LastRunWorldInteractableHoverReason;
	}

	return FString::Printf(
		TEXT("RunWorldInteractableHover{Actor=%s StableId=%s Prompt=%s CanInteract=%s Completed=%s Reason=%s Target=%s Debug=%s}"),
		*GetDebugObjectName(HoverActor),
		*HoveredRunWorldInteractableHandle.StableTargetId.ToString(),
		*HoveredRunWorldInteractablePrompt.ToString(),
		TriggerDebug.bCanInteract ? TEXT("true") : TEXT("false"),
		TriggerDebug.bIsCompleted ? TEXT("true") : TEXT("false"),
		*LastRunWorldInteractableHoverReason.ToString(),
		*HoveredRunWorldInteractableHandle.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(TriggerDebug));
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
	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!DeprojectScreenPositionToWorld(PixelPosition.X, PixelPosition.Y, WorldOrigin, WorldDirection))
	{
		return false;
	}

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.0f;
	return GetWorld() && GetWorld()->LineTraceSingleByChannel(OutHitResult, WorldOrigin, TraceEnd, ECC_Visibility);
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

bool AWacomPlayerController::IsInExplorationFlow() const
{
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	return GM && GM->GetGameFlowState() == EGameFlowState::Exploration;
}

void AWacomPlayerController::StartRunWorldTargetProbePreviewLoop()
{
	if (!bEnableRunWorldTargetProbePreview && !bEnableRunWorldInteractableHoverPrompt)
	{
		ClearRunWorldTargetProbePreview();
		ClearRunWorldInteractableHoverPrompt(TEXT("Disabled"));
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
	if (!IsInExplorationFlow())
	{
		ClearRunWorldTargetProbePreview();
		ClearRunWorldInteractableHoverPrompt(TEXT("NotInExploration"));
		return;
	}

	FWacomInteractionTargetHandle Handle;
	const bool bHasRunTarget = TryProbeRunSceneInteractionTarget(Handle);
	AActor* InteractableActor = nullptr;
	UWacomRunWorldInteractionTargetBridgeComponent* NewBridge = nullptr;
	FName ResolveRejectReason = bHasRunTarget
		? FName(TEXT("NotResolved"))
		: FName(TEXT("NoRunWorldTarget"));
	const bool bResolvedClickable = bHasRunTarget
		&& ResolveRunWorldClickableInteractableFromHandle(
			Handle,
			InteractableActor,
			NewBridge,
			ResolveRejectReason);
	const bool bCanShowRunWorldHover = CanShowRunWorldInteractableHoverPrompt();
	if (!bEnableRunWorldTargetProbePreview || !bCanShowRunWorldHover || !bResolvedClickable)
	{
		NewBridge = nullptr;
	}

	UWacomRunWorldInteractionTargetBridgeComponent* OldBridge = PreviewedRunWorldTargetBridge.Get();
	if (OldBridge != NewBridge)
	{
		if (OldBridge)
		{
			OldBridge->ClearProbePreview();
		}

		PreviewedRunWorldTargetBridge = NewBridge;
		if (NewBridge)
		{
			NewBridge->SetProbePreviewActive(true);
			if (bLogRunWorldTargetProbePreview)
			{
				UE_LOG(LogTemp, Display,
					TEXT("[WacomRunWorldTargetProbe] Preview handle=%s bridge=%s"),
					*Handle.ToString(),
					*GetDebugObjectName(NewBridge));
			}
		}
		else if (bLogRunWorldTargetProbePreview)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomRunWorldTargetProbe] Preview cleared"));
		}
	}

	if (!bEnableRunWorldInteractableHoverPrompt)
	{
		ClearRunWorldInteractableHoverPrompt(TEXT("Disabled"));
		return;
	}
	if (!bCanShowRunWorldHover)
	{
		ClearRunWorldInteractableHoverPrompt(TEXT("BlockedByMenuOrDrag"));
		return;
	}
	if (!bHasRunTarget)
	{
		ClearRunWorldInteractableHoverPrompt(TEXT("NoRunWorldTarget"));
		return;
	}

	if (!bResolvedClickable)
	{
		ClearRunWorldInteractableHoverPrompt(ResolveRejectReason);
		return;
	}

	UpdateRunWorldInteractableHoverPrompt(Handle, InteractableActor);
}

void AWacomPlayerController::ClearRunWorldTargetProbePreview()
{
	if (UWacomRunWorldInteractionTargetBridgeComponent* Bridge = PreviewedRunWorldTargetBridge.Get())
	{
		Bridge->ClearProbePreview();
	}
	PreviewedRunWorldTargetBridge.Reset();
}

bool AWacomPlayerController::CanShowRunWorldInteractableHoverPrompt() const
{
	if (!bEnableRunWorldInteractableHoverPrompt || !IsInExplorationFlow())
	{
		return false;
	}

	const bool bHasActiveGameMenu =
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| ActiveGameMenuWidgets.ContainsByPredicate(
			[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Menu)
			{
				return Menu.IsValid();
			});
	return !bHasActiveGameMenu && !ShouldHandleRunFirstPersonMenuDropProbe();
}

AActor* AWacomPlayerController::ResolveSourceActorFromInteractionTargetHandle(
	const FWacomInteractionTargetHandle& Handle) const
{
	UObject* SourceObject = Handle.SourceObject.Get();
	if (!SourceObject)
	{
		return nullptr;
	}

	if (const UActorComponent* SourceComponent = Cast<UActorComponent>(SourceObject))
	{
		return SourceComponent->GetOwner();
	}
	return Cast<AActor>(SourceObject);
}

bool AWacomPlayerController::ResolveRunWorldClickableInteractableFromHandle(
	const FWacomInteractionTargetHandle& Handle,
	AActor*& OutInteractableActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
	FName& OutRejectReason) const
{
	OutInteractableActor = nullptr;
	OutBridge = nullptr;
	OutRejectReason = NAME_None;

	if (!Handle.IsValid())
	{
		OutRejectReason = TEXT("InvalidHandle");
		return false;
	}
	if (Handle.TargetKind != EWacomInteractionTargetKind::World)
	{
		OutRejectReason = TEXT("WrongTargetKind");
		return false;
	}
	if (Handle.TargetTag != WacomTags::Interaction_Target_Run_Object)
	{
		OutRejectReason = TEXT("WrongTargetTag");
		return false;
	}
	if (!Handle.WorldTargetId.IsValid())
	{
		OutRejectReason = TEXT("MissingWorldTargetId");
		return false;
	}

	OutInteractableActor = ResolveSourceActorFromInteractionTargetHandle(Handle);
	if (!OutInteractableActor)
	{
		OutRejectReason = TEXT("MissingSourceActor");
		return false;
	}

	OutBridge = ResolveRunWorldTargetBridgeFromHandle(Handle);
	if (!IsWorldInteractableActor(OutInteractableActor))
	{
		OutRejectReason = TEXT("MissingWorldInteractableContract");
		return false;
	}
	if (!IsRunWorldClickableInteractableActor(OutInteractableActor))
	{
		OutRejectReason = TEXT("MissingClickableContract");
		return false;
	}
	if (!OutBridge)
	{
		OutRejectReason = TEXT("MissingRunWorldBridge");
		return false;
	}

	OutRejectReason = TEXT("Ok");
	return true;
}

void AWacomPlayerController::UpdateRunWorldInteractableHoverPrompt(
	const FWacomInteractionTargetHandle& Handle,
	AActor* InteractableActor)
{
	if (!InteractableActor)
	{
		ClearRunWorldInteractableHoverPrompt(TEXT("MissingInteractableActor"));
		return;
	}

	if (!IsRunWorldClickableInteractableActor(InteractableActor))
	{
		ClearRunWorldInteractableHoverPrompt(TEXT("MissingClickableContract"));
		return;
	}

	const FText NewPrompt = GetRunWorldClickHoverPromptFromActor(InteractableActor, this);
	const FWacomRunWorldClickableInteractableDebugView TriggerDebug =
		GetRunWorldClickableDebugViewFromActor(InteractableActor, this);
	const FName NewReason = BuildRunWorldClickableHoverReason(TriggerDebug);

	const bool bChanged =
		HoveredRunWorldInteractableActor.Get() != InteractableActor
		|| !HoveredRunWorldInteractablePrompt.EqualTo(NewPrompt)
		|| HoveredRunWorldInteractableHandle.StableTargetId != Handle.StableTargetId
		|| LastRunWorldInteractableHoverReason != NewReason;

	HoveredRunWorldInteractableActor = InteractableActor;
	HoveredRunWorldInteractableHandle = Handle;
	HoveredRunWorldInteractablePrompt = NewPrompt;
	LastRunWorldInteractableHoverReason = NewReason;

	if (bChanged)
	{
		RefreshInteractToast();
		if (bLogRunWorldInteractableHoverPrompt)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableHover] Hover %s"),
				*GetRunWorldInteractableHoverDebugSummary());
		}
	}
}

void AWacomPlayerController::ClearRunWorldInteractableHoverPrompt(FName Reason)
{
	const bool bHadHover =
		HoveredRunWorldInteractableActor.IsValid()
		|| !HoveredRunWorldInteractablePrompt.IsEmpty()
		|| HoveredRunWorldInteractableHandle.IsValid();
	HoveredRunWorldInteractableActor.Reset();
	HoveredRunWorldInteractableHandle = FWacomInteractionTargetHandle();
	HoveredRunWorldInteractablePrompt = FText::GetEmpty();
	LastRunWorldInteractableHoverReason = Reason.IsNone()
		? FName(TEXT("Cleared"))
		: Reason;

	if (bHadHover)
	{
		RefreshInteractToast();
		if (bLogRunWorldInteractableHoverPrompt)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableHover] Cleared reason=%s"),
				*LastRunWorldInteractableHoverReason.ToString());
		}
	}
}

void AWacomPlayerController::ClearRunMenuDropTargetProbe()
{
	if (UWacomRunMenuDropTargetWidget* Target = PreviewedRunMenuDropTarget.Get())
	{
		Target->ClearRunMenuDropPreviewState();
	}
	PreviewedRunMenuDropTarget.Reset();
	LastRunMenuDropProbeDebugSummary = TEXT("RunMenuDropProbe{State=Cleared}");

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchorForRunMenuProbe())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None,
			TOptional<FVector2D>(),
			LastRunMenuDropProbeDebugSummary);
	}
}

void AWacomPlayerController::RefreshRunFirstPersonMenuLeaseDragBinding()
{
	UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchorForRunMenuProbe();
	UWacomFirstPersonCardAnchorComponent* BoundAnchor = RunMenuProbeBoundAnchor.Get();

	const bool bShouldBind =
		Anchor
		&& RunFirstPersonCardSourceComponent
		&& (RunFirstPersonCardSourceComponent->HasActiveMenuLease()
			|| ShouldHandleRunWorldCardDropProbe());

	if ((!bShouldBind || BoundAnchor != Anchor) && BoundAnchor)
	{
		BoundAnchor->OnFirstPersonCardLayerDragStarted.RemoveAll(this);
		BoundAnchor->OnFirstPersonCardLayerDragUpdated.RemoveAll(this);
		BoundAnchor->OnFirstPersonCardLayerDragReleased.RemoveAll(this);
		BoundAnchor->OnFirstPersonCardLayerDragCancelled.RemoveAll(this);
		RunMenuProbeBoundAnchor.Reset();
		bRunFirstPersonMenuLeaseDragBound = false;
	}

	if (bShouldBind && Anchor && RunMenuProbeBoundAnchor.Get() != Anchor)
	{
		Anchor->OnFirstPersonCardLayerDragStarted.RemoveAll(this);
		Anchor->OnFirstPersonCardLayerDragUpdated.RemoveAll(this);
		Anchor->OnFirstPersonCardLayerDragReleased.RemoveAll(this);
		Anchor->OnFirstPersonCardLayerDragCancelled.RemoveAll(this);
		Anchor->OnFirstPersonCardLayerDragStarted.AddUObject(
			this,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragStarted);
		Anchor->OnFirstPersonCardLayerDragUpdated.AddUObject(
			this,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragUpdated);
		Anchor->OnFirstPersonCardLayerDragReleased.AddUObject(
			this,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragReleased);
		Anchor->OnFirstPersonCardLayerDragCancelled.AddUObject(
			this,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerDragCancelled);
		RunMenuProbeBoundAnchor = Anchor;
		bRunFirstPersonMenuLeaseDragBound = true;
	}
}

UWacomFirstPersonCardAnchorComponent*
AWacomPlayerController::ResolveFirstPersonCardAnchorForRunMenuProbe() const
{
	const AWacomPlayerCharacter* WacomCharacter = Cast<AWacomPlayerCharacter>(GetPawn());
	return WacomCharacter ? WacomCharacter->GetFirstPersonCardAnchorComponent() : nullptr;
}

bool AWacomPlayerController::ShouldHandleRunFirstPersonMenuDropProbe() const
{
	const bool bHasActiveGameMenu =
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| ActiveGameMenuWidgets.ContainsByPredicate(
			[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Menu)
			{
				return Menu.IsValid();
			});
	return IsInExplorationFlow()
		&& bHasActiveGameMenu
		&& RunFirstPersonCardSourceComponent
		&& RunFirstPersonCardSourceComponent->HasActiveMenuLease();
}

bool AWacomPlayerController::ShouldHandleRunWorldCardDropProbe() const
{
	if (!bEnableRunWorldCardDrop
		|| !IsInExplorationFlow()
		|| !RunFirstPersonCardSourceComponent
		|| !RunFirstPersonCardSourceComponent->IsRunFirstPersonCardLayerActive()
		|| RunFirstPersonCardSourceComponent->HasActiveMenuLease())
	{
		return false;
	}

	const bool bHasActiveGameMenu =
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| ActiveGameMenuWidgets.ContainsByPredicate(
			[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Menu)
			{
				return Menu.IsValid();
			});
	return !bHasActiveGameMenu;
}

UWacomMenuWidgetBase* AWacomPlayerController::ResolveOwningMenuForActiveRunMenuLease(FName LeaseId) const
{
	if (LeaseId.IsNone())
	{
		return nullptr;
	}

	for (int32 Index = ActiveGameMenuWidgets.Num() - 1; Index >= 0; --Index)
	{
		UWacomMenuWidgetBase* Menu = ActiveGameMenuWidgets[Index].Get();
		if (Menu && Menu->HasOwnedRunFirstPersonCardLayerMenuLease(LeaseId))
		{
			return Menu;
		}
	}
	return nullptr;
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragStarted(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (ShouldHandleRunFirstPersonMenuDropProbe())
	{
		ApplyRunMenuDropProbeFeedback(CardInstanceId, DragView, /*bReleased*/ false);
		return;
	}
	ApplyRunWorldCardDropProbeFeedback(CardInstanceId, DragView, /*bReleased*/ false);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (ShouldHandleRunFirstPersonMenuDropProbe())
	{
		ApplyRunMenuDropProbeFeedback(CardInstanceId, DragView, /*bReleased*/ false);
		return;
	}
	ApplyRunWorldCardDropProbeFeedback(CardInstanceId, DragView, /*bReleased*/ false);
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragReleased(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (ShouldHandleRunFirstPersonMenuDropProbe())
	{
		const bool bKeepReleasePreview =
			ApplyRunMenuDropProbeFeedback(CardInstanceId, DragView, /*bReleased*/ true);
		if (!bKeepReleasePreview)
		{
			ClearRunMenuDropTargetProbe();
		}
		return;
	}
	ApplyRunWorldCardDropProbeFeedback(CardInstanceId, DragView, /*bReleased*/ true);
	ClearRunWorldCardDropProbe();
}

void AWacomPlayerController::HandleRunFirstPersonCardLayerDragCancelled(
	const FGuid& /*CardInstanceId*/,
	const FWacomFirstPersonCardDragView& /*DragView*/)
{
	ClearRunMenuDropTargetProbe();
	ClearRunWorldCardDropProbe();
}

bool AWacomPlayerController::ApplyRunMenuDropProbeFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	const FVector2D ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;

	FWacomRunMenuCardDropResolveResult Result =
		ResolveRunMenuCardDropIntent(CardInstanceId, DragView);
	if (bReleased && Result.bCanSubmit)
	{
		SubmitResolvedRunMenuCardDropIntent(Result);
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, bReleased);
	}
	LastRunMenuDropProbeDebugSummary = Result.DebugSummary;

	if (Result.IntentKind == EWacomRunMenuCardDropIntentKind::None
		|| !CardInstanceId.IsValid())
	{
		ClearRunMenuDropTargetProbe();
		return false;
	}

	UWacomRunMenuDropTargetWidget* NewTarget =
		Cast<UWacomRunMenuDropTargetWidget>(Result.TargetHandle.SourceObject.Get());

	if (UWacomRunMenuDropTargetWidget* PreviousTarget = PreviewedRunMenuDropTarget.Get())
	{
		if (PreviousTarget != NewTarget)
		{
			PreviousTarget->ClearRunMenuDropPreviewState();
		}
	}
	PreviewedRunMenuDropTarget = NewTarget;

	const bool bHasActiveDrag =
		DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard;

	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	TOptional<FVector2D> FeedbackTargetPosition;
	if (NewTarget)
	{
		EWacomRunMenuDropTargetPreviewState PreviewState =
			EWacomRunMenuDropTargetPreviewState::Probe;
		if (Result.IntentKind == EWacomRunMenuCardDropIntentKind::SubmitZoneTarget)
		{
			PreviewState = bReleased && Result.bSubmitted
				? EWacomRunMenuDropTargetPreviewState::Submitted
				: EWacomRunMenuDropTargetPreviewState::SubmitReady;
		}
		else if (Result.IntentKind == EWacomRunMenuCardDropIntentKind::Reject)
		{
			PreviewState = EWacomRunMenuDropTargetPreviewState::Invalid;
		}
		else if (bReleased)
		{
			PreviewState = EWacomRunMenuDropTargetPreviewState::ReleasedProbe;
		}

		NewTarget->SetRunMenuDropPreviewState(PreviewState);
		FeedbackState = Result.IntentKind == EWacomRunMenuCardDropIntentKind::Reject
			? EWacomFirstPersonCardDragTargetFeedbackState::Invalid
			: EWacomFirstPersonCardDragTargetFeedbackState::ZoneProbe;
		FeedbackTargetPosition = ProbePosition;
	}
	else if (bHasActiveDrag)
	{
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
	}

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchorForRunMenuProbe())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			Result.TargetHandle,
			false,
			FeedbackState,
			FeedbackTargetPosition,
			LastRunMenuDropProbeDebugSummary);
	}
	return bReleased
		&& Result.IntentKind == EWacomRunMenuCardDropIntentKind::SubmitZoneTarget
		&& Result.bSubmitted;
}

bool AWacomPlayerController::ApplyRunWorldCardDropProbeFeedback(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	bool bReleased)
{
	const FVector2D ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;

	FWacomInteractionTargetHandle TargetHandle;
	AActor* TargetActor = nullptr;
	UWacomRunWorldInteractionTargetBridgeComponent* TargetBridge = nullptr;
	UWacomRunWorldCardDropReceiverComponent* Receiver = nullptr;
	FString DebugSummary;
	FRunWorldCardInteractionValidation Validation =
		ResolveRunWorldCardDropIntent(
			CardInstanceId,
			DragView,
			TargetHandle,
			TargetActor,
			TargetBridge,
			Receiver,
			DebugSummary);

	bool bSubmitted = false;
	if (bReleased && Validation.bCanSubmit && Receiver)
	{
		bSubmitted = SubmitResolvedRunWorldCardDropIntent(
			CardInstanceId,
			Receiver,
			TargetHandle.StableTargetId,
			Validation);
	}

	FText FailureToastText = FText::GetEmpty();
	FName ToastSource = NAME_None;
	const bool bReleasedOnRunWorldTarget =
		bReleased && TargetHandle.IsValid();
	if (bReleasedOnRunWorldTarget && !bSubmitted)
	{
		const FName FailureReason = Validation.DisabledReason.IsNone()
			? FName(TEXT("SubmitFailed"))
			: Validation.DisabledReason;
		if (Receiver)
		{
			FailureToastText = Receiver->BuildRunWorldCardDropFailureToastText(
				this,
				TargetHandle.StableTargetId,
				CardInstanceId,
				FailureReason);
			ToastSource = TEXT("Receiver");
		}
		else
		{
			FailureToastText = BuildRunWorldCardDropConfigWarningToast(FailureReason);
			ToastSource = TEXT("ControllerFallback");
		}
		if (!FailureToastText.IsEmpty())
		{
			if (UWacomAppToastSubsystem* ToastSubsystem = ResolveAppToastSubsystem())
			{
				ToastSubsystem->ShowWarning(FailureToastText);
			}
		}
	}

	FinalizeRunWorldCardDropDebugSummary(
		DebugSummary,
		ProbePosition,
		CardInstanceId,
		TargetHandle,
		Validation,
		TargetActor,
		Receiver,
		Validation.DisabledReason.IsNone() ? FName(TEXT("Ok")) : Validation.DisabledReason,
		bReleased,
		bSubmitted,
		ToastSource,
		FailureToastText);
	LastRunWorldCardDropDebugSummary = DebugSummary;

	UWacomRunWorldInteractionTargetBridgeComponent* PreviousBridge =
		PreviewedRunWorldCardDropBridge.Get();
	UWacomRunWorldInteractionTargetBridgeComponent* NewBridge =
		TargetBridge && Validation.bCanSubmit ? TargetBridge : nullptr;
	if (PreviousBridge != NewBridge)
	{
		if (PreviousBridge)
		{
			PreviousBridge->ClearProbePreview();
		}
		PreviewedRunWorldCardDropBridge = NewBridge;
		if (NewBridge)
		{
			NewBridge->SetProbePreviewActive(true);
		}
	}

	const bool bHasActiveDrag =
		DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::ArmedForCommit
		|| DragView.GestureState == EWacomFirstPersonCardGestureState::AimingTargetedCard;

	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;
	TOptional<FVector2D> FeedbackTargetPosition;
	if (TargetHandle.IsValid())
	{
		FeedbackState = Validation.bCanSubmit
			? EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
			: EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
		FeedbackTargetPosition = TargetHandle.ScreenPosition;
	}
	else if (bHasActiveDrag)
	{
		FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
		FeedbackTargetPosition = ProbePosition;
	}

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchorForRunMenuProbe())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			TargetHandle,
			Validation.bCanSubmit,
			FeedbackState,
			FeedbackTargetPosition,
			LastRunWorldCardDropDebugSummary);
	}

	if (bLogRunWorldCardDrop)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomRunWorldCardDrop] %s"),
			*LastRunWorldCardDropDebugSummary);
	}

	if (bSubmitted)
	{
		if (UWacomAppToastSubsystem* ToastSubsystem = ResolveAppToastSubsystem())
		{
			const FRunWorldCardInteractionRequest Request = Receiver
				? Receiver->BuildRunWorldCardDropRequest_Implementation(
					TargetHandle.StableTargetId,
					CardInstanceId)
				: FRunWorldCardInteractionRequest();
			for (const FWacomRunWorldCardInteractionReward& Reward : Request.Rewards)
			{
				switch (Reward.Type)
				{
				case EWacomRunWorldCardInteractionRewardType::Gold:
					ToastSubsystem->ShowGoldChanged(Reward.GoldAmount);
					break;
				case EWacomRunWorldCardInteractionRewardType::Card:
					ToastSubsystem->ShowCardGained(Reward.CardDefinition.Get());
					break;
				case EWacomRunWorldCardInteractionRewardType::None:
				default:
					break;
				}
			}
		}
		RefreshRunFirstPersonCardLayer();
	}
	return bSubmitted;
}

FWacomRunMenuCardDropResolveResult AWacomPlayerController::ResolveRunMenuCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView) const
{
	const FVector2D ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;

	FWacomRunMenuCardDropResolveResult Result;
	Result.SourceCardInstanceId = CardInstanceId;
	Result.LeaseId = RunFirstPersonCardSourceComponent
		? RunFirstPersonCardSourceComponent->GetActiveMenuLeaseId()
		: NAME_None;
	Result.LeaseSourceId = RunFirstPersonCardSourceComponent
		? RunFirstPersonCardSourceComponent->GetActiveMenuLeaseSourceId()
		: NAME_None;

	auto RejectWith = [&Result, &ProbePosition](EWacomRunMenuCardDropRejectReason Reason)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = Reason;
		Result.bCanSubmit = false;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	};

	if (!IsInExplorationFlow())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::NotInExploration);
	}

	const bool bHasActiveGameMenu =
		bRunFirstPersonCardLayerTransitionSuppressedByGameMenu
		|| ActiveGameMenuWidgets.ContainsByPredicate(
			[](const TWeakObjectPtr<UWacomMenuWidgetBase>& Menu)
			{
				return Menu.IsValid();
			});
	if (!bHasActiveGameMenu)
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::MissingGameMenu);
	}
	if (!RunFirstPersonCardSourceComponent
		|| !RunFirstPersonCardSourceComponent->HasActiveMenuLease())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::MissingMenuLease);
	}
	if (!CardInstanceId.IsValid())
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::InvalidSourceCard);
	}
	FWacomInteractionTargetHandle TargetHandle;
	const bool bHasZoneTarget =
		TryProbeRunMenuDropTargetAtWidgetPosition(ProbePosition, TargetHandle);
	if (!bHasZoneTarget)
	{
		return RejectWith(EWacomRunMenuCardDropRejectReason::MissingZoneTarget);
	}
	if (TargetHandle.TargetKind != EWacomInteractionTargetKind::Zone)
	{
		Result.TargetHandle = TargetHandle;
		return RejectWith(EWacomRunMenuCardDropRejectReason::UnsupportedTargetKind);
	}

	Result.TargetHandle = TargetHandle;
	Result.ZoneId = TargetHandle.ZoneId;
	Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
	Result.bCanSubmit = false;
	Result.bSubmitted = false;

	UWacomMenuWidgetBase* OwningMenu =
		ResolveOwningMenuForActiveRunMenuLease(Result.LeaseId);
	if (!OwningMenu)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuNotFound;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	Result = OwningMenu->ResolveRunMenuFirstPersonCardDropIntent(Result);

	if (Result.IntentKind != EWacomRunMenuCardDropIntentKind::SubmitZoneTarget
		|| Result.SubmitPolicy != EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard)
	{
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	URunSession* ResolvedRunSession = ResolveRunSessionForFirstPersonCardSource();
	if (!ResolvedRunSession)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MissingSession;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	const FRunDeckOperationValidation Validation =
		ResolvedRunSession->ValidateDestroyCardByInstance(CardInstanceId);
	Result.RunValidationReason = Validation.DisabledReason;
	if (!Validation.bCanExecute)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = Validation.DisabledReason == FName(TEXT("CardNotOwned"))
			? EWacomRunMenuCardDropRejectReason::CardNotOwned
			: EWacomRunMenuCardDropRejectReason::RunValidationFailed;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
		return Result;
	}

	Result.IntentKind = EWacomRunMenuCardDropIntentKind::SubmitZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard;
	Result.bCanSubmit = true;
	FinalizeRunMenuCardDropDebug(Result, ProbePosition, /*bReleased*/ false);
	return Result;
}

FRunWorldCardInteractionValidation AWacomPlayerController::ResolveRunWorldCardDropIntent(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView,
	FWacomInteractionTargetHandle& OutTargetHandle,
	AActor*& OutTargetActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
	UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
	FString& OutDebugSummary) const
{
	OutTargetHandle = FWacomInteractionTargetHandle();
	OutTargetActor = nullptr;
	OutTargetBridge = nullptr;
	OutReceiver = nullptr;

	auto RejectWith = [&](
		FName Reason,
		const FWacomInteractionTargetHandle& TargetHandle =
			FWacomInteractionTargetHandle())
	{
		FRunWorldCardInteractionValidation Result;
		Result.bCanSubmit = false;
		Result.DisabledReason = Reason;
		Result.DebugSummary = FString::Printf(
			TEXT("RunWorldCardInteraction{CanSubmit=false Reason=%s}"),
			*Reason.ToString());
		FinalizeRunWorldCardDropDebugSummary(
			OutDebugSummary,
			DragView.bHasPointerViewportPosition
				? DragView.PointerViewportPosition
				: DragView.CurrentScreenPosition,
			CardInstanceId,
			TargetHandle,
			Result,
			OutTargetActor,
			OutReceiver,
			Reason,
			/*bReleased*/ false,
			/*bSubmitted*/ false);
		return Result;
	};

	if (!bEnableRunWorldCardDrop)
	{
		return RejectWith(TEXT("Disabled"));
	}
	if (!ShouldHandleRunWorldCardDropProbe())
	{
		return RejectWith(TEXT("Blocked"));
	}
	if (!CardInstanceId.IsValid())
	{
		return RejectWith(TEXT("InvalidSourceCard"));
	}

	const FVector2D ProbePosition = DragView.bHasPointerViewportPosition
		? DragView.PointerViewportPosition
		: DragView.CurrentScreenPosition;
	if (!TryProbeRunSceneInteractionTargetAtWidgetPosition(ProbePosition, OutTargetHandle))
	{
		return RejectWith(TEXT("MissingRunWorldTarget"));
	}

	FName ResolveRejectReason = NAME_None;
	if (!ResolveRunWorldClickableInteractableFromHandle(
		OutTargetHandle,
		OutTargetActor,
		OutTargetBridge,
		ResolveRejectReason))
	{
		return RejectWith(ResolveRejectReason, OutTargetHandle);
	}

	OutReceiver = ResolveRunWorldCardDropReceiverFromHandle(OutTargetHandle);
	if (!OutReceiver)
	{
		return RejectWith(TEXT("MissingCardDropReceiver"), OutTargetHandle);
	}

	FRunWorldCardInteractionValidation Validation =
		OutReceiver->ValidateRunWorldCardDrop_Implementation(
			const_cast<AWacomPlayerController*>(this),
			OutTargetHandle.StableTargetId,
			CardInstanceId);
	FinalizeRunWorldCardDropDebugSummary(
		OutDebugSummary,
		ProbePosition,
		CardInstanceId,
		OutTargetHandle,
		Validation,
		OutTargetActor,
		OutReceiver,
		Validation.DisabledReason.IsNone()
			? FName(TEXT("Ok"))
			: Validation.DisabledReason,
		/*bReleased*/ false,
		/*bSubmitted*/ false);
	return Validation;
}

bool AWacomPlayerController::SubmitResolvedRunMenuCardDropIntent(
	FWacomRunMenuCardDropResolveResult& Result)
{
	if (Result.IntentKind != EWacomRunMenuCardDropIntentKind::SubmitZoneTarget
		|| !Result.bCanSubmit)
	{
		Result.bSubmitted = false;
		return false;
	}

	if (Result.SubmitPolicy == EWacomRunMenuCardDropSubmitPolicy::MenuHandled)
	{
		UWacomMenuWidgetBase* OwningMenu =
			ResolveOwningMenuForActiveRunMenuLease(Result.LeaseId);
		if (!OwningMenu)
		{
			Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
			Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuNotFound;
			Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
			Result.bCanSubmit = false;
			Result.bSubmitted = false;
			return false;
		}

		FWacomRunMenuCardDropResolveResult SubmittedResult = Result;
		const bool bSubmitted =
			OwningMenu->SubmitRunMenuFirstPersonCardDropIntent(Result, SubmittedResult);
		Result = SubmittedResult;
		Result.bSubmitted = bSubmitted && Result.bSubmitted;
		if (!Result.bSubmitted)
		{
			Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
			if (Result.RejectReason == EWacomRunMenuCardDropRejectReason::None)
			{
				Result.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
			}
			Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
			Result.bCanSubmit = false;
		}
		return Result.bSubmitted;
	}

	if (Result.SubmitPolicy != EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard
		|| !ResolveRunSessionForFirstPersonCardSource())
	{
		Result.bSubmitted = false;
		return false;
	}

	URunSession* ResolvedRunSession = ResolveRunSessionForFirstPersonCardSource();
	const bool bDestroyed = ResolvedRunSession->DestroyCardByInstance(Result.SourceCardInstanceId);
	Result.bSubmitted = bDestroyed;
	if (!bDestroyed)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
	}
	return bDestroyed;
}

bool AWacomPlayerController::SubmitResolvedRunWorldCardDropIntent(
	const FGuid& CardInstanceId,
	UWacomRunWorldCardDropReceiverComponent* Receiver,
	FName PersistentId,
	FRunWorldCardInteractionValidation& InOutValidation)
{
	if (!Receiver || PersistentId.IsNone() || !CardInstanceId.IsValid())
	{
		InOutValidation.bCanSubmit = false;
		InOutValidation.DisabledReason = TEXT("InvalidSubmitContext");
		return false;
	}

	const bool bSubmitted = Receiver->SubmitRunWorldCardDrop_Implementation(
		this,
		PersistentId,
		CardInstanceId,
		InOutValidation);
	if (!bSubmitted && InOutValidation.DisabledReason.IsNone())
	{
		InOutValidation.DisabledReason = TEXT("SubmitFailed");
	}
	return bSubmitted;
}

UWacomAppToastSubsystem* AWacomPlayerController::ResolveAppToastSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance
		? GameInstance->GetSubsystem<UWacomAppToastSubsystem>()
		: nullptr;
}

UWacomRunWorldInteractionTargetBridgeComponent*
AWacomPlayerController::ResolveRunWorldTargetBridgeFromHandle(
	const FWacomInteractionTargetHandle& Handle) const
{
	UObject* SourceObject = Handle.SourceObject.Get();
	if (!SourceObject)
	{
		return nullptr;
	}

	if (UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		Cast<UWacomRunWorldInteractionTargetBridgeComponent>(SourceObject))
	{
		return Bridge;
	}

	if (const UActorComponent* SourceComponent = Cast<UActorComponent>(SourceObject))
	{
		AActor* SourceOwner = SourceComponent->GetOwner();
		return SourceOwner ? SourceOwner->FindComponentByClass<UWacomRunWorldInteractionTargetBridgeComponent>() : nullptr;
	}

	if (const AActor* SourceActor = Cast<AActor>(SourceObject))
	{
		return SourceActor->FindComponentByClass<UWacomRunWorldInteractionTargetBridgeComponent>();
	}

	return nullptr;
}

UWacomRunWorldCardDropReceiverComponent*
AWacomPlayerController::ResolveRunWorldCardDropReceiverFromHandle(
	const FWacomInteractionTargetHandle& Handle) const
{
	AActor* SourceActor = ResolveSourceActorFromInteractionTargetHandle(Handle);
	if (!SourceActor)
	{
		return nullptr;
	}
	return SourceActor->FindComponentByClass<UWacomRunWorldCardDropReceiverComponent>();
}

void AWacomPlayerController::ClearRunWorldCardDropProbe()
{
	if (UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		PreviewedRunWorldCardDropBridge.Get())
	{
		Bridge->ClearProbePreview();
	}
	PreviewedRunWorldCardDropBridge.Reset();
	LastRunWorldCardDropDebugSummary = TEXT("RunWorldCardDrop{State=Cleared}");

	if (UWacomFirstPersonCardAnchorComponent* Anchor = ResolveFirstPersonCardAnchorForRunMenuProbe())
	{
		Anchor->SetFirstPersonCardDragFeedbackTarget(
			FWacomInteractionTargetHandle(),
			false,
			EWacomFirstPersonCardDragTargetFeedbackState::None,
			TOptional<FVector2D>(),
			LastRunWorldCardDropDebugSummary);
	}
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
	if (HoveredRunWorldInteractableActor.IsValid()
		&& !HoveredRunWorldInteractablePrompt.IsEmpty())
	{
		return HoveredRunWorldInteractablePrompt;
	}

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
	if (!IsInExplorationFlow())
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

void AWacomPlayerController::OpenRunMenuCardLeaseTestMenu()
{
	AWacomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AWacomGameMode>() : nullptr;
	if (!GM || GM->GetGameFlowState() != EGameFlowState::Exploration)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Wacom.OpenRunMenuCardLeaseTestMenu] 当前不在 Exploration，忽略"));
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Wacom.OpenRunMenuCardLeaseTestMenu] UIManager 未就位"));
		return;
	}

	UIManager->EnsurePrimaryLayout(this);
	UWacomPrimaryGameLayout* Layout = UIManager->GetPrimaryLayout();
	if (!Layout)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Wacom.OpenRunMenuCardLeaseTestMenu] PrimaryLayout 未就位"));
		return;
	}

	UCommonActivatableWidgetStack* MenuStack = Layout->GetLayerStack(
		WacomUITags::UI_Layer_GameMenu.GetTag());
	if (!MenuStack)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Wacom.OpenRunMenuCardLeaseTestMenu] GameMenu layer 未就位"));
		return;
	}

	if (UCommonActivatableWidget* ActiveWidget = MenuStack->GetActiveWidget())
	{
		ActiveWidget->DeactivateWidget();
	}

	SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
	UCommonActivatableWidget* PushedWidget = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		UWacomRunMenuCardLeaseTestMenu::StaticClass());
	if (PushedWidget)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[Wacom.OpenRunMenuCardLeaseTestMenu] 打开 C++ lease provider 验证菜单"));
	}
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

static FAutoConsoleCommandWithWorld GWacomOpenRunMenuCardLeaseTestMenuCmd(
	TEXT("Wacom.OpenRunMenuCardLeaseTestMenu"),
	TEXT("打开 C++ Run menu card lease provider 验证菜单。"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AWacomPlayerController* WPC = FindLocalWacomPC(World))
		{
			WPC->OpenRunMenuCardLeaseTestMenu();
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Wacom.OpenRunMenuCardLeaseTestMenu] 找不到 AWacomPlayerController"));
		}
	}));

#undef LOCTEXT_NAMESPACE

