// Copyright Wacom. All Rights Reserved.

#include "Actors/BattleTriggerActor.h"

#define LOCTEXT_NAMESPACE "BattleTriggerActor"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Enemies/EnemyDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

ABattleTriggerActor::ABattleTriggerActor()
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

	HoverPromptText = LOCTEXT("DefaultHoverPrompt", "点击战斗");
	DestroyedHoverPromptText = LOCTEXT("DefaultDestroyedHoverPrompt", "战斗已结束");
}

void ABattleTriggerActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshClickTargetBinding();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}

	// ---- 存档：PersistentId 校验 + 已销毁检查 ----
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: PersistentId 未配置，本触发器不参与存档"),
			*GetName());
	}
	else
	{
		// 关卡级 id 唯一性检查
		for (TActorIterator<ABattleTriggerActor> It(GetWorld()); It; ++It)
		{
			ABattleTriggerActor* Other = *It;
			if (Other && Other != this && !Other->HasAnyFlags(RF_ClassDefaultObject))
			{
				if (Other->PersistentId == PersistentId)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[BattleTriggerActor] PersistentId %s 与 %s 重复，请为每个 Trigger 设置唯一 id"),
						*PersistentId.ToString(),
						*Other->GetName());
					break;
				}
			}
		}

		// 已被销毁过：直接 Destroy
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
			{
				if (URunSession* Run = WacomPC->GetRunSession())
				{
					if (Run->IsTriggerDestroyed(PersistentId))
					{
						UE_LOG(LogTemp, Display,
							TEXT("[BattleTriggerActor] %s (id=%s) 已在存档中被销毁，跳过生成"),
							*GetName(), *PersistentId.ToString());
						Destroy();
						return;
					}
				}
			}
		}
	}

	// TriggerRadius 可能在 Details 面板被修改，BeginPlay 同步一次。
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &ABattleTriggerActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &ABattleTriggerActor::HandleEndOverlap);
	}

	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: EnemyDef 未配置，触发将被忽略"),
			*GetName());
	}
}

void ABattleTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
}

void ABattleTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Trigger 销毁前从 PC 候选列表里反注册，避免悬空指针。
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			WacomPC->UnregisterCandidateInteractable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ABattleTriggerActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!OtherActor) { return; }

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	PC->RegisterCandidateInteractable(this);
}

void ABattleTriggerActor::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	if (!OtherActor) { return; }

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	PC->UnregisterCandidateInteractable(this);
}

void ABattleTriggerActor::TryActivate(AWacomPlayerController* PC)
{
	if (!PC) { return; }
	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: TryActivate 时 EnemyDef 仍为空"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[BattleTriggerActor] %s 触发战斗：EnemyDef=%s"),
		*GetName(), *GetNameSafe(EnemyDef));

	PC->RequestEnterBattle(EnemyDef, this);
}

FText ABattleTriggerActor::GetInteractPromptText_Implementation(AWacomPlayerController* /*PC*/) const
{
	return LOCTEXT("InteractPrompt", "按 E 战斗");
}

FText ABattleTriggerActor::GetHoverPromptText(AWacomPlayerController* PC) const
{
	if (IsDestroyedFor(PC))
	{
		return DestroyedHoverPromptText.IsEmpty()
			? LOCTEXT("FallbackDestroyedHoverPrompt", "战斗已结束")
			: DestroyedHoverPromptText;
	}
	return HoverPromptText.IsEmpty()
		? LOCTEXT("FallbackHoverPrompt", "点击战斗")
		: HoverPromptText;
}

FText ABattleTriggerActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	return GetHoverPromptText(PC);
}

FWacomRunWorldClickableInteractableDebugView
ABattleTriggerActor::GetRunWorldClickableDebugView_Implementation(
	AWacomPlayerController* PC) const
{
	FName LastResult = TEXT("Ok");
	const bool bDestroyed = IsDestroyedFor(PC);
	if (!PC)
	{
		LastResult = TEXT("MissingPlayerController");
	}
	else if (!EnemyDef)
	{
		LastResult = TEXT("MissingEnemyDefinition");
	}
	else if (bDestroyed)
	{
		LastResult = TEXT("Destroyed");
	}

	return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
		this,
		PersistentId,
		GetHoverPromptText(PC),
		CanInteract_Implementation(PC),
		/*bHasCompletionState*/true,
		bDestroyed,
		LastResult,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

FVector ABattleTriggerActor::GetInteractLocation_Implementation(AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool ABattleTriggerActor::CanInteract_Implementation(AWacomPlayerController* PC) const
{
	return PC && EnemyDef;
}

bool ABattleTriggerActor::TryInteract_Implementation(AWacomPlayerController* PC)
{
	if (!CanInteract_Implementation(PC))
	{
		return false;
	}
	TryActivate(PC);
	return true;
}

FWacomBattleTriggerDebugView ABattleTriggerActor::GetBattleTriggerDebugView(
	AWacomPlayerController* PC) const
{
	FWacomBattleTriggerDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.EnemyDefinitionName = EnemyDef ? EnemyDef->GetName() : TEXT("None");
	View.bCanInteract = CanInteract_Implementation(PC);
	View.bIsDestroyed = IsDestroyedFor(PC);
	View.HoverPrompt = GetHoverPromptText(PC).ToString();
	View.DestroyedHoverPrompt = DestroyedHoverPromptText.IsEmpty()
		? FString(TEXT("战斗已结束"))
		: DestroyedHoverPromptText.ToString();

	if (!PC)
	{
		View.LastDebugResult = TEXT("MissingPlayerController");
	}
	else if (!EnemyDef)
	{
		View.LastDebugResult = TEXT("MissingEnemyDefinition");
	}
	else if (View.bIsDestroyed)
	{
		View.LastDebugResult = TEXT("Destroyed");
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

FString ABattleTriggerActor::GetBattleTriggerDebugSummary(AWacomPlayerController* PC) const
{
	const FWacomBattleTriggerDebugView View = GetBattleTriggerDebugView(PC);
	return FString::Printf(
		TEXT("BattleTrigger{Actor=%s PersistentId=%s EnemyDef=%s CanInteract=%s Destroyed=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s DestroyedHoverPrompt=%s Last=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.EnemyDefinitionName,
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsDestroyed ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.DestroyedHoverPrompt,
		*View.LastDebugResult.ToString());
}

void ABattleTriggerActor::LogBattleTriggerDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[BattleTriggerActor] %s"),
		*GetBattleTriggerDebugSummary(PC));
}

void ABattleTriggerActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

bool ABattleTriggerActor::IsDestroyedFor(AWacomPlayerController* PC) const
{
	if (!PC || PersistentId.IsNone())
	{
		return false;
	}
	if (URunSession* Run = PC->GetRunSession())
	{
		return Run->IsTriggerDestroyed(PersistentId);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
