// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomShopTriggerActor.h"

#define LOCTEXT_NAMESPACE "WacomShopTriggerActor"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#include "GameFramework/WacomPlayerController.h"
#include "Shops/ShopDefinition.h"

AWacomShopTriggerActor::AWacomShopTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	RootComponent = TriggerSphere;

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	ClickBounds->SetupAttachment(RootComponent);
	ClickBounds->SetBoxExtent(FVector(100.f, 100.f, 100.f));
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

	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 交易");
	HoverPromptText = LOCTEXT("DefaultHoverPrompt", "点击交易");
}

void AWacomShopTriggerActor::BeginPlay()
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
			TEXT("[ShopTriggerActor] %s: PersistentId 未配置，商店不会打开"),
			*GetName());
	}
	else
	{
		for (TActorIterator<AWacomShopTriggerActor> It(GetWorld()); It; ++It)
		{
			AWacomShopTriggerActor* Other = *It;
			if (Other && Other != this && !Other->HasAnyFlags(RF_ClassDefaultObject))
			{
				if (Other->PersistentId == PersistentId)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[ShopTriggerActor] PersistentId %s 与 %s 重复；两个商店会共享同一份库存状态"),
						*PersistentId.ToString(),
						*Other->GetName());
					break;
				}
			}
		}
	}

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &AWacomShopTriggerActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &AWacomShopTriggerActor::HandleEndOverlap);
	}
}

void AWacomShopTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
}

void AWacomShopTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void AWacomShopTriggerActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
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

void AWacomShopTriggerActor::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
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

FText AWacomShopTriggerActor::GetInteractPromptText_Implementation(AWacomPlayerController* /*PC*/) const
{
	return InteractPromptText.IsEmpty()
		? LOCTEXT("FallbackInteractPrompt", "按 E 交易")
		: InteractPromptText;
}

FText AWacomShopTriggerActor::GetHoverPromptText(AWacomPlayerController* /*PC*/) const
{
	return HoverPromptText.IsEmpty()
		? LOCTEXT("FallbackHoverPrompt", "点击交易")
		: HoverPromptText;
}

FVector AWacomShopTriggerActor::GetInteractLocation_Implementation(AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool AWacomShopTriggerActor::CanInteract_Implementation(AWacomPlayerController* PC) const
{
	return PC && !PersistentId.IsNone();
}

TArray<FRunShopOfferInput> AWacomShopTriggerActor::BuildResolvedOffers() const
{
	if (!ShopDefinition)
	{
		return Offers;
	}

	TArray<FRunShopOfferInput> ResolvedOffers;
	ResolvedOffers.Reserve(ShopDefinition->Offers.Num());
	for (const FShopOfferDefinition& OfferDefinition : ShopDefinition->Offers)
	{
		FRunShopOfferInput OfferInput;
		OfferInput.CardDefinition = OfferDefinition.CardDefinition;
		OfferInput.Price = OfferDefinition.Price;
		ResolvedOffers.Add(OfferInput);
	}
	return ResolvedOffers;
}

bool AWacomShopTriggerActor::TryInteract_Implementation(AWacomPlayerController* PC)
{
	if (!CanInteract_Implementation(PC))
	{
		return false;
	}
	return PC->RequestOpenShop(PersistentId, BuildResolvedOffers());
}

FWacomShopTriggerDebugView AWacomShopTriggerActor::GetShopTriggerDebugView(
	AWacomPlayerController* PC) const
{
	FWacomShopTriggerDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.ShopDefinitionName = ShopDefinition ? ShopDefinition->GetName() : TEXT("None");
	View.ResolvedOfferCount = BuildResolvedOffers().Num();
	View.bCanInteract = CanInteract_Implementation(PC);
	View.HoverPrompt = GetHoverPromptText(PC).ToString();

	if (!PC)
	{
		View.LastDebugResult = TEXT("MissingPlayerController");
	}
	else if (PersistentId.IsNone())
	{
		View.LastDebugResult = TEXT("MissingPersistentId");
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

FString AWacomShopTriggerActor::GetShopTriggerDebugSummary(AWacomPlayerController* PC) const
{
	const FWacomShopTriggerDebugView View = GetShopTriggerDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("ShopTrigger{Actor=%s PersistentId=%s ShopDef=%s Offers=%d CanInteract=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.ShopDefinitionName,
		View.ResolvedOfferCount,
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
}

void AWacomShopTriggerActor::LogShopTriggerDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[ShopTriggerActor] %s"),
		*GetShopTriggerDebugSummary(PC));
}

FText AWacomShopTriggerActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	return GetHoverPromptText(PC);
}

FWacomRunWorldClickableInteractableDebugView
AWacomShopTriggerActor::GetRunWorldClickableDebugView_Implementation(
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

	return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
		this,
		PersistentId,
		GetHoverPromptText(PC),
		CanInteract_Implementation(PC),
		/*bHasCompletionState*/false,
		/*bIsCompleted*/false,
		LastResult,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent,
		ClickBounds);
}

void AWacomShopTriggerActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

#undef LOCTEXT_NAMESPACE
