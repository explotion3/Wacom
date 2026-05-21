// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomShopTriggerActor.h"

#define LOCTEXT_NAMESPACE "WacomShopTriggerActor"

#include "Components/SphereComponent.h"
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

	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 交易");
}

void AWacomShopTriggerActor::BeginPlay()
{
	Super::BeginPlay();

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

#undef LOCTEXT_NAMESPACE
