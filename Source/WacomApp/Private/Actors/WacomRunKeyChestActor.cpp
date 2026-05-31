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
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

#define LOCTEXT_NAMESPACE "WacomRunKeyChestActor"

namespace
{
	const TCHAR* DebugKeyCardPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey");
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
	ClickBounds->SetBoxExtent(FVector(85.f, 65.f, 55.f));
	ClickBounds->bEditableWhenInherited = false;
	FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);

	ChestVisual = CreateDefaultSubobject<UWacomRunKeyChestVisualComponent>(TEXT("ChestVisual"));
	ChestVisual->SetupAttachment(RootComponent);
	ChestVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChestVisual->SetGenerateOverlapEvents(false);
	ChestVisual->SetRelativeScale3D(FVector(0.75f, 0.55f, 0.45f));
	ChestVisual->bEditableWhenInherited = false;
	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		ChestVisual->SetStaticMesh(CubeMesh);
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
	TriggerRadius = 180.f;
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
	return IsCompletedFor(PC)
		? (CompletedPromptText.IsEmpty() ? GetDefaultCompletedPromptText() : CompletedPromptText)
		: (InteractPromptText.IsEmpty() ? GetDefaultInteractPromptText() : InteractPromptText);
}

FText AWacomRunKeyChestActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
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
	ShowChestHintToast(PC);
	return false;
}

FWacomRunWorldClickableInteractableDebugView
AWacomRunKeyChestActor::GetRunWorldClickableDebugView_Implementation(
	AWacomPlayerController* PC) const
{
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
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);

	FWacomRunKeyChestDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.bHasRunSession = PC && PC->GetRunSession();
	View.bCompleted = IsCompletedFor(PC);
	View.bCanInteract = CanInteract_Implementation(PC);
	View.bClickTargetConfigured = ClickDebug.bClickTargetConfigured;
	View.ClickStableId = ClickDebug.ClickTargetStableId;
	View.bHasCardDropReceiver = CardDropReceiverComponent != nullptr;
	View.InteractPrompt = GetInteractPromptText_Implementation(PC).ToString();
	View.HoverPrompt = GetHoverPromptText(PC).ToString();
	View.CompletedPrompt = (CompletedPromptText.IsEmpty()
		? GetDefaultCompletedPromptText()
		: CompletedPromptText).ToString();
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
		TEXT("RunKeyChest{Actor=%s PersistentId=%s HasRun=%s Completed=%s CanInteract=%s ClickTarget=%s ClickStableId=%s Receiver=%s ReceiverCanSubmit=%s ReceiverReject=%s AllowedDefs=%d AllowedIds=%d Consume=%s Gold=%d InteractPrompt=%s HoverPrompt=%s CompletedPrompt=%s Last=%s ClickDebug=%s ReceiverDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCompleted ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickStableId.ToString(),
		View.bHasCardDropReceiver ? TEXT("true") : TEXT("false"),
		View.bReceiverCanSubmit ? TEXT("true") : TEXT("false"),
		*View.ReceiverRejectReason.ToString(),
		View.ReceiverAllowedDefinitionCount,
		View.ReceiverAllowedCardIdCount,
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
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
	}
	if (ClickBounds)
	{
		FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);
	}
	RefreshClickTargetBinding();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}
}

void AWacomRunKeyChestActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ChestVisual ? Cast<UPrimitiveComponent>(ChestVisual) : Cast<UPrimitiveComponent>(ClickBounds),
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

bool AWacomRunKeyChestActor::IsCompletedFor(AWacomPlayerController* PC) const
{
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	return Run && Run->IsRunWorldInteractionCompleted(PersistentId);
}

FText AWacomRunKeyChestActor::GetHoverPromptText(AWacomPlayerController* PC) const
{
	if (IsCompletedFor(PC))
	{
		return CompletedPromptText.IsEmpty()
			? GetDefaultCompletedPromptText()
			: CompletedPromptText;
	}
	return HoverPromptText.IsEmpty()
		? GetDefaultHoverPromptText()
		: HoverPromptText;
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
		? (CompletedPromptText.IsEmpty() ? GetDefaultCompletedPromptText() : CompletedPromptText)
		: (InteractPromptText.IsEmpty() ? GetDefaultInteractPromptText() : InteractPromptText));
}

#undef LOCTEXT_NAMESPACE
