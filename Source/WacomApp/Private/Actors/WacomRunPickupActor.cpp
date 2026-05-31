// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunPickupActor.h"

#define LOCTEXT_NAMESPACE "WacomRunPickupActor"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

AWacomRunPickupActor::AWacomRunPickupActor()
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

	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 拾取");
	HoverPromptText = LOCTEXT("DefaultHoverPrompt", "点击拾取");
	CollectedHoverPromptText = LOCTEXT("DefaultCollectedHoverPrompt", "已拾取");
}

void AWacomRunPickupActor::BeginPlay()
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
			TEXT("[RunPickupActor] %s: PersistentId 未配置，拾取物不会结算"),
			*GetName());
	}
	if (GoldAmount <= 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunPickupActor] %s: GoldAmount=%d 非正数，拾取物不会结算"),
			*GetName(),
			GoldAmount);
	}

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &AWacomRunPickupActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &AWacomRunPickupActor::HandleEndOverlap);
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

void AWacomRunPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
}

void AWacomRunPickupActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void AWacomRunPickupActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
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

void AWacomRunPickupActor::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
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

FText AWacomRunPickupActor::GetInteractPromptText_Implementation(
	AWacomPlayerController* /*PC*/) const
{
	return InteractPromptText.IsEmpty()
		? LOCTEXT("FallbackInteractPrompt", "按 E 拾取")
		: InteractPromptText;
}

FText AWacomRunPickupActor::GetHoverPromptText(AWacomPlayerController* PC) const
{
	if (IsCollectedFor(PC))
	{
		return CollectedHoverPromptText.IsEmpty()
			? LOCTEXT("FallbackCollectedHoverPrompt", "已拾取")
			: CollectedHoverPromptText;
	}
	return HoverPromptText.IsEmpty()
		? LOCTEXT("FallbackHoverPrompt", "点击拾取")
		: HoverPromptText;
}

FText AWacomRunPickupActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	return GetHoverPromptText(PC);
}

FVector AWacomRunPickupActor::GetInteractLocation_Implementation(
	AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool AWacomRunPickupActor::CanInteract_Implementation(AWacomPlayerController* PC) const
{
	if (!PC || PersistentId.IsNone() || GoldAmount <= 0)
	{
		return false;
	}
	URunSession* Run = PC->GetRunSession();
	return Run && !Run->IsPickupCollected(PersistentId);
}

bool AWacomRunPickupActor::TryInteract_Implementation(AWacomPlayerController* PC)
{
	if (!CanInteract_Implementation(PC))
	{
		return false;
	}

	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!Run || !Run->CollectGoldPickup(PersistentId, GoldAmount))
	{
		return false;
	}

	if (UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr)
	{
		if (UWacomAppToastSubsystem* ToastSubsystem =
			GameInstance->GetSubsystem<UWacomAppToastSubsystem>())
		{
			ToastSubsystem->ShowGoldChanged(GoldAmount);
		}
	}

	ApplyCollectedLifecycle(PC);
	return true;
}

FWacomRunWorldClickableInteractableDebugView
AWacomRunPickupActor::GetRunWorldClickableDebugView_Implementation(
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
	else if (GoldAmount <= 0)
	{
		LastResult = TEXT("InvalidGoldAmount");
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

FWacomRunPickupDebugView AWacomRunPickupActor::GetRunPickupDebugView(
	AWacomPlayerController* PC) const
{
	FWacomRunPickupDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.GoldAmount = GoldAmount;
	View.bHasRunSession = PC && PC->GetRunSession();
	View.bCanInteract = CanInteract_Implementation(PC);
	View.bIsCollected = IsCollectedFor(PC);
	View.HoverPrompt = GetHoverPromptText(PC).ToString();
	View.CollectedHoverPrompt = CollectedHoverPromptText.IsEmpty()
		? FString(TEXT("已拾取"))
		: CollectedHoverPromptText.ToString();

	if (!PC)
	{
		View.LastDebugResult = TEXT("MissingPlayerController");
	}
	else if (!PC->GetRunSession())
	{
		View.LastDebugResult = TEXT("MissingRunSession");
	}
	else if (PersistentId.IsNone())
	{
		View.LastDebugResult = TEXT("MissingPersistentId");
	}
	else if (GoldAmount <= 0)
	{
		View.LastDebugResult = TEXT("InvalidGoldAmount");
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
	return View;
}

FString AWacomRunPickupActor::GetRunPickupDebugSummary(AWacomPlayerController* PC) const
{
	const FWacomRunPickupDebugView View = GetRunPickupDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunPickup{Actor=%s PersistentId=%s Gold=%d HasRun=%s CanInteract=%s Collected=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CollectedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		View.GoldAmount,
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsCollected ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.CollectedHoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
}

void AWacomRunPickupActor::LogRunPickupDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[RunPickupActor] %s"),
		*GetRunPickupDebugSummary(PC));
}

void AWacomRunPickupActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		PickupVisual ? Cast<UPrimitiveComponent>(PickupVisual) : Cast<UPrimitiveComponent>(ClickBounds),
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

bool AWacomRunPickupActor::IsCollectedFor(AWacomPlayerController* PC) const
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

void AWacomRunPickupActor::ApplyCollectedLifecycle(AWacomPlayerController* PC)
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

#undef LOCTEXT_NAMESPACE
