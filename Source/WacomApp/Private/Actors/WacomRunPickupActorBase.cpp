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
#include "Pickups/RunPickupDefinition.h"
#include "UObject/UnrealType.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

namespace
{
	bool ShouldValidatePickupPlacementActor(const AWacomRunPickupActorBase& Pickup)
	{
		return !Pickup.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !Pickup.IsTemplate();
	}

	FString BuildPickupDefinitionValidationContext(
		const AWacomRunPickupActorBase& Pickup)
	{
		const FObjectProperty* DefinitionProperty =
			FindFProperty<FObjectProperty>(Pickup.GetClass(), TEXT("PickupDefinition"));
		if (!DefinitionProperty)
		{
			return TEXT("Definition=None");
		}

		const UWacomRunPickupDefinition* Definition =
			Cast<UWacomRunPickupDefinition>(
				DefinitionProperty->GetObjectPropertyValue_InContainer(&Pickup));
		if (!Definition)
		{
			return TEXT("Definition=None");
		}

		const UEnum* RewardTypeEnum = StaticEnum<EWacomRunPickupRewardType>();
		const FString RewardTypeText = RewardTypeEnum
			? RewardTypeEnum->GetNameStringByValue(static_cast<int64>(Definition->RewardType))
			: FString::FromInt(static_cast<int32>(Definition->RewardType));
		return FString::Printf(
			TEXT("Definition=%s PickupId=%s RewardType=%s"),
			*Definition->GetName(),
			*Definition->PickupId.ToString(),
			*RewardTypeText);
	}

	FText FormatPickupValidationError(
		const AWacomRunPickupActorBase& Pickup,
		FName Reason)
	{
		if (Reason == FName(TEXT("MissingPersistentId")))
		{
			return FText::Format(
				LOCTEXT("PlacementMissingPersistentId",
					"Run Pickup 摆放配置错误：Actor={0} 缺少 PersistentId，运行时不会结算。"),
				FText::FromString(Pickup.GetName()));
		}
		if (Reason == FName(TEXT("InvalidGoldAmount")))
		{
			return FText::Format(
				LOCTEXT("PlacementInvalidGoldAmount",
					"Run Pickup 摆放配置错误：Actor={0} PersistentId={1} RewardType=Gold 金币数量必须大于 0。"),
				FText::FromString(Pickup.GetName()),
				FText::FromName(Pickup.PersistentId));
		}
		if (Reason == FName(TEXT("MissingCardDefinition")))
		{
			return FText::Format(
				LOCTEXT("PlacementMissingCardDefinition",
					"Run Pickup 摆放配置错误：Actor={0} PersistentId={1} RewardType=Card 缺少 CardDefinition。"),
				FText::FromString(Pickup.GetName()),
				FText::FromName(Pickup.PersistentId));
		}
		if (Reason == FName(TEXT("MissingPickupDefinition")))
		{
			return FText::Format(
				LOCTEXT("PlacementMissingPickupDefinition",
					"Run RewardPickup 摆放配置错误：Actor={0} PersistentId={1} Definition=None，缺少 PickupDefinition。"),
				FText::FromString(Pickup.GetName()),
				FText::FromName(Pickup.PersistentId));
		}
		if (Reason == FName(TEXT("MissingPickupId")))
		{
			return FText::Format(
				LOCTEXT("PlacementMissingPickupId",
					"Run RewardPickup 摆放配置错误：Actor={0} PersistentId={1} {2}，PickupDefinition 缺少 PickupId。"),
				FText::FromString(Pickup.GetName()),
				FText::FromName(Pickup.PersistentId),
				FText::FromString(BuildPickupDefinitionValidationContext(Pickup)));
		}
		if (Reason == FName(TEXT("MissingRewardType")))
		{
			return FText::Format(
				LOCTEXT("PlacementMissingRewardType",
					"Run RewardPickup 摆放配置错误：Actor={0} PersistentId={1} {2}，PickupDefinition RewardType 不能为 None。"),
				FText::FromString(Pickup.GetName()),
				FText::FromName(Pickup.PersistentId),
				FText::FromString(BuildPickupDefinitionValidationContext(Pickup)));
		}

		return FText::Format(
			LOCTEXT("PlacementUnknownReason",
				"Run Pickup 摆放配置错误：Actor={0} PersistentId={1} {2} Reason={3}。"),
			FText::FromString(Pickup.GetName()),
			FText::FromName(Pickup.PersistentId),
			FText::FromString(BuildPickupDefinitionValidationContext(Pickup)),
			FText::FromName(Reason));
	}
}

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
	RefreshPickupAuthoringState();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}

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
	RefreshPickupAuthoringState();
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
	View.TriggerRadius = TriggerRadius;
	View.ClickBoundsExtent = ClickBounds ? ClickBounds->GetUnscaledBoxExtent() : FVector::ZeroVector;
	View.VisualName = PickupVisual ? PickupVisual->GetFName() : NAME_None;
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
		TEXT("RunPickupBase{Actor=%s PersistentId=%s HasRun=%s CanInteract=%s Collected=%s TriggerRadius=%.1f BoundsExtent=%s Visual=%s ConfigValid=%s ConfigReason=%s Duplicate=%s HasVisual=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CollectedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsCollected ? TEXT("true") : TEXT("false"),
		View.TriggerRadius,
		*View.ClickBoundsExtent.ToCompactString(),
		*View.VisualName.ToString(),
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

#if WITH_EDITOR
EDataValidationResult AWacomRunPickupActorBase::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidatePickupPlacementActor(*this))
	{
		return Result;
	}

	const FName ConfigReason = BuildConfigWarningReason();
	if (!ConfigReason.IsNone())
	{
		Context.AddError(FormatPickupValidationError(*this, ConfigReason));
		Result = EDataValidationResult::Invalid;
	}

	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementDuplicatePersistentId",
				"Run Pickup 摆放警告：Actor={0} PersistentId={1} 与同关卡其他 Pickup 重复；这些 Pickup 会共享同一份已拾取状态。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId)));
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

void AWacomRunPickupActorBase::RefreshPickupAuthoringState()
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
}

void AWacomRunPickupActorBase::ApplyDebugPickupAuthoringDefaults(
	FName InPersistentId,
	float InTriggerRadius,
	bool bInDestroyWhenCollected)
{
	PersistentId = InPersistentId;
	TriggerRadius = InTriggerRadius;
	bDestroyWhenCollected = bInDestroyWhenCollected;
	InteractPromptText = GetDefaultInteractPromptText();
	HoverPromptText = GetDefaultHoverPromptText();
	CollectedHoverPromptText = GetDefaultCollectedHoverPromptText();
	RefreshPickupAuthoringState();
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
