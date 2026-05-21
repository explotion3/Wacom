// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunEventTriggerActor.h"

#define LOCTEXT_NAMESPACE "WacomRunEventTriggerActor"

#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#include "Events/RunEventDefinition.h"
#include "GameFramework/WacomPlayerController.h"
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

	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 查看事件");
	CompletedPromptText = LOCTEXT("DefaultCompletedPrompt", "事件已完成");
	CompletedToastText = LOCTEXT("DefaultCompletedToast", "该事件已完成");
}

void AWacomRunEventTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunEventTriggerActor] %s: PersistentId 未配置，事件不会打开"),
			*GetName());
	}
	else
	{
		for (TActorIterator<AWacomRunEventTriggerActor> It(GetWorld()); It; ++It)
		{
			AWacomRunEventTriggerActor* Other = *It;
			if (Other && Other != this && !Other->HasAnyFlags(RF_ClassDefaultObject))
			{
				if (Other->PersistentId == PersistentId)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[RunEventTriggerActor] PersistentId %s 与 %s 重复；两个事件会共享同一份状态"),
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
			this, &AWacomRunEventTriggerActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &AWacomRunEventTriggerActor::HandleEndOverlap);
	}
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
