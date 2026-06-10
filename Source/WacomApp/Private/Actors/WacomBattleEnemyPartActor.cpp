// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartActor.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"
#include "Actors/WacomBattleEnemyPartTargetAuthoringHelpers.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartVisualLayerComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"

AWacomBattleEnemyPartActor::AWacomBattleEnemyPartActor()
{
	PrimaryActorTick.bCanEverTick = false;

	HitBounds = CreateDefaultSubobject<UWacomBattleEnemyPartHitBoundsComponent>(TEXT("HitBounds"));
	WacomBattleEnemyPartTargetAuthoring::ConfigureHitBoundsComponent(HitBounds, HitBoundsExtent);
	HitBounds->bEditableWhenInherited = false;
	RootComponent = HitBounds;

	VisualLayersRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualLayersRoot"));
	VisualLayersRoot->SetupAttachment(RootComponent);
	VisualLayersRoot->SetRelativeLocation(FVector::ZeroVector);
	VisualLayersRoot->SetRelativeRotation(FRotator::ZeroRotator);
	VisualLayersRoot->SetRelativeScale3D(FVector::OneVector);
	VisualLayersRoot->bEditableWhenInherited = false;

	InteractionTargetComponent =
		CreateDefaultSubobject<UWacomInteractionTargetComponent>(TEXT("InteractionTarget"));
	InteractionTargetComponent->bEditableWhenInherited = false;

	WorldTargetBridgeComponent =
		CreateDefaultSubobject<UWacomBattleEnemyPartWorldTargetBridgeComponent>(TEXT("WorldTargetBridge"));
	WorldTargetBridgeComponent->bEditableWhenInherited = false;

	PresentationComponent =
		CreateDefaultSubobject<UWacomBattleEnemyPartPresentationComponent>(TEXT("Presentation"));
	PresentationComponent->bEditableWhenInherited = false;

	VisualLayerComponent =
		CreateDefaultSubobject<UWacomBattleEnemyPartVisualLayerComponent>(TEXT("VisualLayerComponent"));
	VisualLayerComponent->bEditableWhenInherited = false;

	PredictionWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PredictionWidget"));
	PredictionWidgetComponent->SetupAttachment(RootComponent);
	PredictionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PredictionWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PredictionWidgetComponent->SetGenerateOverlapEvents(false);
	PredictionWidgetComponent->SetVisibility(false, true);
	PredictionWidgetComponent->bEditableWhenInherited = false;

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

	RefreshVisualLayers();
	WacomBattleEnemyPartTargetAuthoring::SyncTargetFacade(
		HitBounds,
		InteractionTargetComponent,
		WorldTargetBridgeComponent,
		EffectivePartId,
		EnemySlotId,
		GetEffectivePartSlotId(),
		HitBoundsExtent);

	if (PresentationComponent)
	{
		PresentationComponent->VisualTargetComponent = nullptr;
		PresentationComponent->FeedbackTargetComponent = VisualLayersRoot;
		PresentationComponent->bEnablePredictionDisplay = bEnablePredictionWidget;
		PresentationComponent->PredictionBadgeScale = PredictionBadgeScale;
		PresentationComponent->PredictionBadgeZOffsetWhenVisible = PredictionBadgeZOffsetWhenVisible;
		PresentationComponent->TargetConfirmPulseScale = TargetConfirmPulseScale;
		PresentationComponent->DamagePulseScale = DamagePulseScale;
		PresentationComponent->DestroyedPulseScale = DestroyedPulseScale;
		PresentationComponent->TargetableAffordanceScale = TargetableAffordanceScale;
		PresentationComponent->DragTargetPreviewScale = DragTargetPreviewScale;
		PresentationComponent->HoverProbeScale = HoverProbeScale;
		PresentationComponent->CueHoldSeconds = CueHoldSeconds;
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

	if (PresentationComponent)
	{
		PresentationComponent->SetPredictionWidgetComponent(PredictionWidgetComponent);
		PresentationComponent->SetBadgeLayoutDebugState(BadgeLayoutStaggerIndex);
	}

	RefreshAuthoringStatusPreview();
}

void AWacomBattleEnemyPartActor::RefreshVisualLayers()
{
	if (VisualLayerComponent)
	{
		VisualLayerComponent->RefreshVisualLayers(
			VisualLayers,
			VisualLayersRoot);
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
	return PartSlotId;
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
		FVector(42.f, 38.f, 42.f));
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeBodySample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Body"),
		TEXT("Body"),
		FVector(62.f, 46.f, 42.f));
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeTailSample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Tail"),
		TEXT("Tail"),
		FVector(48.f, 34.f, 34.f));
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
	View.VisualAuthoringMode = WacomBattleSceneEnemyAuthoring::BuildVisualAuthoringModeName(
		VisualLayers,
		bHostVisualContextActive);
	View.AuthoringState = WacomBattleSceneEnemyAuthoring::BuildPartAuthoringStateName(
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
	const FWacomBattleEnemyPartVisualLayerDebugView VisualLayerView = VisualLayerComponent
		? VisualLayerComponent->BuildVisualLayerDebugView(VisualLayers)
		: FWacomBattleEnemyPartVisualLayerDebugView();
	View.bUsingVisualLayers = VisualLayerView.bUsingVisualLayers;
	View.VisualLayerCount = VisualLayerView.VisualLayerCount;
	View.GeneratedStaticVisualLayerComponentCount = VisualLayerView.GeneratedStaticVisualLayerComponentCount;
	View.GeneratedFlipbookVisualLayerComponentCount = VisualLayerView.GeneratedFlipbookVisualLayerComponentCount;
	View.GeneratedVisualLayerComponentCount = VisualLayerView.GeneratedVisualLayerComponentCount;
	View.RegisteredStaticVisualLayerComponentCount = VisualLayerView.RegisteredStaticVisualLayerComponentCount;
	View.RegisteredFlipbookVisualLayerComponentCount = VisualLayerView.RegisteredFlipbookVisualLayerComponentCount;
	View.RegisteredVisualLayerComponentCount = VisualLayerView.RegisteredVisualLayerComponentCount;
	View.VisibleStaticVisualLayerComponentCount = VisualLayerView.VisibleStaticVisualLayerComponentCount;
	View.VisibleFlipbookVisualLayerComponentCount = VisualLayerView.VisibleFlipbookVisualLayerComponentCount;
	View.VisibleVisualLayerComponentCount = VisualLayerView.VisibleVisualLayerComponentCount;
	View.VisualLayerIds = VisualLayerView.VisualLayerIds;
	View.VisualLayerAssetNames = VisualLayerView.VisualLayerAssetNames;
	View.DuplicateVisualLayerIds = VisualLayerView.DuplicateVisualLayerIds;
	View.MissingVisualLayerAssetCount = VisualLayerView.MissingVisualLayerAssetCount;
	View.MissingVisualLayerSpriteCount = VisualLayerView.MissingVisualLayerSpriteCount;
	View.MissingVisualLayerFlipbookCount = VisualLayerView.MissingVisualLayerFlipbookCount;
	View.FeedbackTargetName = VisualLayersRoot ? FName(*VisualLayersRoot->GetName()) : NAME_None;
	View.PredictionWidgetName = PredictionWidgetComponent
		? FName(*PredictionWidgetComponent->GetName())
		: NAME_None;
	View.PredictionBadgeRelativeLocation = PredictionWidgetComponent
		? PredictionWidgetComponent->GetRelativeLocation()
		: FVector::ZeroVector;
	View.BadgeLayoutStaggerOffset = BadgeLayoutStaggerOffset;
	View.PredictionBadgeDrawSize = PredictionWidgetComponent
		? PredictionWidgetComponent->GetDrawSize()
		: FVector2D::ZeroVector;
	View.BadgeLayoutStaggerIndex = BadgeLayoutStaggerIndex;
	View.PredictionBadgeScale = PredictionBadgeScale;
	View.PredictionBadgeZOffsetWhenVisible = PredictionBadgeZOffsetWhenVisible;
	const WacomBattleEnemyPartTargetAuthoring::FInteractionTargetDebug InteractionDebug =
		WacomBattleEnemyPartTargetAuthoring::BuildInteractionTargetDebug(
			InteractionTargetComponent,
			GetEffectivePartDefinitionId());
	View.bInteractionTargetConfigured = InteractionDebug.bConfigured;
	View.InteractionTargetId = InteractionDebug.TargetId;
	View.InteractionTargetStableId = InteractionDebug.StableTargetId;
	if (WorldTargetBridgeComponent)
	{
		View.BridgeDebugView = WorldTargetBridgeComponent->GetBattleWorldTargetDebugView();
	}
	if (PresentationComponent)
	{
		View.PresentationDebugView = PresentationComponent->GetBattleEnemyPartPresentationDebugView();
	}
	return View;
}

FString AWacomBattleEnemyPartActor::GetBattleSceneEnemyPartDebugSummary() const
{
	const FWacomBattleSceneEnemyPartDebugView View = GetBattleSceneEnemyPartDebugView();
	return WacomBattleSceneEnemyAuthoring::FormatPartDebugSummary(View);
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
	return WacomBattleSceneEnemyAuthoring::ValidatePartPlacement(
		*this,
		Context,
		Super::IsDataValid(Context));
}
#endif

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeSample(
	FName InPartId,
	FName InPartSlotId,
	const FVector& InHitBoundsExtent)
{
	PartId = InPartId;
	PartSlotId = InPartSlotId;
	HitBoundsExtent = InHitBoundsExtent;
	RefreshAuthoringState();
}

FVector AWacomBattleEnemyPartActor::GetAppliedPredictionBadgeRelativeLocation() const
{
	return PredictionRelativeLocation + BadgeLayoutStaggerOffset;
}
