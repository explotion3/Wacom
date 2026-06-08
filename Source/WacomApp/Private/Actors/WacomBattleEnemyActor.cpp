// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WacomBattleEnemyHostVisualComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UI/Battle/BattleHUD.h"

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

	HostVisualComponent =
		CreateDefaultSubobject<UWacomBattleEnemyHostVisualComponent>(TEXT("HostVisualComponent"));
	HostVisualComponent->bEditableWhenInherited = false;
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
		}
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
			if (PartView.PresentationDebugView.StatusBadgeView.bVisible)
			{
				++View.StatusBadgeVisiblePartActorCount;
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
	View.AuthoringState = WacomBattleSceneEnemyAuthoring::BuildHostAuthoringStateName(
		EnemyDefinition,
		View.AttachedPartActorCount,
		Audit);
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
