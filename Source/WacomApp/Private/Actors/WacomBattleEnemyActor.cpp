// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyHostAnimationStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomBattleEnemyHostVisualComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "Blueprint/UserWidget.h"

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#endif

namespace
{
	const TCHAR* DebugSnakeEnemyPath =
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake");

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

	bool DebugSnakePartIdentityMatchesSpec(
		const AWacomBattleEnemyPartActor& PartActor,
		const FDebugSnakeHostPartSpec& Spec)
	{
		return PartActor.GetEffectivePartDefinitionId() == Spec.PartId
			|| PartActor.GetEffectivePartSlotId() == Spec.PartSlotId;
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

	FName BuildEnemyPartComponentBaseName(FName PartSlotId)
	{
		FString SanitizedSlotId = PartSlotId.ToString();
		for (TCHAR& Character : SanitizedSlotId)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}
		if (SanitizedSlotId.IsEmpty())
		{
			SanitizedSlotId = TEXT("Part");
		}
		return FName(*FString::Printf(TEXT("EnemyPart_%s"), *SanitizedSlotId));
	}

	UBlueprint* ResolveOwningBlueprintForAuthoringTemplate(
		const AWacomBattleEnemyActor& Host)
	{
#if WITH_EDITOR
		if (!Host.IsTemplate())
		{
			return nullptr;
		}

		const UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Host.GetClass());
		return BlueprintClass
			? Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy)
			: nullptr;
#else
		return nullptr;
#endif
	}

	bool ApplyDerivedPartIdentity(
		AWacomBattleEnemyPartActor& PartActor,
		FName PartSlotId,
		FName PartId)
	{
		if (PartActor.PartSlotId == PartSlotId && PartActor.PartId == PartId)
		{
			return false;
		}

		MarkObjectEditedForBattleEnemyAuthoring(&PartActor);
		PartActor.PartSlotId = PartSlotId;
		PartActor.PartId = PartId;
		PartActor.RefreshAuthoringState();
		return true;
	}

	bool ApplyDerivedPartIdentityToAllRepresentations(
		const AWacomBattleEnemyActor& Host,
		AWacomBattleEnemyPartActor& PartActor,
		FName PartSlotId,
		FName PartId)
	{
		TArray<AWacomBattleEnemyPartActor*> Representations = { &PartActor };
		if (UChildActorComponent* ChildActorComponent =
			FindChildActorComponentForPart(Host, PartActor))
		{
			if (AWacomBattleEnemyPartActor* LivePart =
				Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActor()))
			{
				Representations.AddUnique(LivePart);
			}
			if (AWacomBattleEnemyPartActor* TemplatePart =
				Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActorTemplate()))
			{
				Representations.AddUnique(TemplatePart);
			}
		}

		bool bChanged = false;
		for (AWacomBattleEnemyPartActor* Representation : Representations)
		{
			if (Representation)
			{
				bChanged |= ApplyDerivedPartIdentity(
					*Representation,
					PartSlotId,
					PartId);
			}
		}
		return bChanged;
	}

	UChildActorComponent* CreateBlueprintEnemyPartChildActorComponent(
		AWacomBattleEnemyActor& Host,
		UBlueprint& Blueprint,
		FName PartSlotId,
		FName PartId)
	{
#if WITH_EDITOR
		USimpleConstructionScript* ConstructionScript =
			Blueprint.SimpleConstructionScript;
		if (!ConstructionScript)
		{
			return nullptr;
		}

		MarkObjectEditedForBattleEnemyAuthoring(&Blueprint);
		MarkObjectEditedForBattleEnemyAuthoring(ConstructionScript);
		const FName ComponentName = ConstructionScript->GenerateNewComponentName(
			UChildActorComponent::StaticClass(),
			BuildEnemyPartComponentBaseName(PartSlotId));
		USCS_Node* Node = ConstructionScript->CreateNode(
			UChildActorComponent::StaticClass(), ComponentName);
		if (!Node)
		{
			return nullptr;
		}

		ConstructionScript->AddNode(Node);
		if (Host.GetRootComponent())
		{
			Node->SetParent(Host.GetRootComponent());
		}

		UChildActorComponent* ChildActorComponent =
			Cast<UChildActorComponent>(Node->ComponentTemplate);
		if (!ChildActorComponent)
		{
			ConstructionScript->RemoveNode(Node);
			return nullptr;
		}

		MarkObjectEditedForBattleEnemyAuthoring(ChildActorComponent);
		ChildActorComponent->SetRelativeTransform(FTransform::Identity);
		ChildActorComponent->SetChildActorClass(
			AWacomBattleEnemyPartActor::StaticClass());
		if (AWacomBattleEnemyPartActor* TemplatePart =
			Cast<AWacomBattleEnemyPartActor>(
				ChildActorComponent->GetChildActorTemplate()))
		{
			ApplyDerivedPartIdentity(*TemplatePart, PartSlotId, PartId);
		}
		return ChildActorComponent;
#else
		return nullptr;
#endif
	}

	UChildActorComponent* CreateInstanceEnemyPartChildActorComponent(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		EObjectFlags ObjectFlags = RF_Transactional;
		if (Host.HasAnyFlags(RF_ArchetypeObject))
		{
			ObjectFlags |= RF_ArchetypeObject;
		}

		const FName ComponentName = MakeUniqueObjectName(
			&Host,
			UChildActorComponent::StaticClass(),
			BuildEnemyPartComponentBaseName(PartSlotId));
		UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(
			&Host,
			ComponentName,
			ObjectFlags);
		if (!ChildActorComponent)
		{
			return nullptr;
		}

		MarkObjectEditedForBattleEnemyAuthoring(&Host);
		Host.AddInstanceComponent(ChildActorComponent);
		ChildActorComponent->SetupAttachment(Host.GetRootComponent());
		ChildActorComponent->SetRelativeTransform(FTransform::Identity);
		ChildActorComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
		MarkObjectEditedForBattleEnemyAuthoring(ChildActorComponent);

		if (AWacomBattleEnemyPartActor* TemplatePart =
			Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActorTemplate()))
		{
			ApplyDerivedPartIdentity(*TemplatePart, PartSlotId, PartId);
		}

		if (!Host.IsTemplate() && Host.GetWorld())
		{
			ChildActorComponent->OnComponentCreated();
			ChildActorComponent->RegisterComponent();
		}

		if (AWacomBattleEnemyPartActor* LivePart =
			Cast<AWacomBattleEnemyPartActor>(ChildActorComponent->GetChildActor()))
		{
			ApplyDerivedPartIdentity(*LivePart, PartSlotId, PartId);
		}
		return ChildActorComponent;
	}

	UChildActorComponent* CreateEnemyPartChildActorComponent(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId,
		FName PartId)
	{
		if (UBlueprint* Blueprint =
			ResolveOwningBlueprintForAuthoringTemplate(Host))
		{
			return CreateBlueprintEnemyPartChildActorComponent(
				Host, *Blueprint, PartSlotId, PartId);
		}
		return CreateInstanceEnemyPartChildActorComponent(
			Host, PartSlotId, PartId);
	}

}

AWacomBattleEnemyActor::AWacomBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EnemyPanelWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyPanelWidget"));
	EnemyPanelWidgetComponent->SetupAttachment(SceneRoot);
	EnemyPanelWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyPanelWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyPanelWidgetComponent->SetVisibility(false, true);

	HostVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HostVisualRoot"));
	HostVisualRoot->SetupAttachment(SceneRoot);
	HostVisualRoot->SetRelativeLocation(FVector::ZeroVector);
	HostVisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
	HostVisualRoot->SetRelativeScale3D(FVector::OneVector);
	HostVisualRoot->bEditableWhenInherited = false;

	HostVisualComponent =
		CreateDefaultSubobject<UWacomBattleEnemyHostVisualComponent>(TEXT("HostVisualComponent"));
	HostVisualComponent->bEditableWhenInherited = false;
}

void AWacomBattleEnemyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshHostVisual();
	RefreshEnemyPanelWidgetComponent();
	RefreshBattleEnemyPartAuthoringState();
}

void AWacomBattleEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshHostVisual();
	RefreshEnemyPanelWidgetComponent();
	TArray<AWacomBattleEnemyPartActor*> RuntimePartActors;
	InitializeRuntimeSceneBinding(RuntimePartActors);
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

UPaperSpriteComponent* AWacomBattleEnemyActor::GetGeneratedHostSpriteVisualComponent() const
{
	return HostVisualComponent
		? HostVisualComponent->GetGeneratedHostSpriteVisualComponent()
		: nullptr;
}

UPaperFlipbookComponent* AWacomBattleEnemyActor::GetGeneratedHostFlipbookVisualComponent() const
{
	return HostVisualComponent
		? HostVisualComponent->GetGeneratedHostFlipbookVisualComponent()
		: nullptr;
}

void AWacomBattleEnemyActor::RefreshHostVisual()
{
	if (!HostVisualComponent)
	{
		return;
	}

	USceneComponent* AttachParent = HostVisualRoot.Get();
	if (!AttachParent)
	{
		AttachParent = SceneRoot.Get();
	}

	HostVisualComponent->RefreshHostVisual(
		AttachParent,
		HostVisualMode == EWacomBattleEnemyHostVisualMode::Flipbook,
		HostSprite,
		HostFlipbook,
		HostVisualRelativeLocation,
		HostVisualRelativeRotation,
		HostVisualRelativeScale3D,
		HostVisualSortOrder,
		HostVisualTint,
		HostVisualMaterialOverride,
		bHostVisualCastShadow,
		bHostVisualVisible,
		HostFlipbookPlayRate,
		bLoopHostFlipbook,
		HostFlipbookStartTimeSeconds,
		bAutoPlayHostFlipbook);
}

void AWacomBattleEnemyActor::RefreshEnemyPanelWidgetComponent()
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass =
		EnemyPanelWidgetClass ? EnemyPanelWidgetClass.Get() : UWacomBattleEnemyPanelWidget::StaticClass();
	EnemyPanelWidgetComponent->SetWidgetClass(WidgetClass);
	EnemyPanelWidgetComponent->SetRelativeLocation(EnemyPanelRelativeLocation);
	EnemyPanelWidgetComponent->SetDrawSize(EnemyPanelDrawSize);
	EnemyPanelWidgetComponent->SetVisibility(false, true);
}

FName AWacomBattleEnemyActor::GetHostVisualModeDebugName() const
{
	return IsHostVisualActive()
		? FName(WacomBattleSceneEnemyAuthoring::GetHostVisualModeDebugString(HostVisualMode))
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
	return HostVisualComponent
		? HostVisualComponent->GetGeneratedHostVisualComponentCount()
		: 0;
}

int32 AWacomBattleEnemyActor::GetRegisteredHostVisualComponentCount() const
{
	return HostVisualComponent
		? HostVisualComponent->GetRegisteredHostVisualComponentCount()
		: 0;
}

int32 AWacomBattleEnemyActor::GetVisibleHostVisualComponentCount() const
{
	return HostVisualComponent
		? HostVisualComponent->GetVisibleHostVisualComponentCount()
		: 0;
}

FName AWacomBattleEnemyActor::GetEffectiveEnemySlotId() const
{
	return EnemySlotId;
}

TArray<AWacomBattleEnemyPartActor*>
AWacomBattleEnemyActor::BuildAttachedBattleEnemyPartActors() const
{
	TArray<AWacomBattleEnemyPartActor*> PartActors;

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

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (AWacomBattleEnemyPartActor* PartActor = Cast<AWacomBattleEnemyPartActor>(AttachedActor))
		{
			PartActors.AddUnique(PartActor);
		}
	}

	const TMap<FName, int32> DefinitionPartOrder =
		WacomBattleSceneEnemyAuthoring::BuildDefinitionPartOrder(EnemyDefinition);
	PartActors.StableSort([&DefinitionPartOrder](
		const AWacomBattleEnemyPartActor& Left,
		const AWacomBattleEnemyPartActor& Right)
	{
		const int32* LeftDefinitionIndex = DefinitionPartOrder.Find(Left.GetEffectivePartDefinitionId());
		const int32* RightDefinitionIndex = DefinitionPartOrder.Find(Right.GetEffectivePartDefinitionId());
		const int32 LeftRank = LeftDefinitionIndex ? *LeftDefinitionIndex : MAX_int32;
		const int32 RightRank = RightDefinitionIndex ? *RightDefinitionIndex : MAX_int32;
		return LeftRank < RightRank;
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
			PartActor->SetHostImpactStyle(DefaultImpactStyle);
			PartActor->SetHostTargetPreviewStyle(DefaultTargetPreviewStyle);
		}
	}
}

void AWacomBattleEnemyActor::InitializeRuntimeSceneBinding(
	TArray<AWacomBattleEnemyPartActor*>& OutPartActors) const
{
	OutPartActors = BuildAttachedBattleEnemyPartActors();
	OutPartActors.RemoveAll([](const AWacomBattleEnemyPartActor* PartActor)
	{
		return !IsValid(PartActor) || PartActor->IsActorBeingDestroyed();
	});

	TArray<FVector> BadgeOffsets;
	TArray<int32> BadgeIndices;
	ApplyRuntimeBadgeLayout(OutPartActors, BadgeOffsets, BadgeIndices);

	const FName EffectiveEnemySlotId = GetEffectiveEnemySlotId();
	const bool bHostVisualActive = IsHostVisualActive();
	for (int32 Index = 0; Index < OutPartActors.Num(); ++Index)
	{
		if (AWacomBattleEnemyPartActor* PartActor = OutPartActors[Index])
		{
			PartActor->ApplyRuntimeHostContext(
				EffectiveEnemySlotId,
				bHostVisualActive,
				DefaultImpactStyle,
				DefaultTargetPreviewStyle,
				BadgeIndices[Index],
				BadgeOffsets[Index]);
		}
	}
}

void AWacomBattleEnemyActor::PlayRuntimeHostActionAnimation(
	FName IntentId,
	TFunction<void()>&& Completion)
{
	const FWacomBattleEnemyHostAnimationClip* Clip = HostAnimationStyle
		? HostAnimationStyle->ResolveActionClip(IntentId)
		: nullptr;
	if (HostAuthoringMode != EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual
		|| HostVisualMode != EWacomBattleEnemyHostVisualMode::Flipbook
		|| !IsHostVisualActive()
		|| !HostVisualComponent
		|| !Clip)
	{
		if (Completion)
		{
			Completion();
		}
		return;
	}

	HostVisualComponent->PlayRuntimeOneShot(
		Clip->Flipbook,
		Clip->PlayRate,
		IntentId,
		false,
		MoveTemp(Completion));
}

void AWacomBattleEnemyActor::PlayRuntimeHostDestroyedAnimation(
	TFunction<void()>&& Completion)
{
	const FWacomBattleEnemyHostAnimationClip* Clip = HostAnimationStyle
		? HostAnimationStyle->ResolveDestroyedClip()
		: nullptr;
	if (HostAuthoringMode != EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual
		|| HostVisualMode != EWacomBattleEnemyHostVisualMode::Flipbook
		|| !IsHostVisualActive()
		|| !HostVisualComponent
		|| !Clip)
	{
		if (Completion)
		{
			Completion();
		}
		return;
	}

	HostVisualComponent->PlayRuntimeOneShot(
		Clip->Flipbook,
		Clip->PlayRate,
		NAME_None,
		true,
		MoveTemp(Completion));
}

void AWacomBattleEnemyActor::ResetRuntimeHostAnimation()
{
	if (HostVisualComponent)
	{
		HostVisualComponent->ResetRuntimePlaybackToIdle();
	}
}

void AWacomBattleEnemyActor::CancelRuntimeHostAnimation()
{
	if (HostVisualComponent)
	{
		HostVisualComponent->CancelRuntimePlayback();
	}
}

void AWacomBattleEnemyActor::InvalidateRuntimePartTopology()
{
	++RuntimePartTopologyRevision;
}

void AWacomBattleEnemyActor::ApplyRuntimeBadgeLayout(
	const TArray<AWacomBattleEnemyPartActor*>& PartActors,
	TArray<FVector>& OutOffsets,
	TArray<int32>& OutIndices) const
{
	OutOffsets.Init(FVector::ZeroVector, PartActors.Num());
	OutIndices.Init(INDEX_NONE, PartActors.Num());
	const float CenterIndex = PartActors.Num() > 0
		? (static_cast<float>(PartActors.Num() - 1) * 0.5f)
		: 0.0f;

	if (!bApplyAttachedPartBadgeStagger)
	{
		return;
	}

	for (int32 Index = 0; Index < PartActors.Num(); ++Index)
	{
		const float RelativeIndex = static_cast<float>(Index) - CenterIndex;
		OutOffsets[Index] = FVector(
			0.0f,
			RelativeIndex * BadgeStaggerHorizontalStep,
			FMath::Abs(RelativeIndex) * BadgeStaggerVerticalStep);
		OutIndices[Index] = Index;
	}
}

void AWacomBattleEnemyActor::RefreshBattleEnemyPartAuthoringState() const
{
	const_cast<AWacomBattleEnemyActor*>(this)->RefreshHostVisual();
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

void AWacomBattleEnemyActor::SyncEnemyPartsFromDefinition()
{
	AuthoringLastAddedPartSlotIds.Reset();
	AuthoringLastUpdatedPartSlotIds.Reset();
	AuthoringLastInvalidDefinitionPartSlotIds.Reset();

#if WITH_EDITOR
	if (const UWorld* World = GetWorld(); World && World->IsGameWorld())
	{
		AuthoringLastPartSyncResult = TEXT("EditorOnly");
		return;
	}

	if (!EnemyDefinition)
	{
		AuthoringLastPartSyncResult = TEXT("MissingEnemyDefinition");
		RefreshBattleEnemyPartAuthoringState();
		return;
	}

	TMap<FName, int32> DefinitionSlotCounts;
	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		if (!PartSlot.PartSlotId.IsNone())
		{
			DefinitionSlotCounts.FindOrAdd(PartSlot.PartSlotId) += 1;
		}
	}

	TArray<const FEnemyPartSlot*> ValidDefinitionParts;
	for (const FEnemyPartSlot& PartSlot : EnemyDefinition->Parts)
	{
		const int32 SlotCount = DefinitionSlotCounts.FindRef(PartSlot.PartSlotId);
		if (PartSlot.PartSlotId.IsNone()
			|| SlotCount != 1
			|| !PartSlot.PartDef
			|| PartSlot.PartDef->PartId.IsNone())
		{
			AuthoringLastInvalidDefinitionPartSlotIds.AddUnique(PartSlot.PartSlotId);
			continue;
		}
		ValidDefinitionParts.Add(&PartSlot);
	}

	if (ValidDefinitionParts.IsEmpty())
	{
		AuthoringLastPartSyncResult = TEXT("NoValidDefinitionParts");
		RefreshBattleEnemyPartAuthoringState();
		return;
	}

	TMap<FName, AWacomBattleEnemyPartActor*> FirstPartBySlotId;
	for (AWacomBattleEnemyPartActor* PartActor : GetBattleEnemyPartActors())
	{
		if (!PartActor || PartActor->PartSlotId.IsNone())
		{
			continue;
		}
		FirstPartBySlotId.FindOrAdd(PartActor->PartSlotId, PartActor);
	}

	bool bChanged = false;
	bool bAddedPart = false;
	for (const FEnemyPartSlot* PartSlot : ValidDefinitionParts)
	{
		if (!PartSlot || !PartSlot->PartDef)
		{
			continue;
		}

		if (AWacomBattleEnemyPartActor** ExistingPart =
			FirstPartBySlotId.Find(PartSlot->PartSlotId))
		{
			if (*ExistingPart
				&& ApplyDerivedPartIdentityToAllRepresentations(
					*this,
					**ExistingPart,
					PartSlot->PartSlotId,
					PartSlot->PartDef->PartId))
			{
				bChanged = true;
				AuthoringLastUpdatedPartSlotIds.Add(PartSlot->PartSlotId);
			}
			continue;
		}

		if (CreateEnemyPartChildActorComponent(
			*this,
			PartSlot->PartSlotId,
			PartSlot->PartDef->PartId))
		{
			bChanged = true;
			bAddedPart = true;
			AuthoringLastAddedPartSlotIds.Add(PartSlot->PartSlotId);
		}
	}

	if (bAddedPart)
	{
		InvalidateRuntimePartTopology();
	}
	RefreshAttachedPartBadgeLayout();
	AuthoringLastPartSyncResult = !AuthoringLastInvalidDefinitionPartSlotIds.IsEmpty()
		? FName(TEXT("AppliedWithInvalidDefinitionSlots"))
		: (bChanged ? FName(TEXT("Applied")) : FName(TEXT("NoChanges")));
	if (UBlueprint* Blueprint = ResolveOwningBlueprintForAuthoringTemplate(*this))
	{
		if (bAddedPart)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		else if (bChanged)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
	}
#else
	AuthoringLastPartSyncResult = TEXT("EditorOnly");
#endif
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

			if (DebugSnakePartIdentityMatchesSpec(*PartActor, Spec))
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
	View.AuthoringMode = WacomBattleSceneEnemyAuthoring::GetHostAuthoringModeDebugString(
		HostAuthoringMode);
	View.HostVisualMode = GetHostVisualModeDebugName();
	View.bUsingHostVisual = IsHostVisualActive();
	View.HostVisualAssetName = GetHostVisualAssetName();
	View.HostAnimationStyleAssetName = HostAnimationStyle
		? FName(*HostAnimationStyle->GetName())
		: NAME_None;
	if (HostVisualComponent)
	{
		View.CurrentHostAnimationClipName = HostVisualComponent->GetCurrentRuntimeClipName();
		View.CurrentHostAnimationIntentId = HostVisualComponent->GetCurrentRuntimeIntentId();
		View.bHostAnimationPlaybackActive = HostVisualComponent->IsRuntimePlaybackActive();
		View.bHostAnimationTerminalState = HostVisualComponent->IsRuntimeTerminalState();
		View.HostAnimationPlayCount = HostVisualComponent->GetRuntimePlaybackCount();
		View.HostAnimationWatchdogCompletionCount =
			HostVisualComponent->GetRuntimeWatchdogCompletionCount();
	}
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
			if (PartView.PresentationDebugView.bHasRuntimePartFacts)
			{
				++View.RuntimeFactsPartActorCount;
				View.RuntimeInitiativeTotal += PartView.PresentationDebugView.CurrentInitiative;
			}
			if (PartView.PresentationDebugView.bHoverActive)
			{
				++View.HoveredPartActorCount;
			}
			if (PartView.PresentationDebugView.PredictionView.bVisible)
			{
				++View.PredictionVisiblePartActorCount;
			}
			if (PartView.BadgeLayoutStaggerIndex != INDEX_NONE)
			{
				++View.BadgeLayoutAppliedPartActorCount;
			}
		}
	}
	const WacomBattleSceneEnemyAuthoring::FHostPartIdentityAudit Audit =
		WacomBattleSceneEnemyAuthoring::BuildHostPartIdentityAudit(EnemyDefinition, PartActors);
	View.UnknownPartIds = Audit.UnknownPartIds;
	View.UnknownPartSlotIds = Audit.UnknownPartSlotIds;
	View.MissingDefinitionPartIds = Audit.MissingDefinitionPartIds;
	View.MissingDefinitionPartSlotIds = Audit.MissingDefinitionPartSlotIds;
	View.DuplicatePartSlotIds = Audit.DuplicatePartSlotIds;
	View.PartDefinitionMismatchSlotIds = Audit.PartDefinitionMismatchSlotIds;
	View.SurplusPartActorNames = Audit.SurplusPartActorNames;
	View.AuthoringState = WacomBattleSceneEnemyAuthoring::BuildHostAuthoringStateName(
		EnemyDefinition,
		View.AttachedPartActorCount,
		Audit);
	View.bAuthoringReady = View.AuthoringState == FName(TEXT("Ready"));
	return View;
}

void AWacomBattleEnemyActor::SetEnemyPanelViewData(const FWacomBattleEnemyPanelViewData& ViewData)
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}

	EnemyPanelWidgetComponent->InitWidget();
	if (UWacomBattleEnemyPanelWidget* PanelWidget =
		Cast<UWacomBattleEnemyPanelWidget>(EnemyPanelWidgetComponent->GetUserWidgetObject()))
	{
		PanelWidget->TakeWidget();
		PanelWidget->SetEnemyPanelViewData({ ViewData });
	}

	EnemyPanelWidgetComponent->SetVisibility(bEnemyPanelVisibleByDefault, true);
}

void AWacomBattleEnemyActor::ClearEnemyPanelViewData()
{
	if (UWacomBattleEnemyPanelWidget* PanelWidget =
		EnemyPanelWidgetComponent ? Cast<UWacomBattleEnemyPanelWidget>(EnemyPanelWidgetComponent->GetUserWidgetObject()) : nullptr)
	{
		PanelWidget->SetEnemyPanelViewData({});
	}

	if (EnemyPanelWidgetComponent)
	{
		EnemyPanelWidgetComponent->SetVisibility(false, true);
	}
}

void AWacomBattleEnemyActor::SetEnemyPanelActionPreview(
	const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts)
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}

	EnemyPanelWidgetComponent->InitWidget();
	if (UWacomBattleEnemyPanelWidget* PanelWidget =
		Cast<UWacomBattleEnemyPanelWidget>(EnemyPanelWidgetComponent->GetUserWidgetObject()))
	{
		PanelWidget->TakeWidget();
		PanelWidget->SetActionPreviewPartViews(PreviewParts);
	}
}

void AWacomBattleEnemyActor::ClearEnemyPanelActionPreview()
{
	if (UWacomBattleEnemyPanelWidget* PanelWidget =
		EnemyPanelWidgetComponent ? Cast<UWacomBattleEnemyPanelWidget>(EnemyPanelWidgetComponent->GetUserWidgetObject()) : nullptr)
	{
		PanelWidget->ClearActionPreview();
	}
}

void AWacomBattleEnemyActor::SetEnemyPanelHoveredVisible(bool bVisible)
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}

	EnemyPanelWidgetComponent->SetVisibility(bVisible || bEnemyPanelVisibleByDefault, true);
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
	AuthoringPartDefinitionMismatchSlotIds = View.PartDefinitionMismatchSlotIds;
	AuthoringSurplusPartActorNames = View.SurplusPartActorNames;
	AuthoringDebugSummary = GetBattleSceneEnemyDebugSummary();
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummaryForHUD(const UBattleHUD* HUD) const
{
	const FWacomBattleSceneEnemyDebugView View = GetBattleSceneEnemyDebugViewForHUD(HUD);
	return WacomBattleSceneEnemyAuthoring::FormatHostDebugSummary(View);
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
	return WacomBattleSceneEnemyAuthoring::ValidateHostPlacement(
		*this,
		Context,
		Super::IsDataValid(Context));
}
#endif
