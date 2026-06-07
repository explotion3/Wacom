// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartActor.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/StaticMesh.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEnemyPartStatusBadgeWidget.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyPartActor"

namespace
{
	const TCHAR* DefaultPartMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

	bool ShouldValidateBattleEnemyPartPlacementActor(const AWacomBattleEnemyPartActor& PartActor)
	{
		return !PartActor.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !PartActor.IsTemplate();
	}

	bool HasAnyNonPositiveExtent(const FVector& Extent)
	{
		return Extent.X <= 0.0f || Extent.Y <= 0.0f || Extent.Z <= 0.0f;
	}

	bool HasAnyZeroScaleAxis(const FVector& Scale)
	{
		return FMath::IsNearlyZero(Scale.X)
			|| FMath::IsNearlyZero(Scale.Y)
			|| FMath::IsNearlyZero(Scale.Z);
	}

	FString BuildVisualLayerComponentName(FName LayerId, int32 LayerIndex)
	{
		const FString LayerName = LayerId.IsNone()
			? FString::Printf(TEXT("Layer%d"), LayerIndex)
			: LayerId.ToString();
		return FString::Printf(TEXT("VisualLayer_%02d_%s"), LayerIndex, *LayerName);
	}

	bool VisualLayerHasAsset(const FWacomBattleEnemyPartVisualLayer& Layer)
	{
		switch (Layer.LayerMode)
		{
		case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
			return Layer.Flipbook != nullptr;
		case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
		default:
			return Layer.Sprite != nullptr;
		}
	}

	FName GetVisualLayerAssetName(const FWacomBattleEnemyPartVisualLayer& Layer)
	{
		switch (Layer.LayerMode)
		{
		case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
			return Layer.Flipbook ? FName(*Layer.Flipbook->GetName()) : NAME_None;
		case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
		default:
			return Layer.Sprite ? FName(*Layer.Sprite->GetName()) : NAME_None;
		}
	}

	const TCHAR* GetVisualLayerModeDebugName(EWacomBattleEnemyPartVisualLayerMode LayerMode)
	{
		switch (LayerMode)
		{
		case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
			return TEXT("Flipbook");
		case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
		default:
			return TEXT("StaticSprite");
		}
	}

	FString JoinNamesForDebug(const TArray<FName>& Names)
	{
		TArray<FString> NameStrings;
		NameStrings.Reserve(Names.Num());
		for (const FName& Name : Names)
		{
			NameStrings.Add(Name.ToString());
		}
		return FString::Join(NameStrings, TEXT("|"));
	}

	FName BuildVisualAuthoringModeName(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
		const UStaticMesh* VisualMesh,
		bool bHostVisualContextActive)
	{
		if (VisualLayers.Num() > 0)
		{
			return TEXT("VisualLayers");
		}
		if (bHostVisualContextActive)
		{
			return TEXT("HitOnly");
		}
		if (VisualMesh)
		{
			return TEXT("LegacyPrototype");
		}
		return TEXT("None");
	}

	FName BuildPartAuthoringStateName(
		FName PartId,
		FName PartSlotId,
		const FVector& HitBoundsExtent,
		FName VisualAuthoringMode)
	{
		if (PartId.IsNone() || PartSlotId.IsNone())
		{
			return TEXT("MissingIdentity");
		}
		if (HasAnyNonPositiveExtent(HitBoundsExtent))
		{
			return TEXT("InvalidHitBounds");
		}
		if (VisualAuthoringMode == FName(TEXT("VisualLayers")))
		{
			return TEXT("UsingVisualLayers");
		}
		if (VisualAuthoringMode == FName(TEXT("HitOnly")))
		{
			return TEXT("HitOnly");
		}
		if (VisualAuthoringMode == FName(TEXT("LegacyPrototype")))
		{
			return TEXT("UsingLegacyPrototype");
		}
		return TEXT("MissingVisualResource");
	}

	bool HasHostVisualContext(const AWacomBattleEnemyPartActor& PartActor)
	{
		if (PartActor.IsHostVisualContextActive())
		{
			return true;
		}

		const AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(PartActor.GetAttachParentActor());
		return Host && Host->IsHostVisualActive();
	}
}

AWacomBattleEnemyPartActor::AWacomBattleEnemyPartActor()
{
	PrimaryActorTick.bCanEverTick = false;

	HitBounds = CreateDefaultSubobject<UWacomBattleEnemyPartHitBoundsComponent>(TEXT("HitBounds"));
	HitBounds->SetBoxExtent(HitBoundsExtent);
	HitBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBounds->SetCollisionObjectType(ECC_WorldDynamic);
	HitBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HitBounds->SetGenerateOverlapEvents(false);
	HitBounds->bEditableWhenInherited = false;
	RootComponent = HitBounds;

	VisualLayersRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualLayersRoot"));
	VisualLayersRoot->SetupAttachment(RootComponent);
	VisualLayersRoot->SetRelativeLocation(FVector::ZeroVector);
	VisualLayersRoot->SetRelativeRotation(FRotator::ZeroRotator);
	VisualLayersRoot->SetRelativeScale3D(FVector::OneVector);
	VisualLayersRoot->bEditableWhenInherited = false;

	PartVisual = CreateDefaultSubobject<UWacomBattleEnemyPartVisualComponent>(TEXT("PartVisual"));
	PartVisual->SetupAttachment(VisualLayersRoot);
	PartVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PartVisual->SetGenerateOverlapEvents(false);
	PartVisual->SetRelativeScale3D(VisualScale);
	PartVisual->SetRelativeLocation(VisualRelativeLocation);
	PartVisual->bEditableWhenInherited = false;
	VisualMesh = LoadObject<UStaticMesh>(nullptr, DefaultPartMeshPath);
	if (VisualMesh)
	{
		PartVisual->SetStaticMesh(VisualMesh);
	}

	InteractionTargetComponent =
		CreateDefaultSubobject<UWacomInteractionTargetComponent>(TEXT("InteractionTarget"));
	InteractionTargetComponent->bEditableWhenInherited = false;

	WorldTargetBridgeComponent =
		CreateDefaultSubobject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(TEXT("WorldTargetBridge"));
	WorldTargetBridgeComponent->bEditableWhenInherited = false;

	PredictionWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PredictionWidget"));
	PredictionWidgetComponent->SetupAttachment(RootComponent);
	PredictionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PredictionWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PredictionWidgetComponent->SetGenerateOverlapEvents(false);
	PredictionWidgetComponent->SetVisibility(false, true);
	PredictionWidgetComponent->bEditableWhenInherited = false;

	StatusBadgeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusBadgeWidget"));
	StatusBadgeWidgetComponent->SetupAttachment(RootComponent);
	StatusBadgeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusBadgeWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StatusBadgeWidgetComponent->SetGenerateOverlapEvents(false);
	StatusBadgeWidgetComponent->SetVisibility(false, true);
	StatusBadgeWidgetComponent->bEditableWhenInherited = false;

	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::RefreshAuthoringState()
{
	const FName EffectivePartId = GetEffectivePartDefinitionId();
	const bool bHitOnlyVisualMode = bHostVisualContextActive && VisualLayers.Num() == 0;

	if (HitBounds)
	{
		HitBounds->SetBoxExtent(HitBoundsExtent);
		HitBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HitBounds->SetCollisionObjectType(ECC_WorldDynamic);
		HitBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
		HitBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		HitBounds->SetGenerateOverlapEvents(false);
	}

	if (PartVisual)
	{
		PartVisual->SetStaticMesh(VisualMesh);
		PartVisual->SetRelativeScale3D(VisualScale);
		PartVisual->SetRelativeLocation(VisualRelativeLocation);
		PartVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PartVisual->SetGenerateOverlapEvents(false);
		PartVisual->SetVisibility(VisualLayers.Num() == 0 && !bHitOnlyVisualMode, true);
	}

	RefreshVisualLayers();

	if (InteractionTargetComponent)
	{
		InteractionTargetComponent->SetStableTargetId(EffectivePartId);
		InteractionTargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
	}

	if (WorldTargetBridgeComponent)
	{
		WorldTargetBridgeComponent->SetPartId(EffectivePartId);
		WorldTargetBridgeComponent->SetBattlePartSlotIdentity(
			NAME_None,
			EnemySlotId,
			GetEffectivePartSlotId());
		WorldTargetBridgeComponent->VisualTargetComponent = PartVisual;
		WorldTargetBridgeComponent->FeedbackTargetComponent = VisualLayersRoot;
		WorldTargetBridgeComponent->bAutoConfigureInteractionTarget = true;
		WorldTargetBridgeComponent->bEnablePredictionDisplay = bEnablePredictionWidget;
		WorldTargetBridgeComponent->bEnableStatusBadgeDisplay = bEnableStatusBadgeWidget;
		WorldTargetBridgeComponent->PredictionBadgeScale = PredictionBadgeScale;
		WorldTargetBridgeComponent->StatusBadgeScale = StatusBadgeScale;
		WorldTargetBridgeComponent->StatusBadgeOpacity = StatusBadgeOpacity;
		WorldTargetBridgeComponent->DestroyedStatusBadgeOpacity = DestroyedStatusBadgeOpacity;
		WorldTargetBridgeComponent->PredictionBadgeZOffsetWhenVisible = PredictionBadgeZOffsetWhenVisible;
		WorldTargetBridgeComponent->TargetConfirmPulseScale = TargetConfirmPulseScale;
		WorldTargetBridgeComponent->DamagePulseScale = DamagePulseScale;
		WorldTargetBridgeComponent->DestroyedPulseScale = DestroyedPulseScale;
		WorldTargetBridgeComponent->TargetableAffordanceScale = TargetableAffordanceScale;
		WorldTargetBridgeComponent->DragTargetPreviewScale = DragTargetPreviewScale;
		WorldTargetBridgeComponent->HoverProbeScale = HoverProbeScale;
		WorldTargetBridgeComponent->CueHoldSeconds = CueHoldSeconds;
	}

	if (PredictionWidgetComponent)
	{
		PredictionWidgetComponent->SetRelativeLocation(GetAppliedPredictionBadgeRelativeLocation());
		PredictionWidgetComponent->SetDrawSize(PredictionDrawSize);
		PredictionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		PredictionWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PredictionWidgetComponent->SetGenerateOverlapEvents(false);
		PredictionWidgetComponent->SetWidgetClass(
			PredictionWidgetClass ? PredictionWidgetClass.Get() : UWacomBattleEnemyPartPredictionWidget::StaticClass());
		PredictionWidgetComponent->SetVisibility(false, true);
	}

	if (StatusBadgeWidgetComponent)
	{
		StatusBadgeWidgetComponent->SetRelativeLocation(GetAppliedStatusBadgeRelativeLocation());
		StatusBadgeWidgetComponent->SetDrawSize(StatusBadgeDrawSize);
		StatusBadgeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		StatusBadgeWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		StatusBadgeWidgetComponent->SetGenerateOverlapEvents(false);
		StatusBadgeWidgetComponent->SetWidgetClass(
			StatusBadgeWidgetClass ? StatusBadgeWidgetClass.Get() : UWacomBattleEnemyPartStatusBadgeWidget::StaticClass());
		StatusBadgeWidgetComponent->SetVisibility(false, true);
	}

	if (WorldTargetBridgeComponent)
	{
		WorldTargetBridgeComponent->SetPredictionWidgetComponent(PredictionWidgetComponent);
		WorldTargetBridgeComponent->SetStatusBadgeWidgetComponent(StatusBadgeWidgetComponent);
		WorldTargetBridgeComponent->SetBadgeLayoutDebugState(BadgeLayoutStaggerIndex);
	}

	RefreshAuthoringStatusPreview();
}

void AWacomBattleEnemyPartActor::RefreshVisualLayers()
{
	for (UPaperSpriteComponent* SpriteComponent : GeneratedVisualLayerComponents)
	{
		if (SpriteComponent)
		{
			SpriteComponent->DestroyComponent();
		}
	}
	GeneratedVisualLayerComponents.Reset();
	for (UPaperFlipbookComponent* FlipbookComponent : GeneratedFlipbookVisualLayerComponents)
	{
		if (FlipbookComponent)
		{
			FlipbookComponent->DestroyComponent();
		}
	}
	GeneratedFlipbookVisualLayerComponents.Reset();

	if (VisualLayers.Num() == 0)
	{
		if (PartVisual)
		{
			PartVisual->SetVisibility(!bHostVisualContextActive, true);
		}
		return;
	}

	if (PartVisual)
	{
		PartVisual->SetVisibility(false, true);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 LayerIndex = 0; LayerIndex < VisualLayers.Num(); ++LayerIndex)
	{
		const FWacomBattleEnemyPartVisualLayer& Layer = VisualLayers[LayerIndex];
		if (!VisualLayerHasAsset(Layer))
		{
			continue;
		}

		const FName ComponentName(*BuildVisualLayerComponentName(Layer.LayerId, LayerIndex));
		USceneComponent* AttachParent = VisualLayersRoot.Get();
		if (!AttachParent)
		{
			AttachParent = RootComponent;
		}
		if (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook)
		{
			UPaperFlipbookComponent* FlipbookComponent =
				NewObject<UPaperFlipbookComponent>(this, ComponentName, RF_Transactional | RF_Transient);
			if (!FlipbookComponent)
			{
				continue;
			}

			FlipbookComponent->SetupAttachment(AttachParent);
			FlipbookComponent->SetFlipbook(Layer.Flipbook);
			FlipbookComponent->SetRelativeLocation(Layer.RelativeLocation);
			FlipbookComponent->SetRelativeRotation(Layer.RelativeRotation);
			FlipbookComponent->SetRelativeScale3D(Layer.RelativeScale3D);
			FlipbookComponent->SetSpriteColor(Layer.Tint);
			FlipbookComponent->SetTranslucentSortPriority(Layer.SortOrder);
			FlipbookComponent->SetVisibility(Layer.bVisible, true);
			FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			FlipbookComponent->SetGenerateOverlapEvents(false);
			FlipbookComponent->SetLooping(Layer.bLoopFlipbook);
			FlipbookComponent->SetPlayRate(Layer.FlipbookPlayRate);
			FlipbookComponent->SetPlaybackPosition(Layer.FlipbookStartTimeSeconds, false);
			if (Layer.bAutoPlayFlipbook && Layer.FlipbookPlayRate > 0.0f)
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
			GeneratedFlipbookVisualLayerComponents.Add(FlipbookComponent);
		}
		else
		{
			UPaperSpriteComponent* SpriteComponent =
				NewObject<UPaperSpriteComponent>(this, ComponentName, RF_Transactional | RF_Transient);
			if (!SpriteComponent)
			{
				continue;
			}

			SpriteComponent->SetupAttachment(AttachParent);
			SpriteComponent->SetSprite(Layer.Sprite);
			SpriteComponent->SetRelativeLocation(Layer.RelativeLocation);
			SpriteComponent->SetRelativeRotation(Layer.RelativeRotation);
			SpriteComponent->SetRelativeScale3D(Layer.RelativeScale3D);
			SpriteComponent->SetSpriteColor(Layer.Tint);
			SpriteComponent->SetTranslucentSortPriority(Layer.SortOrder);
			SpriteComponent->SetVisibility(Layer.bVisible, true);
			SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SpriteComponent->SetGenerateOverlapEvents(false);
			SpriteComponent->bEditableWhenInherited = false;
			AddInstanceComponent(SpriteComponent);
			SpriteComponent->RegisterComponentWithWorld(World);
			GeneratedVisualLayerComponents.Add(SpriteComponent);
		}
	}
}

void AWacomBattleEnemyPartActor::SetEnemySlotId(FName InEnemySlotId)
{
	if (EnemySlotId == InEnemySlotId)
	{
		return;
	}

	EnemySlotId = InEnemySlotId;
	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::SetHostVisualContext(bool bInHostVisualActive)
{
	if (bHostVisualContextActive == bInHostVisualActive)
	{
		return;
	}

	bHostVisualContextActive = bInHostVisualActive;
	RefreshAuthoringState();
}

FName AWacomBattleEnemyPartActor::GetEffectivePartSlotId() const
{
	return PartSlotId.IsNone() ? PartId : PartSlotId;
}

FName AWacomBattleEnemyPartActor::GetEffectivePartDefinitionId() const
{
	return PartId;
}

FName AWacomBattleEnemyPartActor::GetStableSceneTargetId() const
{
	const FName EffectivePartSlotId = GetEffectivePartSlotId();
	if (EffectivePartSlotId.IsNone())
	{
		return NAME_None;
	}
	if (EnemySlotId.IsNone())
	{
		return EffectivePartSlotId;
	}

	return FName(*FString::Printf(
		TEXT("%s.%s"),
		*EnemySlotId.ToString(),
		*EffectivePartSlotId.ToString()));
}

void AWacomBattleEnemyPartActor::SetBadgeLayoutStagger(
	int32 InStaggerIndex,
	const FVector& InStaggerOffset)
{
	BadgeLayoutStaggerIndex = InStaggerIndex;
	BadgeLayoutStaggerOffset = InStaggerOffset;
	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeHeadSample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Head"),
		TEXT("Head"),
		FVector(42.f, 38.f, 42.f),
		FVector(0.42f, 0.38f, 0.42f),
		FVector(0.f, 0.f, 0.f));
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeBodySample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Body"),
		TEXT("Body"),
		FVector(62.f, 46.f, 42.f),
		FVector(0.62f, 0.46f, 0.42f),
		FVector(0.f, 0.f, 0.f));
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeTailSample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Tail"),
		TEXT("Tail"),
		FVector(48.f, 34.f, 34.f),
		FVector(0.48f, 0.34f, 0.34f),
		FVector(0.f, 0.f, 0.f));
}

FWacomBattleSceneEnemyPartDebugView
AWacomBattleEnemyPartActor::GetBattleSceneEnemyPartDebugView() const
{
	FWacomBattleSceneEnemyPartDebugView View;
	View.ActorName = GetName();
	View.PartId = GetEffectivePartDefinitionId();
	View.PartSlotId = GetEffectivePartSlotId();
	View.EnemySlotId = EnemySlotId;
	View.StableSceneTargetId = GetStableSceneTargetId();
	View.VisualAuthoringMode = BuildVisualAuthoringModeName(
		VisualLayers,
		VisualMesh,
		bHostVisualContextActive);
	View.AuthoringState = BuildPartAuthoringStateName(
		View.PartId,
		View.PartSlotId,
		HitBoundsExtent,
		View.VisualAuthoringMode);
	View.bUsingHostVisual = bHostVisualContextActive;
	View.bHitOnlyVisual = View.VisualAuthoringMode == FName(TEXT("HitOnly"));
	View.bAuthoringReady =
		View.AuthoringState != FName(TEXT("MissingIdentity"))
		&& View.AuthoringState != FName(TEXT("InvalidHitBounds"));
	View.HitBoundsExtent = HitBounds ? HitBounds->GetUnscaledBoxExtent() : FVector::ZeroVector;
	View.VisualName = PartVisual ? FName(*PartVisual->GetName()) : NAME_None;
	View.VisualMeshName = (PartVisual && PartVisual->GetStaticMesh())
		? FName(*PartVisual->GetStaticMesh()->GetName())
		: NAME_None;
	View.VisualScale = PartVisual ? PartVisual->GetRelativeScale3D() : FVector::ZeroVector;
	View.VisualRelativeLocation = PartVisual ? PartVisual->GetRelativeLocation() : FVector::ZeroVector;
	View.bUsingVisualLayers = VisualLayers.Num() > 0;
	View.VisualLayerCount = VisualLayers.Num();
	View.GeneratedStaticVisualLayerComponentCount = GeneratedVisualLayerComponents.Num();
	View.GeneratedFlipbookVisualLayerComponentCount = GeneratedFlipbookVisualLayerComponents.Num();
	View.GeneratedVisualLayerComponentCount =
		View.GeneratedStaticVisualLayerComponentCount + View.GeneratedFlipbookVisualLayerComponentCount;
	for (const UPaperSpriteComponent* SpriteComponent : GeneratedVisualLayerComponents)
	{
		if (SpriteComponent && SpriteComponent->IsRegistered())
		{
			++View.RegisteredStaticVisualLayerComponentCount;
		}
		if (SpriteComponent && SpriteComponent->IsVisible())
		{
			++View.VisibleStaticVisualLayerComponentCount;
		}
	}
	for (const UPaperFlipbookComponent* FlipbookComponent : GeneratedFlipbookVisualLayerComponents)
	{
		if (FlipbookComponent && FlipbookComponent->IsRegistered())
		{
			++View.RegisteredFlipbookVisualLayerComponentCount;
		}
		if (FlipbookComponent && FlipbookComponent->IsVisible())
		{
			++View.VisibleFlipbookVisualLayerComponentCount;
		}
	}
	View.RegisteredVisualLayerComponentCount =
		View.RegisteredStaticVisualLayerComponentCount + View.RegisteredFlipbookVisualLayerComponentCount;
	View.VisibleVisualLayerComponentCount =
		View.VisibleStaticVisualLayerComponentCount + View.VisibleFlipbookVisualLayerComponentCount;
	View.VisualLayerIds.Reserve(VisualLayers.Num());
	View.VisualLayerAssetNames.Reserve(VisualLayers.Num());
	for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
	{
		View.VisualLayerIds.Add(Layer.LayerId);
		View.VisualLayerAssetNames.Add(GetVisualLayerAssetName(Layer));
	}
	View.DuplicateVisualLayerIds = BuildDuplicateVisualLayerIds();
	View.MissingVisualLayerAssetCount = CountMissingVisualLayerAssets();
	View.MissingVisualLayerSpriteCount = CountMissingVisualLayerSprites();
	View.MissingVisualLayerFlipbookCount = CountMissingVisualLayerFlipbooks();
	View.FeedbackTargetName = VisualLayersRoot ? FName(*VisualLayersRoot->GetName()) : NAME_None;
	View.PredictionWidgetName = PredictionWidgetComponent
		? FName(*PredictionWidgetComponent->GetName())
		: NAME_None;
	View.StatusBadgeWidgetName = StatusBadgeWidgetComponent
		? FName(*StatusBadgeWidgetComponent->GetName())
		: NAME_None;
	View.PredictionBadgeRelativeLocation = PredictionWidgetComponent
		? PredictionWidgetComponent->GetRelativeLocation()
		: FVector::ZeroVector;
	View.StatusBadgeRelativeLocation = StatusBadgeWidgetComponent
		? StatusBadgeWidgetComponent->GetRelativeLocation()
		: FVector::ZeroVector;
	View.BadgeLayoutStaggerOffset = BadgeLayoutStaggerOffset;
	View.PredictionBadgeDrawSize = PredictionWidgetComponent
		? PredictionWidgetComponent->GetDrawSize()
		: FVector2D::ZeroVector;
	View.StatusBadgeDrawSize = StatusBadgeWidgetComponent
		? StatusBadgeWidgetComponent->GetDrawSize()
		: FVector2D::ZeroVector;
	View.BadgeLayoutStaggerIndex = BadgeLayoutStaggerIndex;
	View.PredictionBadgeScale = PredictionBadgeScale;
	View.StatusBadgeScale = StatusBadgeScale;
	View.StatusBadgeOpacity = StatusBadgeOpacity;
	View.DestroyedStatusBadgeOpacity = DestroyedStatusBadgeOpacity;
	View.PredictionBadgeZOffsetWhenVisible = PredictionBadgeZOffsetWhenVisible;
	if (InteractionTargetComponent)
	{
		View.bInteractionTargetConfigured =
			InteractionTargetComponent->GetInteractionTargetTag().MatchesTagExact(
				WacomTags::Interaction_Target_Battle_EnemyPart)
			&& InteractionTargetComponent->GetStableTargetId() == GetEffectivePartDefinitionId();
		View.InteractionTargetId = InteractionTargetComponent->GetTargetId();
		View.InteractionTargetStableId = InteractionTargetComponent->GetStableTargetId();
	}
	if (WorldTargetBridgeComponent)
	{
		View.BridgeDebugView = WorldTargetBridgeComponent->GetBattleWorldTargetDebugView();
	}
	return View;
}

FString AWacomBattleEnemyPartActor::GetBattleSceneEnemyPartDebugSummary() const
{
	const FWacomBattleSceneEnemyPartDebugView View = GetBattleSceneEnemyPartDebugView();
	return FString::Printf(
		TEXT("BattleSceneEnemyPart{Actor=%s EnemySlotId=%s PartSlotId=%s StableSceneTargetId=%s PartId=%s AuthoringState=%s AuthoringReady=%s VisualAuthoringMode=%s UsingHostVisual=%s HitOnlyVisual=%s HitBounds=%s Visual=%s VisualMesh=%s VisualScale=%s VisualLocation=%s UsingVisualLayers=%s VisualLayerCount=%d GeneratedVisualLayerComponents=%d GeneratedStaticVisualLayerComponents=%d GeneratedFlipbookVisualLayerComponents=%d RegisteredVisualLayerComponents=%d RegisteredStaticVisualLayerComponents=%d RegisteredFlipbookVisualLayerComponents=%d VisibleVisualLayerComponents=%d VisibleStaticVisualLayerComponents=%d VisibleFlipbookVisualLayerComponents=%d MissingVisualLayerAssets=%d MissingVisualLayerSprites=%d MissingVisualLayerFlipbooks=%d VisualLayerIds=%s VisualLayerAssets=%s FeedbackTarget=%s PredictionWidget=%s StatusBadgeWidget=%s PredictionBadgeLocation=%s StatusBadgeLocation=%s PredictionBadgeDrawSize=%s StatusBadgeDrawSize=%s PredictionBadgeScale=%.2f StatusBadgeScale=%.2f StatusBadgeOpacity=%.2f DestroyedStatusBadgeOpacity=%.2f PredictionBadgeZOffset=%.1f BadgeStaggerIndex=%d BadgeStaggerOffset=%s InteractionConfigured=%s InteractionTargetId=%s InteractionStableId=%s BridgePartId=%s Bound=%s Registered=%s RuntimeFacts=%s RuntimePart=%s Hp=%d MaxHp=%d Shield=%d Initiative=%d Destroyed=%s Intent=%s IntentText=%s IntentInitiative=%d IntentResistance=%d StatusText=%s StatusBadgeVisible=%s Targetable=%s LastBind=%s LastCue=%s CueType=%d CueAmount=%d CueCount=%d DragPreview=%d DragPreviewActive=%s DragSource=%s DragCost=%d DragSwift=%s DragCanSubmit=%s DragReject=%s HoverActive=%s HoverReason=%s HoverStableId=%s HoverWorldTargetId=%s HoverScreen=%s PredictionVisible=%s PredictionMode=%d PredictedInitiative=%d PerfectCandidate=%s ActionRisk=%s PredictionReject=%s PredictionBadgeOffsetActive=%s CurrentStatusBadgeOpacity=%.2f}"),
		*View.ActorName,
		*View.EnemySlotId.ToString(),
		*View.PartSlotId.ToString(),
		*View.StableSceneTargetId.ToString(),
		*View.PartId.ToString(),
		*View.AuthoringState.ToString(),
		View.bAuthoringReady ? TEXT("true") : TEXT("false"),
		*View.VisualAuthoringMode.ToString(),
		View.bUsingHostVisual ? TEXT("true") : TEXT("false"),
		View.bHitOnlyVisual ? TEXT("true") : TEXT("false"),
		*View.HitBoundsExtent.ToCompactString(),
		*View.VisualName.ToString(),
		*View.VisualMeshName.ToString(),
		*View.VisualScale.ToCompactString(),
		*View.VisualRelativeLocation.ToCompactString(),
		View.bUsingVisualLayers ? TEXT("true") : TEXT("false"),
		View.VisualLayerCount,
		View.GeneratedVisualLayerComponentCount,
		View.GeneratedStaticVisualLayerComponentCount,
		View.GeneratedFlipbookVisualLayerComponentCount,
		View.RegisteredVisualLayerComponentCount,
		View.RegisteredStaticVisualLayerComponentCount,
		View.RegisteredFlipbookVisualLayerComponentCount,
		View.VisibleVisualLayerComponentCount,
		View.VisibleStaticVisualLayerComponentCount,
		View.VisibleFlipbookVisualLayerComponentCount,
		View.MissingVisualLayerAssetCount,
		View.MissingVisualLayerSpriteCount,
		View.MissingVisualLayerFlipbookCount,
		*JoinNamesForDebug(View.VisualLayerIds),
		*JoinNamesForDebug(View.VisualLayerAssetNames),
		*View.FeedbackTargetName.ToString(),
		*View.PredictionWidgetName.ToString(),
		*View.StatusBadgeWidgetName.ToString(),
		*View.PredictionBadgeRelativeLocation.ToCompactString(),
		*View.StatusBadgeRelativeLocation.ToCompactString(),
		*View.PredictionBadgeDrawSize.ToString(),
		*View.StatusBadgeDrawSize.ToString(),
		View.PredictionBadgeScale,
		View.StatusBadgeScale,
		View.StatusBadgeOpacity,
		View.DestroyedStatusBadgeOpacity,
		View.PredictionBadgeZOffsetWhenVisible,
		View.BadgeLayoutStaggerIndex,
		*View.BadgeLayoutStaggerOffset.ToCompactString(),
		View.bInteractionTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.InteractionTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*View.InteractionTargetStableId.ToString(),
		*View.BridgeDebugView.PartId.ToString(),
		View.BridgeDebugView.bBoundToSnapshot ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.bRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.bHasRuntimePartFacts ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.RuntimePartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.BridgeDebugView.CurrentHp,
		View.BridgeDebugView.MaxHp,
		View.BridgeDebugView.Shield,
		View.BridgeDebugView.CurrentInitiative,
		View.BridgeDebugView.bRuntimePartDestroyed ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.CurrentIntentId.ToString(),
		*View.BridgeDebugView.StatusBadgeView.CurrentIntentText.ToString(),
		View.BridgeDebugView.CurrentIntentInitiative,
		View.BridgeDebugView.CurrentIntentResistanceValue,
		*View.BridgeDebugView.StatusBadgeView.StatusText.ToString(),
		View.BridgeDebugView.StatusBadgeView.bVisible ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.bTargetable ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.LastBindResult.ToString(),
		*View.BridgeDebugView.LastCueKind.ToString(),
		static_cast<int32>(View.BridgeDebugView.LastCueType),
		View.BridgeDebugView.LastCueAmount,
		View.BridgeDebugView.CuePlayCount,
		static_cast<int32>(View.BridgeDebugView.DragPreviewState),
		View.BridgeDebugView.bDragPreviewActive ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.LastDragPredictionDebugInput.SourceCardInstanceId.ToString(
			EGuidFormats::DigitsWithHyphens),
		View.BridgeDebugView.LastDragPredictionDebugInput.SourceCardRuntimeCost,
		View.BridgeDebugView.LastDragPredictionDebugInput.bSourceCardSwift ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.LastDragPredictionDebugInput.bPreviewCanSubmit ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.LastDragPredictionDebugInput.PreviewRejectReason.ToString(),
		View.BridgeDebugView.bHoverActive ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.HoverReason.ToString(),
		*View.BridgeDebugView.HoverStableId.ToString(),
		*View.BridgeDebugView.HoverWorldTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*View.BridgeDebugView.HoverScreenPosition.ToString(),
		View.BridgeDebugView.PredictionView.bVisible ? TEXT("true") : TEXT("false"),
		static_cast<int32>(View.BridgeDebugView.PredictionView.Mode),
		View.BridgeDebugView.PredictionView.PredictedInitiative,
		View.BridgeDebugView.PredictionView.bPerfectReleaseCandidate ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.PredictionView.bActionRisk ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.PredictionView.RejectReason.ToString(),
		View.BridgeDebugView.bPredictionBadgeOffsetActive ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.CurrentStatusBadgeAppliedOpacity);
}

void AWacomBattleEnemyPartActor::LogBattleSceneEnemyPartDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyPartActor] %s"),
		*GetBattleSceneEnemyPartDebugSummary());
}

void AWacomBattleEnemyPartActor::RefreshAuthoringStatusPreview()
{
	const FWacomBattleSceneEnemyPartDebugView View = GetBattleSceneEnemyPartDebugView();
	AuthoringState = View.AuthoringState;
	bAuthoringReady = View.bAuthoringReady;
	VisualAuthoringMode = View.VisualAuthoringMode;
	bAuthoringUsingHostVisual = View.bUsingHostVisual;
	bAuthoringHitOnlyVisual = View.bHitOnlyVisual;
	AuthoringStableSceneTargetId = View.StableSceneTargetId;
	AuthoringVisualLayerCount = View.VisualLayerCount;
	AuthoringGeneratedVisualLayerComponentCount = View.GeneratedVisualLayerComponentCount;
	AuthoringRegisteredVisualLayerComponentCount = View.RegisteredVisualLayerComponentCount;
	AuthoringVisibleVisualLayerComponentCount = View.VisibleVisualLayerComponentCount;
	AuthoringMissingVisualLayerAssetCount = View.MissingVisualLayerAssetCount;
	AuthoringDuplicateVisualLayerIds = View.DuplicateVisualLayerIds;
	AuthoringFeedbackTargetName = View.FeedbackTargetName;
	AuthoringDebugSummary = GetBattleSceneEnemyPartDebugSummary();
}

#if WITH_EDITOR
void AWacomBattleEnemyPartActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshAuthoringState();
}

EDataValidationResult AWacomBattleEnemyPartActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidateBattleEnemyPartPlacementActor(*this))
	{
		return Result == EDataValidationResult::Invalid
			? EDataValidationResult::Invalid
			: EDataValidationResult::Valid;
	}

	if (PartId.IsNone() || GetEffectivePartSlotId().IsNone())
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingPartId",
				"BattleEnemyPart 摆放配置错误：Actor={0} 缺少 PartId 或 PartSlotId。"),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (HasAnyNonPositiveExtent(HitBoundsExtent))
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementInvalidHitBounds",
				"BattleEnemyPart 摆放配置错误：Actor={0} PartId={1} HitBoundsExtent 必须全部大于 0，当前为 {2}。"),
			FText::FromString(GetName()),
			FText::FromName(GetEffectivePartDefinitionId()),
			FText::FromString(HitBoundsExtent.ToCompactString())));
		Result = EDataValidationResult::Invalid;
	}

	TSet<FName> UsedLayerIds;
	for (int32 LayerIndex = 0; LayerIndex < VisualLayers.Num(); ++LayerIndex)
	{
		const FWacomBattleEnemyPartVisualLayer& Layer = VisualLayers[LayerIndex];
		if (Layer.LayerId.IsNone())
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementVisualLayerMissingId",
					"BattleEnemyPart 摆放配置错误：Actor={0} VisualLayers[{1}] 缺少 LayerId。"),
				FText::FromString(GetName()),
				FText::AsNumber(LayerIndex)));
			Result = EDataValidationResult::Invalid;
		}
		else if (UsedLayerIds.Contains(Layer.LayerId))
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementVisualLayerDuplicateId",
					"BattleEnemyPart 摆放配置错误：Actor={0} VisualLayers 中 LayerId={1} 重复。"),
				FText::FromString(GetName()),
				FText::FromName(Layer.LayerId)));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			UsedLayerIds.Add(Layer.LayerId);
		}

		if (HasAnyZeroScaleAxis(Layer.RelativeScale3D))
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementVisualLayerInvalidScale",
					"BattleEnemyPart 摆放配置错误：Actor={0} VisualLayers[{1}] RelativeScale3D 任一轴不能为 0，当前为 {2}。"),
				FText::FromString(GetName()),
				FText::AsNumber(LayerIndex),
				FText::FromString(Layer.RelativeScale3D.ToCompactString())));
			Result = EDataValidationResult::Invalid;
		}

		if (!VisualLayerHasAsset(Layer))
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("PlacementVisualLayerMissingAsset",
					"BattleEnemyPart 摆放警告：Actor={0} VisualLayers[{1}] Mode={2} 缺少对应视觉资源；该层不会生成可见组件。"),
				FText::FromString(GetName()),
				FText::AsNumber(LayerIndex),
				FText::FromString(GetVisualLayerModeDebugName(Layer.LayerMode))));
		}
	}

	if (VisualLayers.Num() == 0 && !VisualMesh && !HasHostVisualContext(*this))
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementMissingVisualResource",
				"BattleEnemyPart 摆放警告：Actor={0} 没有 VisualLayers，也没有旧 VisualMesh；该部位只有命中体和调试信息可见。"),
			FText::FromString(GetName())));
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeSample(
	FName InPartId,
	FName InPartSlotId,
	const FVector& InHitBoundsExtent,
	const FVector& InVisualScale,
	const FVector& InVisualRelativeLocation)
{
	PartId = InPartId;
	PartSlotId = InPartSlotId.IsNone() ? InPartId : InPartSlotId;
	HitBoundsExtent = InHitBoundsExtent;
	VisualScale = InVisualScale;
	VisualRelativeLocation = InVisualRelativeLocation;
	if (!VisualMesh)
	{
		VisualMesh = LoadObject<UStaticMesh>(nullptr, DefaultPartMeshPath);
	}
	RefreshAuthoringState();
}

TArray<FName> AWacomBattleEnemyPartActor::BuildDuplicateVisualLayerIds() const
{
	TSet<FName> Seen;
	TSet<FName> Duplicates;
	for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
	{
		if (Layer.LayerId.IsNone())
		{
			continue;
		}

		if (Seen.Contains(Layer.LayerId))
		{
			Duplicates.Add(Layer.LayerId);
		}
		else
		{
			Seen.Add(Layer.LayerId);
		}
	}

	TArray<FName> Result = Duplicates.Array();
	Result.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
	return Result;
}

int32 AWacomBattleEnemyPartActor::CountMissingVisualLayerAssets() const
{
	int32 Count = 0;
	for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
	{
		if (!VisualLayerHasAsset(Layer))
		{
			++Count;
		}
	}
	return Count;
}

int32 AWacomBattleEnemyPartActor::CountMissingVisualLayerSprites() const
{
	int32 Count = 0;
	for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
	{
		if (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::StaticSprite && !Layer.Sprite)
		{
			++Count;
		}
	}
	return Count;
}

int32 AWacomBattleEnemyPartActor::CountMissingVisualLayerFlipbooks() const
{
	int32 Count = 0;
	for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
	{
		if (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook && !Layer.Flipbook)
		{
			++Count;
		}
	}
	return Count;
}

FVector AWacomBattleEnemyPartActor::GetAppliedPredictionBadgeRelativeLocation() const
{
	return PredictionRelativeLocation + BadgeLayoutStaggerOffset;
}

FVector AWacomBattleEnemyPartActor::GetAppliedStatusBadgeRelativeLocation() const
{
	return StatusBadgeRelativeLocation + BadgeLayoutStaggerOffset;
}

#undef LOCTEXT_NAMESPACE
