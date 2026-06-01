// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunKeyChestActor.h"

#include "Cards/CardDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomRunWorldCardDropReceiver.h"
#include "KeyChests/RunKeyChestDefinition.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "WacomRunKeyChestActor"

namespace
{
	const TCHAR* DefaultChestMeshPath =
		TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* DebugKeyCardPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey");

	bool ShouldValidateKeyChestPlacementActor(const AWacomRunKeyChestActor& Chest)
	{
		return !Chest.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !Chest.IsTemplate();
	}
}

AWacomRunKeyChestActor::AWacomRunKeyChestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<UWacomRunKeyChestTriggerSphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	TriggerSphere->bEditableWhenInherited = false;
	RootComponent = TriggerSphere;

	ClickBounds = CreateDefaultSubobject<UWacomRunKeyChestClickBoundsComponent>(TEXT("ClickBounds"));
	ClickBounds->SetupAttachment(RootComponent);
	ClickBounds->SetBoxExtent(ClickBoundsExtent);
	ClickBounds->bEditableWhenInherited = false;
	FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);

	ChestVisual = CreateDefaultSubobject<UWacomRunKeyChestVisualComponent>(TEXT("ChestVisual"));
	ChestVisual->SetupAttachment(RootComponent);
	ChestVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChestVisual->SetGenerateOverlapEvents(false);
	ChestVisual->SetRelativeScale3D(VisualScale);
	ChestVisual->SetRelativeLocation(VisualRelativeLocation);
	ChestVisual->bEditableWhenInherited = false;
	VisualMesh = LoadObject<UStaticMesh>(nullptr, DefaultChestMeshPath);
	if (VisualMesh)
	{
		ChestVisual->SetStaticMesh(VisualMesh);
	}

	ClickInteractionTargetComponent =
		CreateDefaultSubobject<UWacomInteractionTargetComponent>(TEXT("ClickInteractionTarget"));
	ClickInteractionTargetComponent->bEditableWhenInherited = false;
	ClickTargetBridgeComponent =
		CreateDefaultSubobject<UWacomRunWorldInteractionTargetBridgeComponent>(TEXT("ClickTargetBridge"));
	ClickTargetBridgeComponent->bEditableWhenInherited = false;
	CardDropReceiverComponent =
		CreateDefaultSubobject<UWacomRunWorldCardDropReceiverComponent>(TEXT("CardDropReceiver"));
	CardDropReceiverComponent->bEditableWhenInherited = false;

	InteractPromptText = GetDefaultInteractPromptText();
	HoverPromptText = GetDefaultHoverPromptText();
	CompletedPromptText = GetDefaultCompletedPromptText();
	RefreshClickTargetBinding();
}

void AWacomRunKeyChestActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshAuthoringState();
	TryBindRunSessionFromWorld();
	const FName ConfigReason = BuildConfigWarningReason();
	if (!ConfigReason.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomRunKeyChestActor] %s: 配置无效 Reason=%s，拖卡开箱不会提交"),
			*GetName(),
			*ConfigReason.ToString());
	}
	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomRunKeyChestActor] %s: PersistentId %s 与同关卡其他 KeyChest 重复；这些宝箱会共享同一份完成状态"),
			*GetName(),
			*PersistentId.ToString());
	}
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}
	if (TriggerSphere)
	{
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&AWacomRunKeyChestActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this,
			&AWacomRunKeyChestActor::HandleEndOverlap);
	}
}

void AWacomRunKeyChestActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshAuthoringState();
}

void AWacomRunKeyChestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindRunSession();
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			WacomPC->UnregisterCandidateInteractable(this);
		}
	}
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->ClearProbePreview();
	}
	Super::EndPlay(EndPlayReason);
}

void AWacomRunKeyChestActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	AWacomPlayerController* PC = Pawn ? Cast<AWacomPlayerController>(Pawn->GetController()) : nullptr;
	if (PC)
	{
		EnsureRunSessionBinding(PC);
		PC->RegisterCandidateInteractable(this);
	}
}

void AWacomRunKeyChestActor::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	AWacomPlayerController* PC = Pawn ? Cast<AWacomPlayerController>(Pawn->GetController()) : nullptr;
	if (PC)
	{
		PC->UnregisterCandidateInteractable(this);
	}
}

void AWacomRunKeyChestActor::ConfigureDebugKeyChestSample()
{
	Modify();
	PersistentId = BuildDebugKeyChestPersistentIdFromActorName(GetName());
	ChestDefinition = nullptr;
	TriggerRadius = 180.f;
	ClickBoundsExtent = FVector(85.f, 65.f, 55.f);
	VisualMesh = LoadObject<UStaticMesh>(nullptr, DefaultChestMeshPath);
	VisualScale = FVector(0.75f, 0.55f, 0.45f);
	VisualRelativeLocation = FVector::ZeroVector;
	CompletedVisualMesh = nullptr;
	CompletedVisualScale = FVector(0.75f, 0.55f, 0.18f);
	CompletedVisualRelativeLocation = FVector(0.f, 0.f, -18.f);
	InteractPromptText = GetDefaultInteractPromptText();
	HoverPromptText = GetDefaultHoverPromptText();
	CompletedPromptText = GetDefaultCompletedPromptText();

	if (CardDropReceiverComponent)
	{
		CardDropReceiverComponent->Modify();
		CardDropReceiverComponent->AllowedCardDefinitions.Reset();
		if (UCardDefinition* DebugKeyCard = LoadObject<UCardDefinition>(nullptr, DebugKeyCardPath))
		{
			CardDropReceiverComponent->AllowedCardDefinitions.Add(DebugKeyCard);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomRunKeyChestActor] %s: 无法加载调试钥匙卡 %s，保留 CardId 筛选"),
				*GetName(),
				DebugKeyCardPath);
		}
		CardDropReceiverComponent->AllowedCardIds = { TEXT("DebugKey") };
		CardDropReceiverComponent->RequiredKeywords.Reset();
		CardDropReceiverComponent->BlockedKeywords.Reset();
		CardDropReceiverComponent->GoldReward = 3;
		CardDropReceiverComponent->bConsumeCardOnSuccess = true;
		CardDropReceiverComponent->PreviewPromptText = LOCTEXT("DebugKeyPreviewPrompt", "使用钥匙打开宝箱");
		CardDropReceiverComponent->SuccessPromptText = LOCTEXT("DebugKeySuccessPrompt", "宝箱已打开");
		CardDropReceiverComponent->CompletedPromptText = GetDefaultCompletedPromptText();
		CardDropReceiverComponent->RejectedCardPromptText = GetDefaultInteractPromptText();
	}

	RefreshAuthoringState();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}
}

FText AWacomRunKeyChestActor::GetInteractPromptText_Implementation(
	AWacomPlayerController* PC) const
{
	const_cast<AWacomRunKeyChestActor*>(this)->EnsureRunSessionBinding(PC);
	return IsCompletedFor(PC)
		? ResolveCompletedPromptText()
		: ResolveInteractPromptText();
}

FText AWacomRunKeyChestActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	const_cast<AWacomRunKeyChestActor*>(this)->EnsureRunSessionBinding(PC);
	return GetHoverPromptText(PC);
}

FVector AWacomRunKeyChestActor::GetInteractLocation_Implementation(
	AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool AWacomRunKeyChestActor::CanInteract_Implementation(
	AWacomPlayerController* /*PC*/) const
{
	return !PersistentId.IsNone();
}

bool AWacomRunKeyChestActor::TryInteract_Implementation(AWacomPlayerController* PC)
{
	EnsureRunSessionBinding(PC);
	ShowChestHintToast(PC);
	return false;
}

FWacomRunWorldClickableInteractableDebugView
AWacomRunKeyChestActor::GetRunWorldClickableDebugView_Implementation(
	AWacomPlayerController* PC) const
{
	const_cast<AWacomRunKeyChestActor*>(this)->EnsureRunSessionBinding(PC);
	FName LastResult = TEXT("Ok");
	if (!PC)
	{
		LastResult = TEXT("MissingPlayerController");
	}
	else if (!PC->GetRunSession())
	{
		LastResult = TEXT("MissingRunSession");
	}
	else if (PersistentId.IsNone())
	{
		LastResult = TEXT("MissingPersistentId");
	}
	else if (IsCompletedFor(PC))
	{
		LastResult = TEXT("Completed");
	}
	else
	{
		LastResult = TEXT("NeedsCardDrop");
	}

	return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
		this,
		PersistentId,
		GetHoverPromptText(PC),
		CanInteract_Implementation(PC),
		/*bHasCompletionState*/true,
		IsCompletedFor(PC),
		LastResult,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent,
		ClickBounds);
}

FWacomRunKeyChestDebugView AWacomRunKeyChestActor::GetRunKeyChestDebugView(
	AWacomPlayerController* PC) const
{
	const_cast<AWacomRunKeyChestActor*>(this)->EnsureRunSessionBinding(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);

	FWacomRunKeyChestDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.DefinitionName = ChestDefinition ? ChestDefinition->GetFName() : NAME_None;
	View.ChestId = ChestDefinition ? ChestDefinition->ChestId : NAME_None;
	View.DefinitionConfigWarningReason = ChestDefinition
		? ChestDefinition->GetConfigWarningReason()
		: NAME_None;
	View.bHasRunSession = PC && PC->GetRunSession();
	View.bCompleted = IsCompletedFor(PC);
	View.bCanInteract = CanInteract_Implementation(PC);
	View.ConfigWarningReason = BuildConfigWarningReason();
	View.bConfigValid = View.ConfigWarningReason.IsNone();
	View.bDuplicatePersistentIdDetected = HasDuplicatePersistentIdInWorld();
	View.bClickTargetConfigured = ClickDebug.bClickTargetConfigured;
	View.ClickStableId = ClickDebug.ClickTargetStableId;
	View.TriggerRadius = TriggerRadius;
	View.ClickBoundsExtent = ClickBounds
		? ClickBounds->GetUnscaledBoxExtent()
		: ClickBoundsExtent;
	View.VisualName = ChestVisual ? ChestVisual->GetFName() : NAME_None;
	View.VisualMeshName = VisualMesh ? VisualMesh->GetFName() : NAME_None;
	View.VisualScale = ChestVisual ? ChestVisual->GetRelativeScale3D() : VisualScale;
	View.CompletedVisualMeshName = CompletedVisualMesh
		? CompletedVisualMesh->GetFName()
		: (VisualMesh ? VisualMesh->GetFName() : NAME_None);
	View.CompletedVisualScale = CompletedVisualScale;
	View.CompletedVisualRelativeLocation = CompletedVisualRelativeLocation;
	View.VisualState = View.bCompleted ? TEXT("Open") : TEXT("Closed");
	View.bHasCardDropReceiver = CardDropReceiverComponent != nullptr;
	View.InteractPrompt = GetInteractPromptText_Implementation(PC).ToString();
	View.HoverPrompt = GetHoverPromptText(PC).ToString();
	View.CompletedPrompt = ResolveCompletedPromptText().ToString();
	View.ReceiverDebugSummary = CardDropReceiverComponent
		? CardDropReceiverComponent->GetRunWorldCardDropReceiverDebugSummary(
			PC,
			PersistentId,
			FGuid())
		: TEXT("None");
	View.LastDebugResult = ClickDebug.LastDebugResult;

	if (CardDropReceiverComponent)
	{
		const FWacomRunWorldCardDropReceiverDebugView ReceiverDebug =
			CardDropReceiverComponent->GetRunWorldCardDropReceiverDebugView_Implementation(
				PC,
				PersistentId,
				FGuid());
		View.bReceiverCanSubmit = ReceiverDebug.bCanSubmit;
		View.ReceiverRejectReason = ReceiverDebug.RejectReason;
		View.ReceiverAllowedDefinitionCount = ReceiverDebug.AllowedDefinitionCount;
		View.ReceiverAllowedCardIdCount = ReceiverDebug.AllowedCardIdCount;
		View.ReceiverRequiredKeywordCount = ReceiverDebug.RequiredKeywordCount;
		View.ReceiverBlockedKeywordCount = ReceiverDebug.BlockedKeywordCount;
		View.bReceiverHasPositiveCardFilter = ReceiverDebug.bHasPositiveCardFilter;
		View.bReceiverConsumeCardOnSuccess = ReceiverDebug.bConsumeCardOnSuccess;
		View.ReceiverGoldReward = ReceiverDebug.GoldReward;
	}
	return View;
}

FString AWacomRunKeyChestActor::GetRunKeyChestDebugSummary(
	AWacomPlayerController* PC) const
{
	const FWacomRunKeyChestDebugView View = GetRunKeyChestDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunKeyChest{Actor=%s PersistentId=%s Definition=%s ChestId=%s DefinitionReason=%s HasRun=%s Completed=%s CanInteract=%s ConfigValid=%s ConfigReason=%s Duplicate=%s ClickTarget=%s ClickStableId=%s TriggerRadius=%.1f ClickBoundsExtent=%s VisualName=%s VisualMesh=%s VisualScale=%s CompletedVisualMesh=%s CompletedVisualScale=%s CompletedVisualLocation=%s VisualState=%s Receiver=%s ReceiverCanSubmit=%s ReceiverReject=%s ReceiverAllowedDefs=%d ReceiverAllowedIds=%d RequiredKeywords=%d BlockedKeywords=%d PositiveFilter=%s Consume=%s Gold=%d InteractPrompt=%s HoverPrompt=%s CompletedPrompt=%s Last=%s ClickDebug=%s ReceiverDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.DefinitionName.ToString(),
		*View.ChestId.ToString(),
		*View.DefinitionConfigWarningReason.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCompleted ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bConfigValid ? TEXT("true") : TEXT("false"),
		*View.ConfigWarningReason.ToString(),
		View.bDuplicatePersistentIdDetected ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickStableId.ToString(),
		View.TriggerRadius,
		*View.ClickBoundsExtent.ToCompactString(),
		*View.VisualName.ToString(),
		*View.VisualMeshName.ToString(),
		*View.VisualScale.ToCompactString(),
		*View.CompletedVisualMeshName.ToString(),
		*View.CompletedVisualScale.ToCompactString(),
		*View.CompletedVisualRelativeLocation.ToCompactString(),
		*View.VisualState.ToString(),
		View.bHasCardDropReceiver ? TEXT("true") : TEXT("false"),
		View.bReceiverCanSubmit ? TEXT("true") : TEXT("false"),
		*View.ReceiverRejectReason.ToString(),
		View.ReceiverAllowedDefinitionCount,
		View.ReceiverAllowedCardIdCount,
		View.ReceiverRequiredKeywordCount,
		View.ReceiverBlockedKeywordCount,
		View.bReceiverHasPositiveCardFilter ? TEXT("true") : TEXT("false"),
		View.bReceiverConsumeCardOnSuccess ? TEXT("true") : TEXT("false"),
		View.ReceiverGoldReward,
		*View.InteractPrompt,
		*View.HoverPrompt,
		*View.CompletedPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug),
		*View.ReceiverDebugSummary);
}

void AWacomRunKeyChestActor::LogRunKeyChestDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomRunKeyChestActor] %s"),
		*GetRunKeyChestDebugSummary(PC));
}

void AWacomRunKeyChestActor::RefreshAuthoringState()
{
	SyncReceiverFromDefinition();
	if (CardDropReceiverComponent && !ChestDefinition)
	{
		CardDropReceiverComponent->RejectedCardPromptText = ResolveInteractPromptText();
		CardDropReceiverComponent->CompletedPromptText = ResolveCompletedPromptText();
	}
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
	}
	if (ClickBounds)
	{
		ClickBounds->SetBoxExtent(ClickBoundsExtent);
		FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);
	}
	if (ChestVisual)
	{
		ChestVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ChestVisual->SetGenerateOverlapEvents(false);
	}
	RefreshVisualState();
	RefreshClickTargetBinding();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}
}

void AWacomRunKeyChestActor::RefreshVisualState()
{
	if (!ChestVisual)
	{
		return;
	}

	const bool bCompleted = IsCompletedForBoundRunSession();
	UStaticMesh* MeshToUse = bCompleted && CompletedVisualMesh
		? CompletedVisualMesh.Get()
		: VisualMesh.Get();
	ChestVisual->SetStaticMesh(MeshToUse);
	ChestVisual->SetRelativeScale3D(bCompleted
		? CompletedVisualScale
		: VisualScale);
	ChestVisual->SetRelativeLocation(bCompleted
		? CompletedVisualRelativeLocation
		: VisualRelativeLocation);
	ChestVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChestVisual->SetGenerateOverlapEvents(false);
}

void AWacomRunKeyChestActor::TryBindRunSessionFromWorld()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	EnsureRunSessionBinding(Cast<AWacomPlayerController>(PC));
}

void AWacomRunKeyChestActor::EnsureRunSessionBinding(AWacomPlayerController* PC)
{
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (Run)
	{
		BindRunSessionForCompletedVisual(Run);
	}
	else
	{
		RefreshVisualState();
	}
}

void AWacomRunKeyChestActor::BindRunSessionForCompletedVisual(URunSession* Run)
{
	if (BoundRunSession == Run)
	{
		RefreshVisualState();
		return;
	}

	UnbindRunSession();
	BoundRunSession = Run;
	if (BoundRunSession)
	{
		BoundRunSession->OnRunStateChangedNative.AddUObject(
			this,
			&AWacomRunKeyChestActor::HandleRunStateChanged);
	}
	RefreshVisualState();
}

void AWacomRunKeyChestActor::UnbindRunSession()
{
	if (BoundRunSession)
	{
		BoundRunSession->OnRunStateChangedNative.RemoveAll(this);
	}
	BoundRunSession = nullptr;
}

void AWacomRunKeyChestActor::HandleRunStateChanged()
{
	RefreshVisualState();
}

void AWacomRunKeyChestActor::SyncReceiverFromDefinition()
{
	if (!ChestDefinition || !CardDropReceiverComponent)
	{
		return;
	}

	CardDropReceiverComponent->AllowedCardDefinitions =
		ChestDefinition->AllowedCardDefinitions;
	CardDropReceiverComponent->AllowedCardIds =
		ChestDefinition->AllowedCardIds;
	CardDropReceiverComponent->RequiredKeywords =
		ChestDefinition->RequiredKeywords;
	CardDropReceiverComponent->BlockedKeywords =
		ChestDefinition->BlockedKeywords;
	CardDropReceiverComponent->GoldReward =
		ChestDefinition->GoldReward;
	CardDropReceiverComponent->bConsumeCardOnSuccess =
		ChestDefinition->bConsumeCardOnSuccess;

	CardDropReceiverComponent->PreviewPromptText =
		ChestDefinition->PreviewPromptText;
	CardDropReceiverComponent->SuccessPromptText =
		ChestDefinition->SuccessPromptText;
	CardDropReceiverComponent->CompletedPromptText =
		ChestDefinition->ReceiverCompletedPromptText.IsEmpty()
			? ResolveCompletedPromptText()
			: ChestDefinition->ReceiverCompletedPromptText;
	CardDropReceiverComponent->RejectedCardPromptText =
		ResolveInteractPromptText();
}

#if WITH_EDITOR
EDataValidationResult AWacomRunKeyChestActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidateKeyChestPlacementActor(*this))
	{
		return Result;
	}

	const FName ConfigReason = BuildConfigWarningReason();
	if (!ConfigReason.IsNone())
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementConfigInvalid",
				"KeyChest 摆放配置错误：Actor={0} PersistentId={1} Reason={2} Definition={3} ChestId={4} Receiver={5} AllowedDefs={6} AllowedIds={7} RequiredKeywords={8} BlockedKeywords={9} Gold={10}。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId),
			FText::FromName(ConfigReason),
			FText::FromString(ChestDefinition ? ChestDefinition->GetName() : TEXT("None")),
			FText::FromName(ChestDefinition ? ChestDefinition->ChestId : NAME_None),
			FText::FromString(CardDropReceiverComponent ? CardDropReceiverComponent->GetName() : TEXT("None")),
			FText::AsNumber(CardDropReceiverComponent ? CardDropReceiverComponent->AllowedCardDefinitions.Num() : 0),
			FText::AsNumber(CardDropReceiverComponent ? CardDropReceiverComponent->AllowedCardIds.Num() : 0),
			FText::AsNumber(CardDropReceiverComponent ? CardDropReceiverComponent->RequiredKeywords.Num() : 0),
			FText::AsNumber(CardDropReceiverComponent ? CardDropReceiverComponent->BlockedKeywords.Num() : 0),
			FText::AsNumber(CardDropReceiverComponent ? CardDropReceiverComponent->GoldReward : 0)));
		Result = EDataValidationResult::Invalid;
	}

	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementDuplicatePersistentId",
				"KeyChest 摆放警告：Actor={0} PersistentId={1} 与同关卡其他 KeyChest 重复；这些宝箱会共享同一份 CompletedRunWorldInteractionIds 完成状态。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId)));
		if (Result != EDataValidationResult::Invalid)
		{
			Result = EDataValidationResult::Valid;
		}
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

void AWacomRunKeyChestActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ChestVisual ? Cast<UPrimitiveComponent>(ChestVisual) : Cast<UPrimitiveComponent>(ClickBounds),
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

FName AWacomRunKeyChestActor::BuildConfigWarningReason() const
{
	if (PersistentId.IsNone())
	{
		return TEXT("MissingPersistentId");
	}
	if (!CardDropReceiverComponent)
	{
		return TEXT("MissingCardDropReceiver");
	}
	if (ChestDefinition)
	{
		return ChestDefinition->GetConfigWarningReason();
	}
	const FName ReceiverReason =
		CardDropReceiverComponent->GetRunWorldCardDropReceiverConfigWarningReason();
	if (!ReceiverReason.IsNone())
	{
		return ReceiverReason;
	}
	return NAME_None;
}

bool AWacomRunKeyChestActor::HasDuplicatePersistentIdInWorld() const
{
	if (PersistentId.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<AWacomRunKeyChestActor> It(GetWorld()); It; ++It)
	{
		const AWacomRunKeyChestActor* Other = *It;
		if (Other
			&& Other != this
			&& !Other->HasAnyFlags(RF_ClassDefaultObject)
			&& Other->PersistentId == PersistentId)
		{
			return true;
		}
	}
	return false;
}

bool AWacomRunKeyChestActor::IsCompletedFor(AWacomPlayerController* PC) const
{
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	return Run && Run->IsRunWorldInteractionCompleted(PersistentId);
}

bool AWacomRunKeyChestActor::IsCompletedForBoundRunSession() const
{
	return BoundRunSession
		&& BoundRunSession->IsRunWorldInteractionCompleted(PersistentId);
}

FText AWacomRunKeyChestActor::GetHoverPromptText(AWacomPlayerController* PC) const
{
	if (IsCompletedFor(PC))
	{
		return ResolveCompletedPromptText();
	}
	return ResolveHoverPromptText();
}

FText AWacomRunKeyChestActor::ResolveInteractPromptText() const
{
	if (ChestDefinition && !ChestDefinition->InteractPromptText.IsEmpty())
	{
		return ChestDefinition->InteractPromptText;
	}
	return InteractPromptText.IsEmpty()
		? GetDefaultInteractPromptText()
		: InteractPromptText;
}

FText AWacomRunKeyChestActor::ResolveHoverPromptText() const
{
	if (ChestDefinition && !ChestDefinition->HoverPromptText.IsEmpty())
	{
		return ChestDefinition->HoverPromptText;
	}
	return HoverPromptText.IsEmpty()
		? GetDefaultHoverPromptText()
		: HoverPromptText;
}

FText AWacomRunKeyChestActor::ResolveCompletedPromptText() const
{
	if (ChestDefinition && !ChestDefinition->CompletedPromptText.IsEmpty())
	{
		return ChestDefinition->CompletedPromptText;
	}
	return CompletedPromptText.IsEmpty()
		? GetDefaultCompletedPromptText()
		: CompletedPromptText;
}

FText AWacomRunKeyChestActor::GetDefaultInteractPromptText() const
{
	return LOCTEXT("DefaultInteractPrompt", "需要钥匙");
}

FText AWacomRunKeyChestActor::GetDefaultHoverPromptText() const
{
	return LOCTEXT("DefaultHoverPrompt", "拖入钥匙");
}

FText AWacomRunKeyChestActor::GetDefaultCompletedPromptText() const
{
	return LOCTEXT("DefaultCompletedPrompt", "宝箱已打开");
}

FName AWacomRunKeyChestActor::BuildDebugKeyChestPersistentIdFromActorName(
	const FString& ActorName)
{
	FString Sanitized;
	Sanitized.Reserve(ActorName.Len());
	for (const TCHAR Character : ActorName)
	{
		const bool bAllowed =
			FChar::IsAlnum(Character)
			|| Character == TEXT('_')
			|| Character == TEXT('-');
		Sanitized.AppendChar(bAllowed ? Character : TEXT('_'));
	}
	Sanitized.TrimStartAndEndInline();
	if (Sanitized.IsEmpty())
	{
		Sanitized = TEXT("Chest");
	}
	return FName(*FString::Printf(TEXT("Chest.Debug.%s"), *Sanitized));
}

void AWacomRunKeyChestActor::ShowChestHintToast(AWacomPlayerController* PC) const
{
	UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr;
	UWacomAppToastSubsystem* ToastSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
	if (!ToastSubsystem)
	{
		return;
	}

	ToastSubsystem->ShowWarning(IsCompletedFor(PC)
		? ResolveCompletedPromptText()
		: ResolveInteractPromptText());
}

#undef LOCTEXT_NAMESPACE
