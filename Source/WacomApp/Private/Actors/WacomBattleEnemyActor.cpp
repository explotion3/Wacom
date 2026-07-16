// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyHostAnimationStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomBattleEnemyHostVisualComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "Blueprint/UserWidget.h"

namespace
{
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
	if (bRuntimeEncounterPresentationRetired)
	{
		return;
	}
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
	if (bRuntimeEncounterPresentationRetired)
	{
		OutPartActors.Reset();
		return;
	}

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
	if (bRuntimeEncounterPresentationRetired
		|| HostAuthoringMode != EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual
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
	if (bRuntimeEncounterPresentationRetired
		|| HostAuthoringMode != EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual
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
	if (!bRuntimeEncounterPresentationRetired && HostVisualComponent)
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

void AWacomBattleEnemyActor::RetireRuntimeEncounterPresentation()
{
	if (bRuntimeEncounterPresentationRetired)
	{
		return;
	}

	bRuntimeEncounterPresentationRetired = true;
	CancelRuntimeHostAnimation();
	ClearEnemyPanelActionPreview();
	ClearEnemyPanelViewData();
	for (AWacomBattleEnemyPartActor* PartActor : BuildAttachedBattleEnemyPartActors())
	{
		if (IsValid(PartActor) && !PartActor->IsActorBeingDestroyed())
		{
			PartActor->RetireRuntimeEncounterPresentation();
		}
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
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

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugView() const
{
	return GetBattleSceneEnemyDebugViewForHUD(nullptr);
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugViewForHUD(
	const UBattleHUD* HUD) const
{
	FWacomBattleSceneEnemyDebugView View;
	const FWacomBattleSceneEnemyHostAuthoringReport AuthoringReport =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*this);
	View.ActorName = GetName();
	View.EnemyDefinitionName = EnemyDefinition ? FName(*EnemyDefinition->GetName()) : NAME_None;
	View.EnemyId = EnemyDefinition ? EnemyDefinition->EnemyId : NAME_None;
	View.EnemySlotId = GetEffectiveEnemySlotId();
	View.AuthoringMode = WacomBattleSceneEnemyAuthoring::GetHostAuthoringModeDebugString(
		HostAuthoringMode);
	View.HostVisualMode = GetHostVisualModeDebugName();
	View.bUsingHostVisual = AuthoringReport.bUsingHostVisual;
	View.bRuntimeEncounterPresentationRetired = bRuntimeEncounterPresentationRetired;
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
	View.AttachedPartActorCount = AuthoringReport.PartActorCount;
	View.AttachedPartIds = AuthoringReport.AttachedPartIds;
	View.AttachedPartSlotIds = AuthoringReport.AttachedPartSlotIds;
	View.StableSceneTargetIds = AuthoringReport.StableSceneTargetIds;
	for (const AWacomBattleEnemyPartActor* PartActor : PartActors)
	{
		if (PartActor)
		{
			const FWacomBattleSceneEnemyPartDebugView PartView =
				PartActor->GetBattleSceneEnemyPartDebugView();
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
	View.UnknownPartIds = AuthoringReport.IdentityAudit.UnknownPartIds;
	View.UnknownPartSlotIds = AuthoringReport.IdentityAudit.UnknownPartSlotIds;
	View.MissingDefinitionPartIds = AuthoringReport.IdentityAudit.MissingDefinitionPartIds;
	View.MissingDefinitionPartSlotIds =
		AuthoringReport.IdentityAudit.MissingDefinitionPartSlotIds;
	View.DuplicatePartSlotIds = AuthoringReport.IdentityAudit.DuplicatePartSlotIds;
	View.PartDefinitionMismatchSlotIds =
		AuthoringReport.IdentityAudit.PartDefinitionMismatchSlotIds;
	View.SurplusPartActorNames = AuthoringReport.IdentityAudit.SurplusPartActorNames;
	View.AuthoringState = AuthoringReport.AuthoringState;
	View.bAuthoringReady = AuthoringReport.bAuthoringReady;
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
