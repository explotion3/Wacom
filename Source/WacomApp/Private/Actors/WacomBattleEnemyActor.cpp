// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UI/Battle/BattleHUD.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyActor"

namespace
{
	const TCHAR* DebugSnakeEnemyPath =
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake");

	bool ShouldValidateBattleEnemyHostPlacementActor(const AWacomBattleEnemyActor& EnemyActor)
	{
		return !EnemyActor.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !EnemyActor.IsTemplate();
	}

	FString JoinNames(const TArray<FName>& Names)
	{
		TArray<FString> Strings;
		Strings.Reserve(Names.Num());
		for (const FName& Name : Names)
		{
			Strings.Add(Name.ToString());
		}
		return FString::Join(Strings, TEXT(","));
	}

	const TCHAR* GetHostVisualModeDebugString(EWacomBattleEnemyHostVisualMode VisualMode)
	{
		switch (VisualMode)
		{
		case EWacomBattleEnemyHostVisualMode::Flipbook:
			return TEXT("Flipbook");
		case EWacomBattleEnemyHostVisualMode::StaticSprite:
		default:
			return TEXT("StaticSprite");
		}
	}

	struct FEnemyPartAuthoringSlot
	{
		FName PartId = NAME_None;
		FName PartSlotId = NAME_None;
		int32 DefinitionIndex = INDEX_NONE;
	};

	FName BuildDefinitionPartSlotId(const FEnemyPartSlot& PartSlot)
	{
		if (!PartSlot.PartSlotId.IsNone())
		{
			return PartSlot.PartSlotId;
		}

		return PartSlot.PartDef ? PartSlot.PartDef->PartId : NAME_None;
	}

	TArray<FEnemyPartAuthoringSlot> BuildDefinitionAuthoringSlots(const UEnemyDefinition* EnemyDefinition)
	{
		TArray<FEnemyPartAuthoringSlot> Slots;
		if (!EnemyDefinition)
		{
			return Slots;
		}

		Slots.Reserve(EnemyDefinition->Parts.Num());
		for (int32 Index = 0; Index < EnemyDefinition->Parts.Num(); ++Index)
		{
			const FEnemyPartSlot& PartSlot = EnemyDefinition->Parts[Index];
			if (!PartSlot.PartDef || PartSlot.PartDef->PartId.IsNone())
			{
				continue;
			}

			FEnemyPartAuthoringSlot Slot;
			Slot.PartId = PartSlot.PartDef->PartId;
			Slot.PartSlotId = BuildDefinitionPartSlotId(PartSlot);
			Slot.DefinitionIndex = Index;
			Slots.Add(Slot);
		}
		return Slots;
	}

	TMap<FName, int32> BuildDefinitionPartOrder(const UEnemyDefinition* EnemyDefinition)
	{
		TMap<FName, int32> PartOrder;
		if (!EnemyDefinition)
		{
			return PartOrder;
		}

		for (int32 Index = 0; Index < EnemyDefinition->Parts.Num(); ++Index)
		{
			const FEnemyPartSlot& PartSlot = EnemyDefinition->Parts[Index];
			if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone())
			{
				PartOrder.FindOrAdd(PartSlot.PartDef->PartId, Index);
			}
		}
		return PartOrder;
	}

	FString BuildPartIdLeaf(FName PartId)
	{
		FString PartIdString = PartId.ToString();
		FString Left;
		FString Right;
		while (PartIdString.Split(TEXT("."), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromStart))
		{
			PartIdString = Right;
		}
		return PartIdString;
	}

	FString BuildPartSortKey(const AWacomBattleEnemyPartActor* PartActor)
	{
		if (!PartActor)
		{
			return FString();
		}

		const FName EffectivePartId = PartActor->GetEffectivePartDefinitionId();
		const FString PartIdKey = EffectivePartId.IsNone()
			? FString(TEXT("~"))
			: EffectivePartId.ToString();
		return FString::Printf(TEXT("%s|%s"), *PartIdKey, *PartActor->GetName());
	}

	AWacomBattleEnemyPartActor* ResolveChildActorComponentPartActor(
		UChildActorComponent* ChildActorComponent,
		bool bAllowTemplateFallback)
	{
		if (!ChildActorComponent)
		{
			return nullptr;
		}

		if (AWacomBattleEnemyPartActor* PartActor =
			Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActor()))
		{
			return PartActor;
		}

		return bAllowTemplateFallback
			? Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActorTemplate())
			: nullptr;
	}

	void CollectInstanceChildActorComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutChildActorComponents)
	{
		Host.GetComponents<UChildActorComponent>(OutChildActorComponents);
	}

	void CollectBlueprintTemplateChildActorComponents(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutChildActorComponents)
	{
		UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Host.GetClass());
		if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript)
		{
			return;
		}

		for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
		{
			if (!Node)
			{
				continue;
			}

			UChildActorComponent* ChildActorComponent =
				Cast<UChildActorComponent>(Node->GetActualComponentTemplate(BlueprintClass));
			if (ChildActorComponent)
			{
				OutChildActorComponents.AddUnique(ChildActorComponent);
			}
		}
	}

	void CollectChildActorComponentsForPartDiscovery(
		const AWacomBattleEnemyActor& Host,
		TArray<UChildActorComponent*>& OutChildActorComponents,
		bool& bOutAllowTemplateFallback)
	{
		OutChildActorComponents.Reset();

		CollectInstanceChildActorComponents(Host, OutChildActorComponents);
		if (OutChildActorComponents.Num() > 0)
		{
			bOutAllowTemplateFallback = Host.IsTemplate();
			return;
		}

		CollectBlueprintTemplateChildActorComponents(Host, OutChildActorComponents);
		bOutAllowTemplateFallback = true;
	}

	void MarkObjectEditedForBattleEnemyAuthoring(UObject* Object)
	{
#if WITH_EDITOR
		if (Object)
		{
			Object->Modify();
			Object->MarkPackageDirty();
		}
#endif
	}

	struct FDebugSnakeHostPartSpec
	{
		FName PartId = NAME_None;
		FName PartSlotId = NAME_None;
		FVector RelativeLocation = FVector::ZeroVector;
	};

	const TArray<FDebugSnakeHostPartSpec>& GetDebugSnakeHostPartSpecs()
	{
		static const TArray<FDebugSnakeHostPartSpec> Specs = {
			{ TEXT("Snake.Head"), TEXT("Head"), FVector(96.0f, -6.0f, 16.0f) },
			{ TEXT("Snake.Body"), TEXT("Body"), FVector(0.0f, 0.0f, 0.0f) },
			{ TEXT("Snake.Tail"), TEXT("Tail"), FVector(-92.0f, 16.0f, -8.0f) }
		};
		return Specs;
	}

	UChildActorComponent* FindChildActorComponentForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor);

	bool NameContainsPartSlotId(const FString& Name, FName PartSlotId)
	{
		if (PartSlotId.IsNone())
		{
			return false;
		}

		const FString SlotName = PartSlotId.ToString();
		return Name.Contains(SlotName, ESearchCase::IgnoreCase);
	}

	bool NameContainsDefinitionSlot(const FString& Name, const FEnemyPartAuthoringSlot& Slot)
	{
		if (!Slot.PartSlotId.IsNone()
			&& Name.Contains(Slot.PartSlotId.ToString(), ESearchCase::IgnoreCase))
		{
			return true;
		}

		const FString PartIdLeaf = BuildPartIdLeaf(Slot.PartId);
		return !PartIdLeaf.IsEmpty()
			&& Name.Contains(PartIdLeaf, ESearchCase::IgnoreCase);
	}

	bool DebugSnakePartNameMatches(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor,
		FName PartSlotId)
	{
		if (NameContainsPartSlotId(PartActor.GetName(), PartSlotId))
		{
			return true;
		}

		if (const UChildActorComponent* ChildActorComponent =
			FindChildActorComponentForPart(Host, PartActor))
		{
			return NameContainsPartSlotId(ChildActorComponent->GetName(), PartSlotId);
		}

		return false;
	}

	bool DebugSnakePartMatchesSpec(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		return PartActor.GetEffectivePartDefinitionId() == Spec.PartId
			|| PartActor.GetEffectivePartSlotId() == Spec.PartSlotId
			|| DebugSnakePartNameMatches(Host, PartActor, Spec.PartSlotId);
	}

	void ConfigureDebugSnakePartActor(
		AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		if (Spec.PartSlotId == FName(TEXT("Head")))
		{
			PartActor.ConfigureDebugSnakeHeadSample();
		}
		else if (Spec.PartSlotId == FName(TEXT("Body")))
		{
			PartActor.ConfigureDebugSnakeBodySample();
		}
		else if (Spec.PartSlotId == FName(TEXT("Tail")))
		{
			PartActor.ConfigureDebugSnakeTailSample();
		}
		else
		{
			PartActor.PartId = Spec.PartId;
			PartActor.PartSlotId = Spec.PartSlotId;
			PartActor.RefreshAuthoringState();
		}
	}

	UChildActorComponent* FindChildActorComponentForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor)
	{
		TArray<UChildActorComponent*> ChildActorComponents;
		CollectInstanceChildActorComponents(Host, ChildActorComponents);
		if (ChildActorComponents.Num() == 0)
		{
			CollectBlueprintTemplateChildActorComponents(Host, ChildActorComponents);
		}
		for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
		{
			if (ResolveChildActorComponentPartActor(ChildActorComponent, /*bAllowTemplateFallback*/true)
				== &PartActor)
			{
				return ChildActorComponent;
			}
		}
		return nullptr;
	}

	void ApplyDebugSnakePartRelativeLocation(
		const AWacomBattleEnemyActor& Host,
		AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		if (UChildActorComponent* ChildActorComponent = FindChildActorComponentForPart(Host, PartActor))
		{
			MarkObjectEditedForBattleEnemyAuthoring(ChildActorComponent);
			ChildActorComponent->SetRelativeLocation(Spec.RelativeLocation);
			return;
		}

		if (PartActor.GetAttachParentActor() == &Host)
		{
			MarkObjectEditedForBattleEnemyAuthoring(&PartActor);
			PartActor.SetActorRelativeLocation(Spec.RelativeLocation);
		}
	}

	bool PartActorMatchesDefinitionSlot(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor,
		const AWacomBattleEnemyPartActor* TemplatePartActor,
		const FEnemyPartAuthoringSlot& Slot)
	{
		const auto MatchesIdentity = [&Slot](const AWacomBattleEnemyPartActor& Candidate)
		{
			return Candidate.GetEffectivePartDefinitionId() == Slot.PartId
				|| (!Slot.PartSlotId.IsNone() && Candidate.PartSlotId == Slot.PartSlotId);
		};

		if (MatchesIdentity(PartActor) || (TemplatePartActor && MatchesIdentity(*TemplatePartActor)))
		{
			return true;
		}

		if (NameContainsDefinitionSlot(PartActor.GetName(), Slot))
		{
			return true;
		}

		if (const UChildActorComponent* ChildActorComponent =
			FindChildActorComponentForPart(Host, PartActor))
		{
			return NameContainsDefinitionSlot(ChildActorComponent->GetName(), Slot);
		}

		return false;
	}

	bool PartActorNeedsDefinitionIdentityFill(const AWacomBattleEnemyPartActor& PartActor)
	{
		return PartActor.PartId.IsNone() || PartActor.PartSlotId.IsNone();
	}

	void ApplyDefinitionIdentityToPartActor(
		AWacomBattleEnemyPartActor& PartActor,
		const FEnemyPartAuthoringSlot& Slot)
	{
		bool bChanged = false;
		if (PartActor.PartId.IsNone() && !Slot.PartId.IsNone())
		{
			PartActor.PartId = Slot.PartId;
			bChanged = true;
		}
		if (PartActor.PartSlotId.IsNone() && !Slot.PartSlotId.IsNone())
		{
			PartActor.PartSlotId = Slot.PartSlotId;
			bChanged = true;
		}

		if (bChanged)
		{
			MarkObjectEditedForBattleEnemyAuthoring(&PartActor);
			PartActor.RefreshAuthoringState();
		}
	}

	AWacomBattleEnemyPartActor* ResolveChildActorTemplateForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor)
	{
		if (UChildActorComponent* ChildActorComponent = FindChildActorComponentForPart(Host, PartActor))
		{
			return Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActorTemplate());
		}
		return nullptr;
	}

	int32 FindDefinitionSlotIndexForPart(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& PartActor,
		const AWacomBattleEnemyPartActor* TemplatePartActor,
		const TArray<FEnemyPartAuthoringSlot>& Slots,
		const TSet<int32>& UsedSlotIndices,
		bool bAllowNameMatch)
	{
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			if (UsedSlotIndices.Contains(SlotIndex))
			{
				continue;
			}

			const FEnemyPartAuthoringSlot& Slot = Slots[SlotIndex];
			const bool bIdentityMatches =
				PartActor.GetEffectivePartDefinitionId() == Slot.PartId
				|| (!Slot.PartSlotId.IsNone() && PartActor.PartSlotId == Slot.PartSlotId)
				|| (TemplatePartActor
					&& (TemplatePartActor->GetEffectivePartDefinitionId() == Slot.PartId
						|| (!Slot.PartSlotId.IsNone() && TemplatePartActor->PartSlotId == Slot.PartSlotId)));
			if (bIdentityMatches)
			{
				return SlotIndex;
			}

			if (bAllowNameMatch && PartActorMatchesDefinitionSlot(Host, PartActor, TemplatePartActor, Slot))
			{
				return SlotIndex;
			}
		}

		return INDEX_NONE;
	}

	FName BuildHostAuthoringStateName(
		const UEnemyDefinition* EnemyDefinition,
		int32 PartActorCount,
		bool bHasDuplicatePartSlotIds,
		bool bHasPartSlotMismatch,
		bool bHasPartDefinitionMismatch)
	{
		if (!EnemyDefinition)
		{
			return TEXT("MissingEnemyDefinition");
		}
		if (PartActorCount <= 0)
		{
			return TEXT("NoPartActors");
		}
		if (bHasDuplicatePartSlotIds)
		{
			return TEXT("DuplicatePartSlotIds");
		}
		if (bHasPartSlotMismatch)
		{
			return TEXT("PartSlotMismatch");
		}
		if (bHasPartDefinitionMismatch)
		{
			return TEXT("PartDefinitionMismatch");
		}
		return TEXT("Ready");
	}
}

AWacomBattleEnemyActor::AWacomBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	HostVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HostVisualRoot"));
	HostVisualRoot->SetupAttachment(SceneRoot);
	HostVisualRoot->SetRelativeLocation(FVector::ZeroVector);
	HostVisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
	HostVisualRoot->SetRelativeScale3D(FVector::OneVector);
	HostVisualRoot->bEditableWhenInherited = false;
}

void AWacomBattleEnemyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshHostVisual();
	RefreshBattleEnemyPartAuthoringState();
}

void AWacomBattleEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshHostVisual();
	SyncHostIdentityToPartActors();
	RefreshAuthoringStatusPreview();
}

bool AWacomBattleEnemyActor::HasHostVisualResource() const
{
	switch (HostVisualMode)
	{
	case EWacomBattleEnemyHostVisualMode::Flipbook:
		return HostFlipbook != nullptr;
	case EWacomBattleEnemyHostVisualMode::StaticSprite:
	default:
		return HostSprite != nullptr;
	}
}

bool AWacomBattleEnemyActor::IsHostVisualActive() const
{
	return bHostVisualVisible && HasHostVisualResource();
}

void AWacomBattleEnemyActor::RefreshHostVisual()
{
	if (GeneratedHostSpriteVisualComponent)
	{
		GeneratedHostSpriteVisualComponent->DestroyComponent();
		GeneratedHostSpriteVisualComponent = nullptr;
	}
	if (GeneratedHostFlipbookVisualComponent)
	{
		GeneratedHostFlipbookVisualComponent->DestroyComponent();
		GeneratedHostFlipbookVisualComponent = nullptr;
	}

	if (!IsHostVisualActive())
	{
		return;
	}

	USceneComponent* AttachParent = HostVisualRoot.Get();
	if (!AttachParent)
	{
		AttachParent = SceneRoot.Get();
	}
	if (!AttachParent)
	{
		AttachParent = RootComponent;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (HostVisualMode == EWacomBattleEnemyHostVisualMode::Flipbook)
	{
		UPaperFlipbookComponent* FlipbookComponent =
			NewObject<UPaperFlipbookComponent>(
				this,
				TEXT("HostVisual_Flipbook"),
				RF_Transactional | RF_Transient);
		if (!FlipbookComponent)
		{
			return;
		}

		FlipbookComponent->SetupAttachment(AttachParent);
		FlipbookComponent->SetFlipbook(HostFlipbook);
		FlipbookComponent->SetRelativeLocation(HostVisualRelativeLocation);
		FlipbookComponent->SetRelativeRotation(HostVisualRelativeRotation);
		FlipbookComponent->SetRelativeScale3D(HostVisualRelativeScale3D);
		FlipbookComponent->SetSpriteColor(HostVisualTint);
		FlipbookComponent->SetTranslucentSortPriority(HostVisualSortOrder);
		FlipbookComponent->SetVisibility(bHostVisualVisible, true);
		FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FlipbookComponent->SetGenerateOverlapEvents(false);
		FlipbookComponent->SetLooping(bLoopHostFlipbook);
		FlipbookComponent->SetPlayRate(HostFlipbookPlayRate);
		FlipbookComponent->SetPlaybackPosition(HostFlipbookStartTimeSeconds, false);
		if (bAutoPlayHostFlipbook && HostFlipbookPlayRate > 0.0f)
		{
			FlipbookComponent->Play();
		}
		else
		{
			FlipbookComponent->Stop();
		}
		FlipbookComponent->bEditableWhenInherited = false;
		AddInstanceComponent(FlipbookComponent);
		FlipbookComponent->RegisterComponentWithWorld(World);
		GeneratedHostFlipbookVisualComponent = FlipbookComponent;
		return;
	}

	UPaperSpriteComponent* SpriteComponent =
		NewObject<UPaperSpriteComponent>(
			this,
			TEXT("HostVisual_Sprite"),
			RF_Transactional | RF_Transient);
	if (!SpriteComponent)
	{
		return;
	}

	SpriteComponent->SetupAttachment(AttachParent);
	SpriteComponent->SetSprite(HostSprite);
	SpriteComponent->SetRelativeLocation(HostVisualRelativeLocation);
	SpriteComponent->SetRelativeRotation(HostVisualRelativeRotation);
	SpriteComponent->SetRelativeScale3D(HostVisualRelativeScale3D);
	SpriteComponent->SetSpriteColor(HostVisualTint);
	SpriteComponent->SetTranslucentSortPriority(HostVisualSortOrder);
	SpriteComponent->SetVisibility(bHostVisualVisible, true);
	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteComponent->SetGenerateOverlapEvents(false);
	SpriteComponent->bEditableWhenInherited = false;
	AddInstanceComponent(SpriteComponent);
	SpriteComponent->RegisterComponentWithWorld(World);
	GeneratedHostSpriteVisualComponent = SpriteComponent;
}

FName AWacomBattleEnemyActor::GetHostVisualModeDebugName() const
{
	return IsHostVisualActive()
		? FName(GetHostVisualModeDebugString(HostVisualMode))
		: NAME_None;
}

FName AWacomBattleEnemyActor::GetHostVisualAssetName() const
{
	if (!IsHostVisualActive())
	{
		return NAME_None;
	}

	switch (HostVisualMode)
	{
	case EWacomBattleEnemyHostVisualMode::Flipbook:
		return HostFlipbook ? FName(*HostFlipbook->GetName()) : NAME_None;
	case EWacomBattleEnemyHostVisualMode::StaticSprite:
	default:
		return HostSprite ? FName(*HostSprite->GetName()) : NAME_None;
	}
}

int32 AWacomBattleEnemyActor::GetGeneratedHostVisualComponentCount() const
{
	return (GeneratedHostSpriteVisualComponent ? 1 : 0)
		+ (GeneratedHostFlipbookVisualComponent ? 1 : 0);
}

int32 AWacomBattleEnemyActor::GetRegisteredHostVisualComponentCount() const
{
	return (GeneratedHostSpriteVisualComponent && GeneratedHostSpriteVisualComponent->IsRegistered() ? 1 : 0)
		+ (GeneratedHostFlipbookVisualComponent && GeneratedHostFlipbookVisualComponent->IsRegistered() ? 1 : 0);
}

int32 AWacomBattleEnemyActor::GetVisibleHostVisualComponentCount() const
{
	return (GeneratedHostSpriteVisualComponent && GeneratedHostSpriteVisualComponent->IsVisible() ? 1 : 0)
		+ (GeneratedHostFlipbookVisualComponent && GeneratedHostFlipbookVisualComponent->IsVisible() ? 1 : 0);
}

FName AWacomBattleEnemyActor::GetEffectiveEnemySlotId() const
{
	return EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : EnemySlotId;
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::BuildAttachedBattleEnemyPartActors() const
{
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);

	TArray<AWacomBattleEnemyPartActor*> PartActors;
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AWacomBattleEnemyPartActor* PartActor = Cast<AWacomBattleEnemyPartActor>(AttachedActor))
		{
			PartActors.AddUnique(PartActor);
		}
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	bool bAllowTemplateFallback = false;
	CollectChildActorComponentsForPartDiscovery(*this, ChildActorComponents, bAllowTemplateFallback);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		if (!ChildActorComponent)
		{
			continue;
		}

		if (AWacomBattleEnemyPartActor* PartActor =
			ResolveChildActorComponentPartActor(ChildActorComponent, bAllowTemplateFallback))
		{
			PartActors.AddUnique(PartActor);
		}
	}

	const TMap<FName, int32> DefinitionPartOrder = BuildDefinitionPartOrder(EnemyDefinition);
	PartActors.Sort([&DefinitionPartOrder](
		const AWacomBattleEnemyPartActor& Left,
		const AWacomBattleEnemyPartActor& Right)
	{
		const int32* LeftDefinitionIndex = DefinitionPartOrder.Find(Left.GetEffectivePartDefinitionId());
		const int32* RightDefinitionIndex = DefinitionPartOrder.Find(Right.GetEffectivePartDefinitionId());
		const int32 LeftRank = LeftDefinitionIndex ? *LeftDefinitionIndex : MAX_int32;
		const int32 RightRank = RightDefinitionIndex ? *RightDefinitionIndex : MAX_int32;
		if (LeftRank != RightRank)
		{
			return LeftRank < RightRank;
		}

		return BuildPartSortKey(&Left) < BuildPartSortKey(&Right);
	});
	return PartActors;
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::GetBattleEnemyPartActors() const
{
	return BuildAttachedBattleEnemyPartActors();
}

void AWacomBattleEnemyActor::SyncHostIdentityToPartActors() const
{
	const FName EffectiveEnemySlotId = GetEffectiveEnemySlotId();
	const bool bHostVisualActive = IsHostVisualActive();
	for (AWacomBattleEnemyPartActor* PartActor : GetBattleEnemyPartActors())
	{
		if (PartActor)
		{
			PartActor->SetEnemySlotId(EffectiveEnemySlotId);
			PartActor->SetHostVisualContext(bHostVisualActive);
		}
	}
}

void AWacomBattleEnemyActor::SyncDefinitionIdentityToPartActors() const
{
	const TArray<FEnemyPartAuthoringSlot> Slots = BuildDefinitionAuthoringSlots(EnemyDefinition);
	if (Slots.Num() == 0)
	{
		return;
	}

	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	if (PartActors.Num() == 0)
	{
		return;
	}

	TSet<int32> UsedSlotIndices;
	TArray<AWacomBattleEnemyPartActor*> EmptyIdentityPartActors;
	for (AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (!PartActor)
		{
			continue;
		}

		AWacomBattleEnemyPartActor* TemplatePartActor = ResolveChildActorTemplateForPart(*this, *PartActor);
		const bool bPartActorNeedsFill = PartActorNeedsDefinitionIdentityFill(*PartActor);
		const bool bTemplateNeedsFill = TemplatePartActor && PartActorNeedsDefinitionIdentityFill(*TemplatePartActor);
		if (bPartActorNeedsFill || bTemplateNeedsFill)
		{
			EmptyIdentityPartActors.Add(PartActor);
			continue;
		}

		const int32 SlotIndex = FindDefinitionSlotIndexForPart(
			*this,
			*PartActor,
			TemplatePartActor,
			Slots,
			UsedSlotIndices,
			/*bAllowNameMatch*/ false);
		if (SlotIndex != INDEX_NONE)
		{
			UsedSlotIndices.Add(SlotIndex);
		}
	}

	for (AWacomBattleEnemyPartActor* PartActor : EmptyIdentityPartActors)
	{
		if (!PartActor)
		{
			continue;
		}

		AWacomBattleEnemyPartActor* TemplatePartActor = ResolveChildActorTemplateForPart(*this, *PartActor);
		int32 SlotIndex = FindDefinitionSlotIndexForPart(
			*this,
			*PartActor,
			TemplatePartActor,
			Slots,
			UsedSlotIndices,
			/*bAllowNameMatch*/ true);
		if (SlotIndex == INDEX_NONE)
		{
			for (int32 CandidateIndex = 0; CandidateIndex < Slots.Num(); ++CandidateIndex)
			{
				if (!UsedSlotIndices.Contains(CandidateIndex))
				{
					SlotIndex = CandidateIndex;
					break;
				}
			}
		}
		if (SlotIndex == INDEX_NONE)
		{
			continue;
		}

		UsedSlotIndices.Add(SlotIndex);
		const FEnemyPartAuthoringSlot& Slot = Slots[SlotIndex];
		ApplyDefinitionIdentityToPartActor(*PartActor, Slot);
		if (TemplatePartActor && TemplatePartActor != PartActor)
		{
			ApplyDefinitionIdentityToPartActor(*TemplatePartActor, Slot);
		}
	}
}

void AWacomBattleEnemyActor::RefreshBattleEnemyPartAuthoringState() const
{
	const_cast<AWacomBattleEnemyActor*>(this)->RefreshHostVisual();
	SyncDefinitionIdentityToPartActors();
	SyncHostIdentityToPartActors();
	for (AWacomBattleEnemyPartActor* PartActor : GetBattleEnemyPartActors())
	{
		if (PartActor)
		{
			PartActor->RefreshAuthoringState();
		}
	}
	const_cast<AWacomBattleEnemyActor*>(this)->RefreshAuthoringStatusPreview();
}

void AWacomBattleEnemyActor::RefreshAttachedPartBadgeLayout() const
{
	RefreshBattleEnemyPartAuthoringState();
	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	const float CenterIndex = PartActors.Num() > 0
		? (static_cast<float>(PartActors.Num() - 1) * 0.5f)
		: 0.0f;

	for (int32 Index = 0; Index < PartActors.Num(); ++Index)
	{
		AWacomBattleEnemyPartActor* PartActor = PartActors[Index];
		if (!PartActor)
		{
			continue;
		}

		FVector StaggerOffset = FVector::ZeroVector;
		int32 StaggerIndex = INDEX_NONE;
		if (bApplyAttachedPartBadgeStagger)
		{
			const float RelativeIndex = static_cast<float>(Index) - CenterIndex;
			StaggerOffset = FVector(
				0.0f,
				RelativeIndex * BadgeStaggerHorizontalStep,
				FMath::Abs(RelativeIndex) * BadgeStaggerVerticalStep);
			StaggerIndex = Index;
		}
		PartActor->SetBadgeLayoutStagger(StaggerIndex, StaggerOffset);
	}
}

void AWacomBattleEnemyActor::ConfigureDebugSnakeHostSample()
{
	MarkObjectEditedForBattleEnemyAuthoring(this);
	MarkObjectEditedForBattleEnemyAuthoring(SceneRoot);
	EnemyDefinition = LoadObject<UEnemyDefinition>(nullptr, DebugSnakeEnemyPath);
	EnemySlotId = TEXT("Enemy");
	bApplyAttachedPartBadgeStagger = true;
	BadgeStaggerHorizontalStep = 28.0f;
	BadgeStaggerVerticalStep = 18.0f;

	const TArray<FDebugSnakeHostPartSpec>& Specs = GetDebugSnakeHostPartSpecs();
	TArray<AWacomBattleEnemyPartActor*> CandidatePartActors = GetBattleEnemyPartActors();
	TArray<AWacomBattleEnemyPartActor*> AssignedPartActors;
	AssignedPartActors.SetNumZeroed(Specs.Num());
	TSet<AWacomBattleEnemyPartActor*> UsedPartActors;

	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		const FDebugSnakeHostPartSpec& Spec = Specs[SpecIndex];
		for (AWacomBattleEnemyPartActor* PartActor : CandidatePartActors)
		{
			if (!PartActor || UsedPartActors.Contains(PartActor))
			{
				continue;
			}

			if (DebugSnakePartMatchesSpec(*this, *PartActor, Spec))
			{
				AssignedPartActors[SpecIndex] = PartActor;
				UsedPartActors.Add(PartActor);
				break;
			}
		}
	}

	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		if (AssignedPartActors[SpecIndex])
		{
			continue;
		}

		for (AWacomBattleEnemyPartActor* PartActor : CandidatePartActors)
		{
			if (PartActor && !UsedPartActors.Contains(PartActor))
			{
				AssignedPartActors[SpecIndex] = PartActor;
				UsedPartActors.Add(PartActor);
				break;
			}
		}
	}

	for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); ++SpecIndex)
	{
		AWacomBattleEnemyPartActor* PartActor = AssignedPartActors[SpecIndex];
		if (!PartActor)
		{
			continue;
		}

		const FDebugSnakeHostPartSpec& Spec = Specs[SpecIndex];
		MarkObjectEditedForBattleEnemyAuthoring(PartActor);
		ConfigureDebugSnakePartActor(*PartActor, Spec);
		ApplyDebugSnakePartRelativeLocation(*this, *PartActor, Spec);

	}

	RefreshAttachedPartBadgeLayout();
	RefreshAuthoringStatusPreview();
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugView() const
{
	return GetBattleSceneEnemyDebugViewForHUD(nullptr);
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugViewForHUD(
	const UBattleHUD* HUD) const
{
	FWacomBattleSceneEnemyDebugView View;
	View.ActorName = GetName();
	View.EnemyDefinitionName = EnemyDefinition ? FName(*EnemyDefinition->GetName()) : NAME_None;
	View.EnemyId = EnemyDefinition ? EnemyDefinition->EnemyId : NAME_None;
	View.EnemySlotId = GetEffectiveEnemySlotId();
	View.HostVisualMode = GetHostVisualModeDebugName();
	View.bUsingHostVisual = IsHostVisualActive();
	View.HostVisualAssetName = GetHostVisualAssetName();
	View.GeneratedHostVisualComponentCount = GetGeneratedHostVisualComponentCount();
	View.RegisteredHostVisualComponentCount = GetRegisteredHostVisualComponentCount();
	View.VisibleHostVisualComponentCount = GetVisibleHostVisualComponentCount();
	View.bUsedByBattleHUD = HUD && HUD->IsBattleSceneEnemyHostInCurrentRegistry(this);
	View.ActiveBattleHUDName = HUD ? HUD->GetName() : TEXT("None");

	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	View.AttachedPartActorCount = PartActors.Num();
	View.AttachedPartIds.Reserve(PartActors.Num());
	for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (PartActor)
		{
			const FWacomBattleSceneEnemyPartDebugView PartView =
				PartActor->GetBattleSceneEnemyPartDebugView();
			View.AttachedPartIds.Add(PartActor->GetEffectivePartDefinitionId());
			View.AttachedPartSlotIds.Add(PartActor->GetEffectivePartSlotId());
			View.StableSceneTargetIds.Add(PartActor->GetStableSceneTargetId());
			if (PartView.BridgeDebugView.bBoundToSnapshot)
			{
				++View.BoundPartActorCount;
			}
			else
			{
				++View.UnboundPartActorCount;
			}
			if (PartView.BridgeDebugView.bHasRuntimePartFacts)
			{
				++View.RuntimeFactsPartActorCount;
				View.RuntimeInitiativeTotal += PartView.BridgeDebugView.CurrentInitiative;
			}
			if (PartView.BridgeDebugView.bHoverActive)
			{
				++View.HoveredPartActorCount;
			}
			if (PartView.BridgeDebugView.PredictionView.bVisible)
			{
				++View.PredictionVisiblePartActorCount;
			}
			if (PartView.BridgeDebugView.StatusBadgeView.bVisible)
			{
				++View.StatusBadgeVisiblePartActorCount;
			}
			if (PartView.BadgeLayoutStaggerIndex != INDEX_NONE)
			{
				++View.BadgeLayoutAppliedPartActorCount;
			}
		}
	}
	View.UnknownPartIds = BuildUnknownPartIds();
	View.UnknownPartSlotIds = BuildUnknownPartSlotIds();
	View.MissingDefinitionPartIds = BuildMissingDefinitionPartIds();
	View.MissingDefinitionPartSlotIds = BuildMissingDefinitionPartSlotIds();
	View.DuplicatePartSlotIds = BuildDuplicateConfiguredPartSlotIds();
	View.AuthoringState = BuildHostAuthoringStateName(
		EnemyDefinition,
		View.AttachedPartActorCount,
		View.DuplicatePartSlotIds.Num() > 0,
		View.UnknownPartSlotIds.Num() > 0 || View.MissingDefinitionPartSlotIds.Num() > 0,
		View.UnknownPartIds.Num() > 0 || View.MissingDefinitionPartIds.Num() > 0);
	View.bAuthoringReady = View.AuthoringState == FName(TEXT("Ready"));
	return View;
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummary() const
{
	return GetBattleSceneEnemyDebugSummaryForHUD(nullptr);
}

void AWacomBattleEnemyActor::RefreshAuthoringStatusPreview()
{
	const FWacomBattleSceneEnemyDebugView View = GetBattleSceneEnemyDebugView();
	AuthoringState = View.AuthoringState;
	bAuthoringReady = View.bAuthoringReady;
	AuthoringHostVisualMode = View.HostVisualMode;
	bAuthoringUsingHostVisual = View.bUsingHostVisual;
	AuthoringHostVisualAssetName = View.HostVisualAssetName;
	AuthoringGeneratedHostVisualComponentCount = View.GeneratedHostVisualComponentCount;
	AuthoringRegisteredHostVisualComponentCount = View.RegisteredHostVisualComponentCount;
	AuthoringVisibleHostVisualComponentCount = View.VisibleHostVisualComponentCount;
	AuthoringPartActorCount = View.AttachedPartActorCount;
	AuthoringPartIds = View.AttachedPartIds;
	AuthoringPartSlotIds = View.AttachedPartSlotIds;
	AuthoringStableSceneTargetIds = View.StableSceneTargetIds;
	AuthoringUnknownPartSlotIds = View.UnknownPartSlotIds;
	AuthoringMissingDefinitionPartSlotIds = View.MissingDefinitionPartSlotIds;
	AuthoringDuplicatePartSlotIds = View.DuplicatePartSlotIds;
	AuthoringDebugSummary = GetBattleSceneEnemyDebugSummary();
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummaryForHUD(const UBattleHUD* HUD) const
{
	const FWacomBattleSceneEnemyDebugView View = GetBattleSceneEnemyDebugViewForHUD(HUD);
	return FString::Printf(
		TEXT("BattleSceneEnemy{Actor=%s Definition=%s EnemyId=%s EnemySlotId=%s HostVisualMode=%s UsingHostVisual=%s HostVisualAsset=%s GeneratedHostVisualComponents=%d RegisteredHostVisualComponents=%d VisibleHostVisualComponents=%d AuthoringState=%s AuthoringReady=%s PartCount=%d BoundParts=%d UnboundParts=%d RuntimeFacts=%d RuntimeInitiativeTotal=%d HoveredParts=%d PredictionVisibleParts=%d StatusBadgeVisibleParts=%d BadgeLayoutAppliedParts=%d UsedByBattleHUD=%s ActiveBattleHUD=%s PartIds=[%s] PartSlotIds=[%s] StableSceneTargets=[%s] UnknownPartIds=[%s] UnknownPartSlotIds=[%s] MissingDefinitionPartIds=[%s] MissingDefinitionPartSlotIds=[%s] DuplicatePartSlotIds=[%s]}"),
		*View.ActorName,
		*View.EnemyDefinitionName.ToString(),
		*View.EnemyId.ToString(),
		*View.EnemySlotId.ToString(),
		*View.HostVisualMode.ToString(),
		View.bUsingHostVisual ? TEXT("true") : TEXT("false"),
		*View.HostVisualAssetName.ToString(),
		View.GeneratedHostVisualComponentCount,
		View.RegisteredHostVisualComponentCount,
		View.VisibleHostVisualComponentCount,
		*View.AuthoringState.ToString(),
		View.bAuthoringReady ? TEXT("true") : TEXT("false"),
		View.AttachedPartActorCount,
		View.BoundPartActorCount,
		View.UnboundPartActorCount,
		View.RuntimeFactsPartActorCount,
		View.RuntimeInitiativeTotal,
		View.HoveredPartActorCount,
		View.PredictionVisiblePartActorCount,
		View.StatusBadgeVisiblePartActorCount,
		View.BadgeLayoutAppliedPartActorCount,
		View.bUsedByBattleHUD ? TEXT("true") : TEXT("false"),
		*View.ActiveBattleHUDName,
		*JoinNames(View.AttachedPartIds),
		*JoinNames(View.AttachedPartSlotIds),
		*JoinNames(View.StableSceneTargetIds),
		*JoinNames(View.UnknownPartIds),
		*JoinNames(View.UnknownPartSlotIds),
		*JoinNames(View.MissingDefinitionPartIds),
		*JoinNames(View.MissingDefinitionPartSlotIds),
		*JoinNames(View.DuplicatePartSlotIds));
}

void AWacomBattleEnemyActor::LogBattleSceneEnemyDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyActor] %s"),
		*GetBattleSceneEnemyDebugSummary());
}

#if WITH_EDITOR
void AWacomBattleEnemyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshBattleEnemyPartAuthoringState();
}

EDataValidationResult AWacomBattleEnemyActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidateBattleEnemyHostPlacementActor(*this))
	{
		return Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	RefreshBattleEnemyPartAuthoringState();
	const TArray<AWacomBattleEnemyPartActor*> PartActors = GetBattleEnemyPartActors();
	if (!EnemyDefinition)
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingEnemyDefinition",
				"BattleEnemy Host 摆放配置错误：Actor={0} 缺少 EnemyDefinition；无法校验子 PartActor 的 PartId / PartSlotId。"),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (PartActors.Num() == 0)
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementNoAttachedParts",
				"BattleEnemy Host 摆放配置错误：Actor={0} 没有配置任何 BattleEnemyPartActor；无法注册场景敌人部位目标。"),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	const TArray<FName> DuplicatePartSlotIds = BuildDuplicateConfiguredPartSlotIds();
	if (DuplicatePartSlotIds.Num() > 0)
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementDuplicatePartSlotIds",
				"BattleEnemy Host 摆放配置错误：Actor={0} 下有重复 PartSlotId：{1}。"),
			FText::FromString(GetName()),
			FText::FromString(JoinNames(DuplicatePartSlotIds))));
		Result = EDataValidationResult::Invalid;
	}

	const TArray<FName> UnknownPartIds = BuildUnknownPartIds();
	if (EnemyDefinition && UnknownPartIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementUnknownPartIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 下有未在定义中声明的 PartId：{2}。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(UnknownPartIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> UnknownPartSlotIds = BuildUnknownPartSlotIds();
	if (EnemyDefinition && UnknownPartSlotIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementUnknownPartSlotIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 下有未在定义中声明的 PartSlotId：{2}。请确认 Host 子 PartActor 的 PartSlotId 是否对应 EnemyDefinition.Parts[].PartSlotId。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(UnknownPartSlotIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> MissingDefinitionPartIds = BuildMissingDefinitionPartIds();
	if (EnemyDefinition && MissingDefinitionPartIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementMissingDefinitionPartIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 中有未映射到 Host 的 PartId：{2}。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(MissingDefinitionPartIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	const TArray<FName> MissingDefinitionPartSlotIds = BuildMissingDefinitionPartSlotIds();
	if (EnemyDefinition && MissingDefinitionPartSlotIds.Num() > 0)
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementMissingDefinitionPartSlotIds",
				"BattleEnemy Host 摆放警告：Actor={0} EnemyDefinition={1} 中有未映射到 Host 的 PartSlotId：{2}。对应部位无法按 EnemySlotId + PartSlotId 绑定场景目标。"),
			FText::FromString(GetName()),
			FText::FromString(EnemyDefinition->GetName()),
			FText::FromString(JoinNames(MissingDefinitionPartSlotIds))));
		Result = Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	if (!IsHostVisualActive() && PartActors.Num() > 0)
	{
		bool bAnyPartHasVisibleResource = false;
		for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
		{
			if (!PartActor)
			{
				continue;
			}

			const FName PartVisualMode =
				PartActor->GetBattleSceneEnemyPartDebugView().VisualAuthoringMode;
			if (PartVisualMode == FName(TEXT("VisualLayers"))
				|| PartVisualMode == FName(TEXT("LegacyPrototype")))
			{
				bAnyPartHasVisibleResource = true;
				break;
			}
		}

		if (!bAnyPartHasVisibleResource)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementMissingAnyVisualResource",
					"BattleEnemy Host 摆放警告：Actor={0} 没有 Host 整体视觉，子 PartActor 也没有 VisualLayers / LegacyPrototype 可见资源；该敌人只有命中体和调试信息可见。"),
				FText::FromString(GetName())));
			Result = Result == EDataValidationResult::Invalid
				? EDataValidationResult::Invalid
				: EDataValidationResult::Valid;
		}
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

TSet<FName> AWacomBattleEnemyActor::BuildDefinitionPartIdSet() const
{
	TSet<FName> PartIds;
	if (!EnemyDefinition)
	{
		return PartIds;
	}

	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		if (PartSlot.PartDef && !PartSlot.PartDef->PartId.IsNone())
		{
			PartIds.Add(PartSlot.PartDef->PartId);
		}
	}
	return PartIds;
}

TSet<FName> AWacomBattleEnemyActor::BuildDefinitionPartSlotIdSet() const
{
	TSet<FName> PartSlotIds;
	if (!EnemyDefinition)
	{
		return PartSlotIds;
	}

	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		const FName PartSlotId = BuildDefinitionPartSlotId(PartSlot);
		if (!PartSlotId.IsNone())
		{
			PartSlotIds.Add(PartSlotId);
		}
	}
	return PartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildConfiguredPartIds() const
{
	TArray<FName> PartIds;
	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (PartActor && !PartActor->GetEffectivePartDefinitionId().IsNone())
		{
			PartIds.AddUnique(PartActor->GetEffectivePartDefinitionId());
		}
	}
	return PartIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildConfiguredPartSlotIds() const
{
	TArray<FName> PartSlotIds;
	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (PartActor && !PartActor->GetEffectivePartSlotId().IsNone())
		{
			PartSlotIds.AddUnique(PartActor->GetEffectivePartSlotId());
		}
	}
	return PartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildUnknownPartIds() const
{
	TArray<FName> UnknownPartIds;
	if (!EnemyDefinition)
	{
		return UnknownPartIds;
	}

	const TSet<FName> DefinitionPartIds = BuildDefinitionPartIdSet();
	for (const FName& PartId : BuildConfiguredPartIds())
	{
		if (!DefinitionPartIds.Contains(PartId))
		{
			UnknownPartIds.AddUnique(PartId);
		}
	}
	return UnknownPartIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildUnknownPartSlotIds() const
{
	TArray<FName> UnknownPartSlotIds;
	if (!EnemyDefinition)
	{
		return UnknownPartSlotIds;
	}

	const TSet<FName> DefinitionPartSlotIds = BuildDefinitionPartSlotIdSet();
	for (const FName& PartSlotId : BuildConfiguredPartSlotIds())
	{
		if (!DefinitionPartSlotIds.Contains(PartSlotId))
		{
			UnknownPartSlotIds.AddUnique(PartSlotId);
		}
	}
	return UnknownPartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildMissingDefinitionPartIds() const
{
	TArray<FName> MissingPartIds;
	if (!EnemyDefinition)
	{
		return MissingPartIds;
	}

	const TArray<FName> ConfiguredPartIds = BuildConfiguredPartIds();
	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		if (!PartSlot.PartDef || PartSlot.PartDef->PartId.IsNone())
		{
			continue;
		}

		if (!ConfiguredPartIds.Contains(PartSlot.PartDef->PartId))
		{
			MissingPartIds.AddUnique(PartSlot.PartDef->PartId);
		}
	}
	return MissingPartIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildMissingDefinitionPartSlotIds() const
{
	TArray<FName> MissingPartSlotIds;
	if (!EnemyDefinition)
	{
		return MissingPartSlotIds;
	}

	const TArray<FName> ConfiguredPartSlotIds = BuildConfiguredPartSlotIds();
	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		const FName PartSlotId = BuildDefinitionPartSlotId(PartSlot);
		if (PartSlotId.IsNone())
		{
			continue;
		}

		if (!ConfiguredPartSlotIds.Contains(PartSlotId))
		{
			MissingPartSlotIds.AddUnique(PartSlotId);
		}
	}
	return MissingPartSlotIds;
}

TArray<FName> AWacomBattleEnemyActor::BuildDuplicateConfiguredPartSlotIds() const
{
	TArray<FName> DuplicatePartSlotIds;
	TSet<FName> SeenPartSlotIds;

	for (const AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (!PartActor)
		{
			continue;
		}

		const FName PartSlotId = PartActor->GetEffectivePartSlotId();
		if (PartSlotId.IsNone())
		{
			continue;
		}

		if (SeenPartSlotIds.Contains(PartSlotId))
		{
			DuplicatePartSlotIds.AddUnique(PartSlotId);
		}
		else
		{
			SeenPartSlotIds.Add(PartSlotId);
		}
	}
	return DuplicatePartSlotIds;
}

#undef LOCTEXT_NAMESPACE
