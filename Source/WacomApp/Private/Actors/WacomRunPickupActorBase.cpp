// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunPickupActorBase.h"

#define LOCTEXT_NAMESPACE "WacomRunPickupActorBase"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

AWacomRunPickupActorBase::AWacomRunPickupActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	RootComponent = TriggerSphere;

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	ClickBounds->SetupAttachment(RootComponent);
	ClickBounds->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);

	PickupVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupVisual"));
	PickupVisual->SetupAttachment(RootComponent);
	PickupVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupVisual->SetGenerateOverlapEvents(false);
	PickupVisual->SetRelativeScale3D(FVector(0.35f));
	if (UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		PickupVisual->SetStaticMesh(SphereMesh);
	}

	ClickInteractionTargetComponent =
		CreateDefaultSubobject<UWacomInteractionTargetComponent>(TEXT("ClickInteractionTarget"));

	ClickTargetBridgeComponent =
		CreateDefaultSubobject<UWacomRunWorldInteractionTargetBridgeComponent>(TEXT("ClickTargetBridge"));
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		PickupVisual,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

void AWacomRunPickupActorBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshClickTargetBindingAndRuntimeTarget();

	const FName ConfigReason = BuildConfigWarningReason();
	if (ConfigReason == FName(TEXT("MissingPersistentId")))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunPickupActorBase] %s: PersistentId 未配置，拾取物不会结算"),
			*GetName());
	}
	else if (!ConfigReason.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunPickupActorBase] %s: 配置无效 Reason=%s，拾取物不会结算"),
			*GetName(),
			*ConfigReason.ToString());
	}
	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunPickupActorBase] %s: PersistentId %s 与同关卡其他 Run Pickup 重复；这些拾取物会共享同一份已拾取状态"),
			*GetName(),
			*PersistentId.ToString());
	}

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &AWacomRunPickupActorBase::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &AWacomRunPickupActorBase::HandleEndOverlap);
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			if (IsCollectedFor(WacomPC))
			{
				ApplyCollectedLifecycle(WacomPC);
			}
		}
	}
}

void AWacomRunPickupActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
	}
}

void AWacomRunPickupActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void AWacomRunPickupActorBase::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC)
	{
		return;
	}

	PC->RegisterCandidateInteractable(this);
}

void AWacomRunPickupActorBase::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC)
	{
		return;
	}

	PC->UnregisterCandidateInteractable(this);
}

FText AWacomRunPickupActorBase::GetInteractPromptText_Implementation(
	AWacomPlayerController* /*PC*/) const
{
	return InteractPromptText.IsEmpty()
		? GetDefaultInteractPromptText()
		: InteractPromptText;
}

FText AWacomRunPickupActorBase::GetHoverPromptText(AWacomPlayerController* PC) const
{
	if (IsCollectedFor(PC))
	{
		return CollectedHoverPromptText.IsEmpty()
			? GetDefaultCollectedHoverPromptText()
			: CollectedHoverPromptText;
	}
	return HoverPromptText.IsEmpty()
		? GetDefaultHoverPromptText()
		: HoverPromptText;
}

FText AWacomRunPickupActorBase::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	return GetHoverPromptText(PC);
}

FVector AWacomRunPickupActorBase::GetInteractLocation_Implementation(
	AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool AWacomRunPickupActorBase::CanInteract_Implementation(AWacomPlayerController* PC) const
{
	if (!PC || !BuildConfigWarningReason().IsNone())
	{
		return false;
	}
	URunSession* Run = PC->GetRunSession();
	return Run && !Run->IsPickupCollected(PersistentId);
}

bool AWacomRunPickupActorBase::TryInteract_Implementation(AWacomPlayerController* PC)
{
	if (!CanInteract_Implementation(PC))
	{
		return false;
	}
	if (!TryCollectPickupReward(PC))
	{
		return false;
	}

	ApplyCollectedLifecycle(PC);
	return true;
}

FWacomRunWorldClickableInteractableDebugView
AWacomRunPickupActorBase::GetRunWorldClickableDebugView_Implementation(
	AWacomPlayerController* PC) const
{
	FName LastResult = TEXT("Ok");
	const FName ConfigReason = BuildConfigWarningReason();
	if (!PC)
	{
		LastResult = TEXT("MissingPlayerController");
	}
	else if (!PC->GetRunSession())
	{
		LastResult = TEXT("MissingRunSession");
	}
	else if (!ConfigReason.IsNone())
	{
		LastResult = ConfigReason;
	}
	else if (HasDuplicatePersistentIdInWorld())
	{
		LastResult = TEXT("DuplicatePersistentId");
	}
	else if (IsCollectedFor(PC))
	{
		LastResult = TEXT("Collected");
	}

	return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
		this,
		PersistentId,
		GetHoverPromptText(PC),
		CanInteract_Implementation(PC),
		/*bHasCompletionState*/true,
		IsCollectedFor(PC),
		LastResult,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent,
		ClickBounds);
}

FWacomRunPickupBaseDebugView AWacomRunPickupActorBase::GetRunPickupBaseDebugView(
	AWacomPlayerController* PC) const
{
	FWacomRunPickupBaseDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.bHasRunSession = PC && PC->GetRunSession();
	View.bCanInteract = CanInteract_Implementation(PC);
	View.bIsCollected = IsCollectedFor(PC);
	View.ConfigWarningReason = BuildConfigWarningReason();
	View.bConfigValid = View.ConfigWarningReason.IsNone();
	View.bDuplicatePersistentIdDetected = HasDuplicatePersistentIdInWorld();
	View.HoverPrompt = GetHoverPromptText(PC).ToString();
	View.CollectedHoverPrompt = CollectedHoverPromptText.IsEmpty()
		? GetDefaultCollectedHoverPromptText().ToString()
		: CollectedHoverPromptText.ToString();

	if (!PC)
	{
		View.LastDebugResult = TEXT("MissingPlayerController");
	}
	else if (!PC->GetRunSession())
	{
		View.LastDebugResult = TEXT("MissingRunSession");
	}
	else if (!View.ConfigWarningReason.IsNone())
	{
		View.LastDebugResult = View.ConfigWarningReason;
	}
	else if (View.bDuplicatePersistentIdDetected)
	{
		View.LastDebugResult = TEXT("DuplicatePersistentId");
	}
	else if (View.bIsCollected)
	{
		View.LastDebugResult = TEXT("Collected");
	}
	else
	{
		View.LastDebugResult = TEXT("Ok");
	}

	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	View.bClickTargetConfigured = ClickDebug.bClickTargetConfigured;
	View.ClickTargetStableId = ClickDebug.ClickTargetStableId;
	View.bHasRenderableVisual = ClickDebug.bHasRenderableVisualTarget;
	return View;
}

FString AWacomRunPickupActorBase::GetRunPickupBaseDebugSummary(
	AWacomPlayerController* PC) const
{
	const FWacomRunPickupBaseDebugView View = GetRunPickupBaseDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunPickupBase{Actor=%s PersistentId=%s HasRun=%s CanInteract=%s Collected=%s ConfigValid=%s ConfigReason=%s Duplicate=%s HasVisual=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CollectedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsCollected ? TEXT("true") : TEXT("false"),
		View.bConfigValid ? TEXT("true") : TEXT("false"),
		*View.ConfigWarningReason.ToString(),
		View.bDuplicatePersistentIdDetected ? TEXT("true") : TEXT("false"),
		View.bHasRenderableVisual ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.CollectedHoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
}

void AWacomRunPickupActorBase::LogRunPickupBaseDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[RunPickupActorBase] %s"),
		*GetRunPickupBaseDebugSummary(PC));
}

void AWacomRunPickupActorBase::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		PickupVisual ? Cast<UPrimitiveComponent>(PickupVisual) : Cast<UPrimitiveComponent>(ClickBounds),
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

void AWacomRunPickupActorBase::RefreshClickTargetBindingAndRuntimeTarget()
{
	RefreshClickTargetBinding();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}
}

FName AWacomRunPickupActorBase::BuildConfigWarningReason() const
{
	if (PersistentId.IsNone())
	{
		return TEXT("MissingPersistentId");
	}
	return GetRewardConfigWarningReason();
}

bool AWacomRunPickupActorBase::HasDuplicatePersistentIdInWorld() const
{
	if (PersistentId.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<AWacomRunPickupActorBase> It(GetWorld()); It; ++It)
	{
		const AWacomRunPickupActorBase* Other = *It;
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

FName AWacomRunPickupActorBase::BuildDebugPersistentIdFromActorName(
	const FString& ActorName,
	const TCHAR* Prefix,
	const TCHAR* EmptyFallback)
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
		Sanitized = EmptyFallback ? EmptyFallback : TEXT("Pickup");
	}
	return FName(*FString::Printf(TEXT("%s%s"), Prefix ? Prefix : TEXT("Pickup.Debug."), *Sanitized));
}

bool AWacomRunPickupActorBase::IsCollectedFor(AWacomPlayerController* PC) const
{
	if (!PC || PersistentId.IsNone())
	{
		return false;
	}
	if (URunSession* Run = PC->GetRunSession())
	{
		return Run->IsPickupCollected(PersistentId);
	}
	return false;
}

void AWacomRunPickupActorBase::ApplyCollectedLifecycle(AWacomPlayerController* PC)
{
	if (PC)
	{
		PC->UnregisterCandidateInteractable(this);
	}
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->ClearProbePreview();
	}

	if (bDestroyWhenCollected)
	{
		Destroy();
		return;
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

FText AWacomRunPickupActorBase::GetDefaultInteractPromptText() const
{
	return LOCTEXT("DefaultInteractPrompt", "按 E 拾取");
}

FText AWacomRunPickupActorBase::GetDefaultHoverPromptText() const
{
	return LOCTEXT("DefaultHoverPrompt", "点击拾取");
}

FText AWacomRunPickupActorBase::GetDefaultCollectedHoverPromptText() const
{
	return LOCTEXT("DefaultCollectedHoverPrompt", "已拾取");
}

#undef LOCTEXT_NAMESPACE
