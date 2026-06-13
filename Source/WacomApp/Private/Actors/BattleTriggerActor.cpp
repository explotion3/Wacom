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
#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Encounters/EncounterDefinition.h"
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

	FString JoinTriggerNames(const TArray<FName>& Names)
	{
		TArray<FString> Strings;
		Strings.Reserve(Names.Num());
		for (const FName& Name : Names)
		{
			Strings.Add(Name.ToString());
		}
		return FString::Join(Strings, TEXT(","));
	}

	const FEncounterEnemySlot* FindEncounterEnemySlotById(
		const UEncounterDefinition* Encounter,
		FName EnemySlotId)
	{
		if (!Encounter || EnemySlotId.IsNone())
		{
			return nullptr;
		}

		for (const FEncounterEnemySlot& Slot : Encounter->EnemySlots)
		{
			if (Slot.EnemySlotId == EnemySlotId)
			{
				return &Slot;
			}
		}
		return nullptr;
	}

	void CollectValidEncounterEnemySlotIdsInOrder(
		const UEncounterDefinition* Encounter,
		TArray<FName>& OutEnemySlotIds)
	{
		OutEnemySlotIds.Reset();
		if (!Encounter)
		{
			return;
		}

		TSet<FName> UsedIds;
		for (const FEncounterEnemySlot& Slot : Encounter->EnemySlots)
		{
			if (Slot.EnemySlotId.IsNone() || !Slot.EnemyDefinition || UsedIds.Contains(Slot.EnemySlotId))
			{
				continue;
			}

			UsedIds.Add(Slot.EnemySlotId);
			OutEnemySlotIds.Add(Slot.EnemySlotId);
		}
	}

	void CollectSceneEnemyHostSlotDiff(
		const UEncounterDefinition* Encounter,
		const TArray<FWacomBattleSceneEnemyHostSlot>& SceneEnemyHostSlots,
		TArray<FName>& OutMissingSlotIds,
		TArray<FName>& OutExtraSlotIds)
	{
		OutMissingSlotIds.Reset();
		OutExtraSlotIds.Reset();
		if (!Encounter)
		{
			return;
		}

		TArray<FName> EncounterSlotIds;
		CollectValidEncounterEnemySlotIdsInOrder(Encounter, EncounterSlotIds);
		TSet<FName> EncounterSlotIdSet(EncounterSlotIds);
		TSet<FName> UsedSceneSlotIds;
		for (const FWacomBattleSceneEnemyHostSlot& Slot : SceneEnemyHostSlots)
		{
			if (Slot.EnemySlotId.IsNone())
			{
				continue;
			}

			UsedSceneSlotIds.Add(Slot.EnemySlotId);
			if (!EncounterSlotIdSet.Contains(Slot.EnemySlotId))
			{
				OutExtraSlotIds.AddUnique(Slot.EnemySlotId);
			}
		}

		for (const FName& EncounterSlotId : EncounterSlotIds)
		{
			if (!UsedSceneSlotIds.Contains(EncounterSlotId))
			{
				OutMissingSlotIds.Add(EncounterSlotId);
			}
		}
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

	if (!HasConfiguredBattleDefinition())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: EncounterDefinition 未配置或无有效敌人槽，触发将被忽略"),
			*GetName());
	}
}

void ABattleTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
	TArray<AWacomBattleEnemyActor*> Hosts;
	BuildBattleSceneEnemyHosts(Hosts);
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
	const UEnemyDefinition* FirstEnemySlotDefinition = ResolveFirstValidEnemySlotDefinition();
	if (!FirstEnemySlotDefinition)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: TryActivate 时 EncounterDefinition 无有效敌人"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[BattleTriggerActor] %s 触发战斗：FirstEnemySlotDefinition=%s EncounterDefinition=%s"),
		*GetName(),
		*GetNameSafe(FirstEnemySlotDefinition),
		*GetNameSafe(EncounterDefinition));

	PC->RequestEnterBattle(this);
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

void ABattleTriggerActor::SyncSceneEnemyHostSlotsFromEncounter()
{
	if (!EncounterDefinition)
	{
		return;
	}

	TArray<FName> EncounterSlotIds;
	CollectValidEncounterEnemySlotIdsInOrder(EncounterDefinition, EncounterSlotIds);
	if (EncounterSlotIds.IsEmpty())
	{
		return;
	}

	Modify();

	TMap<FName, AWacomBattleEnemyActor*> ExistingHostsBySlotId;
	TArray<FWacomBattleSceneEnemyHostSlot> ExtraSlots;
	TSet<FName> EncounterSlotIdSet(EncounterSlotIds);
	TSet<FName> UsedExistingSlotIds;
	for (const FWacomBattleSceneEnemyHostSlot& Slot : SceneEnemyHostSlots)
	{
		if (Slot.EnemySlotId.IsNone())
		{
			ExtraSlots.Add(Slot);
			continue;
		}

		if (EncounterSlotIdSet.Contains(Slot.EnemySlotId))
		{
			if (!UsedExistingSlotIds.Contains(Slot.EnemySlotId))
			{
				ExistingHostsBySlotId.Add(Slot.EnemySlotId, Slot.SceneEnemyHost);
				UsedExistingSlotIds.Add(Slot.EnemySlotId);
			}
			else
			{
				ExtraSlots.Add(Slot);
			}
			continue;
		}

		ExtraSlots.Add(Slot);
	}

	TArray<FWacomBattleSceneEnemyHostSlot> SyncedSlots;
	SyncedSlots.Reserve(EncounterSlotIds.Num() + ExtraSlots.Num());
	for (const FName& EncounterSlotId : EncounterSlotIds)
	{
		FWacomBattleSceneEnemyHostSlot SyncedSlot;
		SyncedSlot.EnemySlotId = EncounterSlotId;
		if (AWacomBattleEnemyActor** ExistingHost = ExistingHostsBySlotId.Find(EncounterSlotId))
		{
			SyncedSlot.SceneEnemyHost = *ExistingHost;
		}
		SyncedSlots.Add(SyncedSlot);
	}
	SyncedSlots.Append(ExtraSlots);
	SceneEnemyHostSlots = MoveTemp(SyncedSlots);

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	BuildBattleSceneEnemyHosts(SceneHosts);
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
	else if (!HasConfiguredBattleDefinition())
	{
		LastResult = TEXT("MissingBattleDefinition");
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
				"BattleTrigger 摆放配置错误：Actor={0} 缺少 PersistentId，胜利销毁和撤离 BattleProgress 无法持久化。EncounterDefinition={1}。"),
			FText::FromString(GetName()),
			FText::FromString(EncounterDefinition ? EncounterDefinition->GetName() : TEXT("None"))));
		Result = EDataValidationResult::Invalid;
	}

	if (!HasConfiguredBattleDefinition())
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingEnemyDefinition",
				"BattleTrigger 摆放配置错误：Actor={0} PersistentId={1} 缺少有效 EncounterDefinition，运行时不会进入战斗。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId)));
		Result = EDataValidationResult::Invalid;
	}
	else if (EncounterDefinition)
	{
		if (EncounterDefinition->EnemySlots.IsEmpty())
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementEncounterMissingEnemySlots",
					"BattleTrigger 摆放配置错误：Actor={0} EncounterDefinition={1} 的 EnemySlots 为空，运行时不会进入战斗。"),
				FText::FromString(GetName()),
				FText::FromString(EncounterDefinition->GetName())));
			Result = EDataValidationResult::Invalid;
		}

		TSet<FName> UsedEncounterEnemySlotIds;
		for (int32 SlotIndex = 0; SlotIndex < EncounterDefinition->EnemySlots.Num(); ++SlotIndex)
		{
			const FEncounterEnemySlot& Slot = EncounterDefinition->EnemySlots[SlotIndex];
			if (Slot.EnemySlotId.IsNone())
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementEncounterMissingEnemySlotId",
						"BattleTrigger 摆放配置错误：Actor={0} EncounterDefinition={1} EnemySlots[{2}] 缺少 EnemySlotId，运行时多敌人身份无法稳定匹配。"),
					FText::FromString(GetName()),
					FText::FromString(EncounterDefinition->GetName()),
					FText::AsNumber(SlotIndex)));
				Result = EDataValidationResult::Invalid;
			}
			else if (UsedEncounterEnemySlotIds.Contains(Slot.EnemySlotId))
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementEncounterDuplicateEnemySlotId",
						"BattleTrigger 摆放配置错误：Actor={0} EncounterDefinition={1} EnemySlotId={2} 重复，运行时 BattleSession 会拒绝初始化。"),
					FText::FromString(GetName()),
					FText::FromString(EncounterDefinition->GetName()),
					FText::FromName(Slot.EnemySlotId)));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				UsedEncounterEnemySlotIds.Add(Slot.EnemySlotId);
			}

			if (!Slot.EnemyDefinition)
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementEncounterMissingEnemyDefinition",
						"BattleTrigger 摆放配置错误：Actor={0} EncounterDefinition={1} EnemySlots[{2}] 缺少 EnemyDefinition，运行时不会进入战斗。"),
					FText::FromString(GetName()),
					FText::FromString(EncounterDefinition->GetName()),
					FText::AsNumber(SlotIndex)));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	TArray<FName> MissingSceneEnemyHostSlotIds;
	TArray<FName> ExtraSceneEnemyHostSlotIds;
	CollectSceneEnemyHostSlotDiff(
		EncounterDefinition,
		SceneEnemyHostSlots,
		MissingSceneEnemyHostSlotIds,
		ExtraSceneEnemyHostSlotIds);

	if (SceneEnemyHostSlots.Num() > 0)
	{
		TSet<FName> UsedHostSlotIds;
		TSet<AWacomBattleEnemyActor*> UsedHosts;
		for (int32 SlotIndex = 0; SlotIndex < SceneEnemyHostSlots.Num(); ++SlotIndex)
		{
			const FWacomBattleSceneEnemyHostSlot& Slot = SceneEnemyHostSlots[SlotIndex];
			if (Slot.EnemySlotId.IsNone())
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementSceneEnemyHostSlotMissingEnemySlotId",
						"BattleTrigger 摆放配置错误：Actor={0} SceneEnemyHostSlots[{1}] 缺少 EnemySlotId，无法映射 Encounter 敌人槽。"),
					FText::FromString(GetName()),
					FText::AsNumber(SlotIndex)));
				Result = EDataValidationResult::Invalid;
			}
			else if (UsedHostSlotIds.Contains(Slot.EnemySlotId))
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementSceneEnemyHostSlotDuplicateEnemySlotId",
						"BattleTrigger 摆放配置错误：Actor={0} SceneEnemyHostSlots 中 EnemySlotId={1} 重复。"),
					FText::FromString(GetName()),
					FText::FromName(Slot.EnemySlotId)));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				UsedHostSlotIds.Add(Slot.EnemySlotId);
			}

			if (!Slot.SceneEnemyHost)
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementSceneEnemyHostSlotMissingHost",
						"BattleTrigger 摆放配置错误：Actor={0} SceneEnemyHostSlots[{1}] 缺少 SceneEnemyHost。"),
					FText::FromString(GetName()),
					FText::AsNumber(SlotIndex)));
				Result = EDataValidationResult::Invalid;
				continue;
			}

			if (UsedHosts.Contains(Slot.SceneEnemyHost))
			{
				Context.AddError(FText::Format(
					LOCTEXT("PlacementSceneEnemyHostSlotDuplicateHost",
						"BattleTrigger 摆放配置错误：Actor={0} SceneEnemyHostSlots 重复引用 Host={1}。"),
					FText::FromString(GetName()),
					FText::FromString(Slot.SceneEnemyHost->GetName())));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				UsedHosts.Add(Slot.SceneEnemyHost);
			}

			if (EncounterDefinition && !Slot.EnemySlotId.IsNone())
			{
				const FEncounterEnemySlot* EncounterSlot =
					FindEncounterEnemySlotById(EncounterDefinition, Slot.EnemySlotId);
				if (!EncounterSlot)
				{
					Context.AddError(FText::Format(
						LOCTEXT("PlacementSceneEnemyHostSlotUnknownEncounterSlot",
							"BattleTrigger 摆放配置错误：Actor={0} SceneEnemyHostSlots EnemySlotId={1} 不在 EncounterDefinition={2} 中；该 Host 无法绑定正式 Encounter 敌人槽。"),
						FText::FromString(GetName()),
						FText::FromName(Slot.EnemySlotId),
						FText::FromString(EncounterDefinition->GetName())));
					Result = EDataValidationResult::Invalid;
				}
				else if (EncounterSlot->EnemyDefinition
					&& Slot.SceneEnemyHost->EnemyDefinition
					&& Slot.SceneEnemyHost->EnemyDefinition != EncounterSlot->EnemyDefinition)
				{
					Context.AddWarning(FText::Format(
						LOCTEXT("PlacementSceneEnemyHostSlotDefinitionMismatch",
							"BattleTrigger 摆放警告：Actor={0} EnemySlotId={1} 的 Encounter EnemyDefinition={2} 与 SceneEnemyHost={3} EnemyDefinition={4} 不一致；请确认 Host 部位制作是否对应。"),
						FText::FromString(GetName()),
						FText::FromName(Slot.EnemySlotId),
						FText::FromString(EncounterSlot->EnemyDefinition->GetName()),
						FText::FromString(Slot.SceneEnemyHost->GetName()),
						FText::FromString(Slot.SceneEnemyHost->EnemyDefinition
							? Slot.SceneEnemyHost->EnemyDefinition->GetName()
							: TEXT("None"))));
					Result = Result == EDataValidationResult::Invalid
						? EDataValidationResult::Invalid
						: EDataValidationResult::Valid;
				}
			}
		}
	}

	if (EncounterDefinition)
	{
		for (const FName& MissingSlotId : MissingSceneEnemyHostSlotIds)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementSceneEnemyHostSlotMissingEncounterSlot",
					"BattleTrigger 摆放配置错误：Actor={0} EncounterDefinition={1} 中的 EnemySlotId={2} 未映射到 SceneEnemyHostSlots；该敌人槽没有场景 Host。"),
				FText::FromString(GetName()),
				FText::FromString(EncounterDefinition->GetName()),
				FText::FromName(MissingSlotId)));
			Result = EDataValidationResult::Invalid;
		}

		if (SceneEnemyHostSlots.IsEmpty())
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementEncounterMissingSceneEnemyHost",
					"BattleTrigger 摆放配置错误：Actor={0} PersistentId={1} 使用 EncounterDefinition={2}，但未绑定 SceneEnemyHostSlots；该战斗没有场景敌人目标。"),
				FText::FromString(GetName()),
				FText::FromName(PersistentId),
				FText::FromString(EncounterDefinition->GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		const UEnemyDefinition* FirstEnemySlotDefinition = ResolveFirstValidEnemySlotDefinition();
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementDuplicatePersistentId",
				"BattleTrigger 摆放警告：Actor={0} PersistentId={1} FirstEnemySlotDefinition={2} 与同关卡其他 BattleTrigger 重复；这些战斗会共享销毁状态和 BattleProgress。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId),
			FText::FromString(FirstEnemySlotDefinition ? FirstEnemySlotDefinition->GetName() : TEXT("None"))));
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
	return PC && HasConfiguredBattleDefinition();
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
	const UEnemyDefinition* FirstEnemySlotDefinition = ResolveFirstValidEnemySlotDefinition();
	View.FirstEnemySlotDefinitionName = FirstEnemySlotDefinition ? FirstEnemySlotDefinition->GetName() : TEXT("None");
	View.EncounterDefinitionName = EncounterDefinition ? EncounterDefinition->GetName() : TEXT("None");
	View.EncounterDefinitionId =
		EncounterDefinition ? EncounterDefinition->EncounterDefinitionId : NAME_None;
	View.EncounterEnemySlotCount =
		EncounterDefinition ? EncounterDefinition->EnemySlots.Num() : 0;
	View.bUsingEncounterDefinition = EncounterDefinition != nullptr;
	TArray<AWacomBattleEnemyActor*> SceneHosts;
	BuildBattleSceneEnemyHosts(SceneHosts);
	View.SceneEnemyHostSlotCount = SceneEnemyHostSlots.Num();
	View.SceneEnemyHostCount = SceneHosts.Num();
	View.bSceneEnemyHostConfigured = SceneHosts.Num() > 0;
	for (const FWacomBattleSceneEnemyHostSlot& Slot : SceneEnemyHostSlots)
	{
		View.SceneEnemyHostSlotIds.Add(Slot.EnemySlotId);
	}
	CollectSceneEnemyHostSlotDiff(
		EncounterDefinition,
		SceneEnemyHostSlots,
		View.MissingSceneEnemyHostSlotIds,
		View.ExtraSceneEnemyHostSlotIds);
	AWacomBattleEnemyActor* FirstSceneHost = SceneHosts.Num() > 0 ? SceneHosts[0] : nullptr;
	View.FirstSceneEnemyHostName = FirstSceneHost ? FirstSceneHost->GetName() : TEXT("None");
	View.SceneEnemyHostEnemyDefinitionName =
		(FirstSceneHost && FirstSceneHost->EnemyDefinition)
			? FirstSceneHost->EnemyDefinition->GetName()
			: TEXT("None");
	for (AWacomBattleEnemyActor* Host : SceneHosts)
	{
		if (Host)
		{
			View.SceneEnemyHostPartCount += Host->GetBattleEnemyPartActors().Num();
		}
	}
	View.bSceneEnemyHostDefinitionMatches = true;
	if (SceneHosts.Num() == 0)
	{
		View.bSceneEnemyHostDefinitionMatches = false;
	}
	else if (SceneEnemyHostSlots.Num() > 0 && EncounterDefinition)
	{
		for (const FWacomBattleSceneEnemyHostSlot& Slot : SceneEnemyHostSlots)
		{
			if (!Slot.SceneEnemyHost || !Slot.SceneEnemyHost->EnemyDefinition)
			{
				continue;
			}

			const FEncounterEnemySlot* EncounterSlot =
				FindEncounterEnemySlotById(EncounterDefinition, Slot.EnemySlotId);
			if (EncounterSlot
				&& EncounterSlot->EnemyDefinition
				&& EncounterSlot->EnemyDefinition != Slot.SceneEnemyHost->EnemyDefinition)
			{
				View.bSceneEnemyHostDefinitionMatches = false;
				break;
			}
		}
	}
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
	else if (!HasConfiguredBattleDefinition())
	{
		View.LastDebugResult = TEXT("MissingBattleDefinition");
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
		TEXT("BattleTrigger{Actor=%s PersistentId=%s FirstEnemySlotDefinition=%s EncounterDefinition=%s EncounterDefinitionId=%s EncounterSlots=%d UsingEncounter=%s FirstSceneEnemyHost=%s SceneEnemyHostEnemyDef=%s SceneEnemyHostSlots=%d SceneEnemyHostCount=%d SceneEnemyHostSlotIds=[%s] MissingSceneEnemyHostSlotIds=[%s] ExtraSceneEnemyHostSlotIds=[%s] SceneEnemyHostParts=%d SceneEnemyHostConfigured=%s SceneEnemyHostDefinitionMatches=%s CanInteract=%s Destroyed=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s DestroyedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.FirstEnemySlotDefinitionName,
		*View.EncounterDefinitionName,
		*View.EncounterDefinitionId.ToString(),
		View.EncounterEnemySlotCount,
		View.bUsingEncounterDefinition ? TEXT("true") : TEXT("false"),
		*View.FirstSceneEnemyHostName,
		*View.SceneEnemyHostEnemyDefinitionName,
		View.SceneEnemyHostSlotCount,
		View.SceneEnemyHostCount,
		*JoinTriggerNames(View.SceneEnemyHostSlotIds),
		*JoinTriggerNames(View.MissingSceneEnemyHostSlotIds),
		*JoinTriggerNames(View.ExtraSceneEnemyHostSlotIds),
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

const UEnemyDefinition* ABattleTriggerActor::ResolveFirstValidEnemySlotDefinition() const
{
	if (EncounterDefinition)
	{
		for (const FEncounterEnemySlot& Slot : EncounterDefinition->EnemySlots)
		{
			if (!Slot.EnemySlotId.IsNone() && Slot.EnemyDefinition)
			{
				return Slot.EnemyDefinition;
			}
		}
		return nullptr;
	}
	return nullptr;
}

void ABattleTriggerActor::BuildBattleEnemySlots(TArray<FBattleEnemySlotInit>& OutEnemySlots) const
{
	OutEnemySlots.Reset();
	if (!EncounterDefinition)
	{
		return;
	}

	OutEnemySlots.Reserve(EncounterDefinition->EnemySlots.Num());
	for (const FEncounterEnemySlot& Slot : EncounterDefinition->EnemySlots)
	{
		if (Slot.EnemySlotId.IsNone() || !Slot.EnemyDefinition)
		{
			continue;
		}

		FBattleEnemySlotInit BattleSlot;
		BattleSlot.EnemySlotId = Slot.EnemySlotId;
		BattleSlot.Enemy = Slot.EnemyDefinition;
		OutEnemySlots.Add(BattleSlot);
	}
}

void ABattleTriggerActor::BuildBattleSceneEnemyHosts(
	TArray<AWacomBattleEnemyActor*>& OutSceneEnemyHosts) const
{
	OutSceneEnemyHosts.Reset();

	if (!EncounterDefinition || SceneEnemyHostSlots.IsEmpty())
	{
		return;
	}

	TArray<FName> EncounterSlotIds;
	CollectValidEncounterEnemySlotIdsInOrder(EncounterDefinition, EncounterSlotIds);
	OutSceneEnemyHosts.Reserve(EncounterSlotIds.Num());
	for (const FName& EncounterSlotId : EncounterSlotIds)
	{
		const FWacomBattleSceneEnemyHostSlot* MatchedSceneSlot = nullptr;
		for (const FWacomBattleSceneEnemyHostSlot& Slot : SceneEnemyHostSlots)
		{
			if (Slot.EnemySlotId == EncounterSlotId && Slot.SceneEnemyHost)
			{
				MatchedSceneSlot = &Slot;
				break;
			}
		}

		if (!MatchedSceneSlot || !MatchedSceneSlot->SceneEnemyHost)
		{
			continue;
		}

		AWacomBattleEnemyActor* Host = MatchedSceneSlot->SceneEnemyHost;
		Host->EnemySlotId = EncounterSlotId;
		Host->RefreshBattleEnemyPartAuthoringState();
		OutSceneEnemyHosts.AddUnique(Host);
	}
}

bool ABattleTriggerActor::TryBuildBattleEntryViewStageRequest(
	FWacomFirstPersonViewStageRequest& OutRequest) const
{
	OutRequest = FWacomFirstPersonViewStageRequest();
	if (!BattleEntryViewpoint)
	{
		return false;
	}

	OutRequest.bHasViewTransform = true;
	OutRequest.ViewTransform = BattleEntryViewpoint->GetActorTransform();
	OutRequest.Reason = FName(TEXT("BattleEntry"));
	OutRequest.DebugSource = PersistentId.IsNone()
		? FName(*GetName())
		: PersistentId;
	return true;
}

void ABattleTriggerActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

bool ABattleTriggerActor::HasConfiguredBattleDefinition() const
{
	TArray<FBattleEnemySlotInit> EnemySlots;
	BuildBattleEnemySlots(EnemySlots);
	return EnemySlots.Num() > 0;
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
