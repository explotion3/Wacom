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

#include "Actors/WacomBattleEnemyActor.h"
#include "Enemies/EnemyDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

namespace
{
	bool ShouldValidateBattleTriggerPlacementActor(const ABattleTriggerActor& Trigger)
	{
		return !Trigger.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !Trigger.IsTemplate();
	}
}

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
		if (HasDuplicatePersistentIdInWorld())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattleTriggerActor] %s: PersistentId %s 与同关卡其他 BattleTrigger 重复，请为每个 Trigger 设置唯一 id"),
				*GetName(),
				*PersistentId.ToString());
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
		ClickTargetBridgeComponent,
		ClickBounds);
}

#if WITH_EDITOR
EDataValidationResult ABattleTriggerActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidateBattleTriggerPlacementActor(*this))
	{
		return Result;
	}

	if (PersistentId.IsNone())
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingPersistentId",
				"BattleTrigger 摆放配置错误：Actor={0} 缺少 PersistentId，胜利销毁和撤离 BattleProgress 无法持久化。EnemyDef={1}。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDef ? EnemyDef->GetName() : TEXT("None"))));
		Result = EDataValidationResult::Invalid;
	}

	if (!EnemyDef)
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingEnemyDefinition",
				"BattleTrigger 摆放配置错误：Actor={0} PersistentId={1} 缺少 EnemyDef，运行时不会进入战斗。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId)));
		Result = EDataValidationResult::Invalid;
	}

	if (!SceneEnemyHost)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementMissingSceneEnemyHost",
				"BattleTrigger 摆放警告：Actor={0} PersistentId={1} 未绑定 SceneEnemyHost；战斗仍可使用 EnemyInfoBar fallback，但场景敌人部位不会参与 hover / prediction / cue / 拖卡目标绑定。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId)));
	}
	else if (EnemyDef && SceneEnemyHost->EnemyDefinition && SceneEnemyHost->EnemyDefinition != EnemyDef)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementSceneEnemyHostDefinitionMismatch",
				"BattleTrigger 摆放警告：Actor={0} EnemyDef={1} 与 SceneEnemyHost={2} EnemyDefinition={3} 不一致；场景部位会按当前战斗 Snapshot 的 PartId 绑定，请确认制作配置。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDef->GetName()),
			FText::FromString(SceneEnemyHost->GetName()),
			FText::FromString(SceneEnemyHost->EnemyDefinition ? SceneEnemyHost->EnemyDefinition->GetName() : TEXT("None"))));
	}

	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementDuplicatePersistentId",
				"BattleTrigger 摆放警告：Actor={0} PersistentId={1} EnemyDef={2} 与同关卡其他 BattleTrigger 重复；这些战斗会共享销毁状态和 BattleProgress。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId),
			FText::FromString(EnemyDef ? EnemyDef->GetName() : TEXT("None"))));
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
	View.SceneEnemyHostName = SceneEnemyHost ? SceneEnemyHost->GetName() : TEXT("None");
	View.SceneEnemyHostEnemyDefinitionName =
		(SceneEnemyHost && SceneEnemyHost->EnemyDefinition)
			? SceneEnemyHost->EnemyDefinition->GetName()
			: TEXT("None");
	View.SceneEnemyHostPartCount =
		SceneEnemyHost ? SceneEnemyHost->GetBattleEnemyPartActors().Num() : 0;
	View.bSceneEnemyHostConfigured = SceneEnemyHost != nullptr;
	View.bSceneEnemyHostDefinitionMatches =
		SceneEnemyHost
		&& EnemyDef
		&& SceneEnemyHost->EnemyDefinition
		&& SceneEnemyHost->EnemyDefinition == EnemyDef;
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
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("BattleTrigger{Actor=%s PersistentId=%s EnemyDef=%s SceneEnemyHost=%s SceneEnemyHostEnemyDef=%s SceneEnemyHostParts=%d SceneEnemyHostConfigured=%s SceneEnemyHostDefinitionMatches=%s CanInteract=%s Destroyed=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s DestroyedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.EnemyDefinitionName,
		*View.SceneEnemyHostName,
		*View.SceneEnemyHostEnemyDefinitionName,
		View.SceneEnemyHostPartCount,
		View.bSceneEnemyHostConfigured ? TEXT("true") : TEXT("false"),
		View.bSceneEnemyHostDefinitionMatches ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsDestroyed ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.DestroyedHoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
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

bool ABattleTriggerActor::HasDuplicatePersistentIdInWorld() const
{
	if (PersistentId.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<ABattleTriggerActor> It(GetWorld()); It; ++It)
	{
		const ABattleTriggerActor* Other = *It;
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
