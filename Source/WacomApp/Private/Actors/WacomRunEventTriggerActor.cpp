// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunEventTriggerActor.h"

#define LOCTEXT_NAMESPACE "WacomRunEventTriggerActor"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#include "Events/RunEventDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunState.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

AWacomRunEventTriggerActor::AWacomRunEventTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	RootComponent = TriggerSphere;

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	ClickBounds->SetupAttachment(RootComponent);
	ClickBounds->SetBoxExtent(FVector(80.f, 80.f, 100.f));
	FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);

	ClickInteractionTargetComponent =
		CreateDefaultSubobject<UWacomInteractionTargetComponent>(TEXT("ClickInteractionTarget"));

	ClickTargetBridgeComponent =
		CreateDefaultSubobject<UWacomRunWorldInteractionTargetBridgeComponent>(TEXT("ClickTargetBridge"));
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);

	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 查看事件");
	CompletedPromptText = LOCTEXT("DefaultCompletedPrompt", "事件已完成");
	HoverPromptText = LOCTEXT("DefaultHoverPrompt", "点击查看事件");
	CompletedHoverPromptText = LOCTEXT("DefaultCompletedHoverPrompt", "事件已完成");
	CompletedToastText = LOCTEXT("DefaultCompletedToast", "该事件已完成");
}

void AWacomRunEventTriggerActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshClickTargetBinding();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}

	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunEventTriggerActor] %s: PersistentId 未配置，事件不会打开"),
			*GetName());
	}
	else if (HasDuplicatePersistentIdInWorld())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunEventTriggerActor] %s: PersistentId %s 与同关卡其他事件重复；两个事件会共享同一份状态"),
			*GetName(),
			*PersistentId.ToString());
	}

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &AWacomRunEventTriggerActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &AWacomRunEventTriggerActor::HandleEndOverlap);
	}
}

void AWacomRunEventTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
}

void AWacomRunEventTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			WacomPC->UnregisterCandidateInteractable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AWacomRunEventTriggerActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	PC->RegisterCandidateInteractable(this);
}

void AWacomRunEventTriggerActor::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	PC->UnregisterCandidateInteractable(this);
}

void AWacomRunEventTriggerActor::ConfigureDebugSnakeGiftSample()
{
	ConfigureDebugSample(
		TEXT("Event.DebugSnakeGift.Actor"),
		TEXT("/Game/Wacom/Data/Events/DA_Event_DebugSnakeGift.DA_Event_DebugSnakeGift"));
}

void AWacomRunEventTriggerActor::ConfigureDebugFlagRewardSample()
{
	ConfigureDebugSample(
		TEXT("Event.DebugFlagReward.Actor"),
		TEXT("/Game/Wacom/Data/Events/DA_Event_DebugFlagReward.DA_Event_DebugFlagReward"));
}

FWacomRunEventTriggerDebugView AWacomRunEventTriggerActor::GetRunEventTriggerDebugView(
	AWacomPlayerController* PC) const
{
	FWacomRunEventTriggerDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.EventDefinitionName = EventDefinition ? EventDefinition->GetName() : TEXT("None");
	View.EventId = EventDefinition ? EventDefinition->EventId : NAME_None;
	View.StartNodeId = EventDefinition ? EventDefinition->StartNodeId : NAME_None;
	View.bCanInteract = CanInteract_Implementation(PC);
	View.bDuplicatePersistentIdDetected = HasDuplicatePersistentIdInWorld();
	View.HoverPrompt = GetHoverPromptText(PC).ToString();
	View.CompletedHoverPrompt = CompletedHoverPromptText.IsEmpty()
		? FString(TEXT("事件已完成"))
		: CompletedHoverPromptText.ToString();
	{
		const FWacomRunWorldClickableInteractableDebugView ClickDebug =
			GetRunWorldClickableDebugView_Implementation(PC);
		View.bClickTargetConfigured = ClickDebug.bClickTargetConfigured;
		View.ClickTargetStableId = ClickDebug.ClickTargetStableId;
	}

	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	View.bHasRunSession = Run != nullptr;
	if (!PC)
	{
		View.LastDebugResult = TEXT("MissingPlayerController");
		return View;
	}
	if (PersistentId.IsNone())
	{
		View.LastDebugResult = TEXT("MissingPersistentId");
		return View;
	}
	if (!EventDefinition)
	{
		View.LastDebugResult = TEXT("MissingEventDefinition");
		return View;
	}
	if (!Run)
	{
		View.LastDebugResult = TEXT("MissingRunSession");
		return View;
	}

	const FRunState& State = Run->GetRunState();
	View.bIsActiveEvent = State.ActiveRunEventId == PersistentId;
	View.bIsCompleted = Run->IsRunEventCompleted(PersistentId);
	if (const FRunEventState* EventState = State.RunEventStates.Find(PersistentId))
	{
		View.CurrentNodeId = EventState->CurrentNodeId;
	}
	else
	{
		View.CurrentNodeId = EventDefinition->StartNodeId;
	}
	View.LastDebugResult = View.bDuplicatePersistentIdDetected
		? FName(TEXT("DuplicatePersistentId"))
		: FName(TEXT("Ok"));

	return View;
}

FString AWacomRunEventTriggerActor::GetRunEventTriggerDebugSummary(AWacomPlayerController* PC) const
{
	const FWacomRunEventTriggerDebugView View = GetRunEventTriggerDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunEventTrigger{Actor=%s PersistentId=%s EventDef=%s EventId=%s StartNode=%s HasRun=%s CanInteract=%s Active=%s Completed=%s CurrentNode=%s Duplicate=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CompletedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.EventDefinitionName,
		*View.EventId.ToString(),
		*View.StartNodeId.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsActiveEvent ? TEXT("true") : TEXT("false"),
		View.bIsCompleted ? TEXT("true") : TEXT("false"),
		*View.CurrentNodeId.ToString(),
		View.bDuplicatePersistentIdDetected ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.CompletedHoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
}

void AWacomRunEventTriggerActor::LogRunEventTriggerDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[RunEventTriggerActor] %s"),
		*GetRunEventTriggerDebugSummary(PC));
}

FText AWacomRunEventTriggerActor::GetInteractPromptText_Implementation(AWacomPlayerController* PC) const
{
	if (IsEventCompletedFor(PC))
	{
		return CompletedPromptText.IsEmpty()
			? LOCTEXT("FallbackCompletedPrompt", "事件已完成")
			: CompletedPromptText;
	}
	return InteractPromptText.IsEmpty()
		? LOCTEXT("FallbackInteractPrompt", "按 E 查看事件")
		: InteractPromptText;
}

FText AWacomRunEventTriggerActor::GetHoverPromptText(AWacomPlayerController* PC) const
{
	if (IsEventCompletedFor(PC))
	{
		return CompletedHoverPromptText.IsEmpty()
			? LOCTEXT("FallbackCompletedHoverPrompt", "事件已完成")
			: CompletedHoverPromptText;
	}
	return HoverPromptText.IsEmpty()
		? LOCTEXT("FallbackHoverPrompt", "点击查看事件")
		: HoverPromptText;
}

FText AWacomRunEventTriggerActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	return GetHoverPromptText(PC);
}

FWacomRunWorldClickableInteractableDebugView
AWacomRunEventTriggerActor::GetRunWorldClickableDebugView_Implementation(
	AWacomPlayerController* PC) const
{
	FName LastResult = TEXT("Ok");
	if (!PC)
	{
		LastResult = TEXT("MissingPlayerController");
	}
	else if (PersistentId.IsNone())
	{
		LastResult = TEXT("MissingPersistentId");
	}
	else if (!EventDefinition)
	{
		LastResult = TEXT("MissingEventDefinition");
	}
	else if (!PC->GetRunSession())
	{
		LastResult = TEXT("MissingRunSession");
	}
	else if (HasDuplicatePersistentIdInWorld())
	{
		LastResult = TEXT("DuplicatePersistentId");
	}

	return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
		this,
		PersistentId,
		GetHoverPromptText(PC),
		CanInteract_Implementation(PC),
		/*bHasCompletionState*/true,
		IsEventCompletedFor(PC),
		LastResult,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent,
		ClickBounds);
}

FVector AWacomRunEventTriggerActor::GetInteractLocation_Implementation(AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool AWacomRunEventTriggerActor::CanInteract_Implementation(AWacomPlayerController* PC) const
{
	if (!PC || PersistentId.IsNone() || !EventDefinition)
	{
		return false;
	}
	return true;
}

bool AWacomRunEventTriggerActor::TryInteract_Implementation(AWacomPlayerController* PC)
{
	if (!CanInteract_Implementation(PC))
	{
		return false;
	}
	if (IsEventCompletedFor(PC))
	{
		ShowCompletedToast(PC);
		return false;
	}
	return PC->RequestOpenRunEvent(PersistentId, EventDefinition);
}

bool AWacomRunEventTriggerActor::ConfigureDebugSample(
	FName InPersistentId,
	const TCHAR* EventDefinitionObjectPath)
{
	UWacomRunEventDefinition* LoadedDefinition =
		LoadObject<UWacomRunEventDefinition>(nullptr, EventDefinitionObjectPath);
	if (!LoadedDefinition)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunEventTriggerActor] %s: 无法加载调试事件资产 %s，保持当前配置"),
			*GetName(),
			EventDefinitionObjectPath);
		return false;
	}

	Modify();
	PersistentId = InPersistentId;
	EventDefinition = LoadedDefinition;
	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 查看事件");
	CompletedPromptText = LOCTEXT("DefaultCompletedPrompt", "事件已完成");
	HoverPromptText = LOCTEXT("DefaultHoverPrompt", "点击查看事件");
	CompletedHoverPromptText = LOCTEXT("DefaultCompletedHoverPrompt", "事件已完成");
	CompletedToastText = LOCTEXT("DefaultCompletedToast", "该事件已完成");
	RefreshClickTargetBinding();
	return true;
}

void AWacomRunEventTriggerActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

bool AWacomRunEventTriggerActor::HasDuplicatePersistentIdInWorld() const
{
	if (PersistentId.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<AWacomRunEventTriggerActor> It(GetWorld()); It; ++It)
	{
		const AWacomRunEventTriggerActor* Other = *It;
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

bool AWacomRunEventTriggerActor::IsEventCompletedFor(AWacomPlayerController* PC) const
{
	if (!PC || PersistentId.IsNone())
	{
		return false;
	}
	if (URunSession* Run = PC->GetRunSession())
	{
		return Run->IsRunEventCompleted(PersistentId);
	}
	return false;
}

void AWacomRunEventTriggerActor::ShowCompletedToast(AWacomPlayerController* PC) const
{
	if (!PC)
	{
		return;
	}
	if (UGameInstance* GI = PC->GetGameInstance())
	{
		if (UWacomAppToastSubsystem* ToastSubsystem = GI->GetSubsystem<UWacomAppToastSubsystem>())
		{
			ToastSubsystem->ShowWarning(CompletedToastText.IsEmpty()
				? LOCTEXT("FallbackCompletedToast", "该事件已完成")
				: CompletedToastText);
		}
	}
}

#undef LOCTEXT_NAMESPACE
