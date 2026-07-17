// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartActor.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringHelpers.h"
#include "Actors/WacomBattleEnemyPartTargetAuthoringHelpers.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartVisualLayerComponent.h"
#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"
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

	ImpactAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("ImpactAnchor"));
	ImpactAnchor->SetupAttachment(RootComponent);
	ImpactAnchor->SetRelativeLocation(ImpactAnchorRelativeLocation);
	ImpactAnchor->SetRelativeRotation(FRotator::ZeroRotator);
	ImpactAnchor->SetRelativeScale3D(FVector::OneVector);
	ImpactAnchor->bEditableWhenInherited = false;

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
	if (bRuntimeEncounterPresentationRetired)
	{
		return;
	}
	InitializeRuntimePresentationState();
	NotifyRuntimePartTopologyChanged();
}

void AWacomBattleEnemyPartActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelRuntimePartActionAnimation();
	NotifyRuntimePartTopologyChanged();
	Super::EndPlay(EndPlayReason);
}

void AWacomBattleEnemyPartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::RefreshAuthoringState()
{
	RefreshVisualLayers();
	ApplyRuntimeFacadeAndPresentationState();
	RefreshAuthoringStatusPreview();
}

void AWacomBattleEnemyPartActor::InitializeRuntimePresentationState()
{
	RefreshVisualLayers();
	ApplyRuntimeFacadeAndPresentationState();
}

void AWacomBattleEnemyPartActor::RetireRuntimeEncounterPresentation()
{
	if (bRuntimeEncounterPresentationRetired)
	{
		return;
	}

	bRuntimeEncounterPresentationRetired = true;
	CancelRuntimePartActionAnimation();
	if (WorldTargetBridgeComponent)
	{
		WorldTargetBridgeComponent->ClearBattleBinding();
	}
	if (PresentationComponent)
	{
		PresentationComponent->ClearRuntimePartFacts();
	}
	if (PredictionWidgetComponent)
	{
		PredictionWidgetComponent->SetVisibility(false, true);
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

void AWacomBattleEnemyPartActor::PlayRuntimePartActionAnimation(
	FName IntentId,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	const FWacomBattleEnemyPartAnimationClip* Clip = PartAnimationStyle
		? PartAnimationStyle->ResolveActionClip(IntentId)
		: nullptr;
	if (bRuntimeEncounterPresentationRetired
		|| bHostVisualContextActive
		|| !VisualLayerComponent
		|| !PartAnimationStyle
		|| !Clip)
	{
		Callbacks.CompleteImmediately();
		return;
	}

	VisualLayerComponent->PlayRuntimeActionOneShot(
		VisualLayers,
		PartAnimationStyle->TargetVisualLayerId,
		Clip->Flipbook,
		Clip->PlayRate,
		Clip->ImpactNormalizedTime,
		IntentId,
		MoveTemp(Callbacks));
}

void AWacomBattleEnemyPartActor::CancelRuntimePartActionAnimation()
{
	if (VisualLayerComponent)
	{
		VisualLayerComponent->CancelRuntimeActionPlayback(true);
	}
}

void AWacomBattleEnemyPartActor::ApplyRuntimeFacadeAndPresentationState()
{
	const FName EffectivePartId = GetEffectivePartDefinitionId();

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
		if (ImpactAnchor)
		{
			ImpactAnchor->SetRelativeLocation(ImpactAnchorRelativeLocation);
		}
		PresentationComponent->VisualTargetComponent = nullptr;
		PresentationComponent->SetPresentationTargets(VisualLayersRoot, ImpactAnchor, HitBounds);
		PresentationComponent->SetImpactFeedbackStyle(
			ResolveImpactStyle(),
			bEnableImpactFeedback);
		PresentationComponent->SetTargetPreviewFeedbackStyle(
			ResolveTargetPreviewStyle(),
			bEnableTargetPreviewFeedback);
		PresentationComponent->bEnablePredictionDisplay = bEnablePredictionWidget;
		PresentationComponent->PredictionBadgeScale = PredictionBadgeScale;
		PresentationComponent->PredictionBadgeZOffsetWhenVisible = PredictionBadgeZOffsetWhenVisible;
		PresentationComponent->TargetableAffordanceScale = TargetableAffordanceScale;
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
}

void AWacomBattleEnemyPartActor::ApplyRuntimeHostContext(
	FName InEnemySlotId,
	bool bInHostVisualActive,
	UWacomBattleEnemyPartImpactStyle* InHostImpactStyle,
	UWacomBattleEnemyPartTargetPreviewStyle* InHostTargetPreviewStyle,
	int32 InBadgeStaggerIndex,
	const FVector& InBadgeStaggerOffset)
{
	EnemySlotId = InEnemySlotId;
	bHostVisualContextActive = bInHostVisualActive;
	HostImpactStyle = InHostImpactStyle;
	HostTargetPreviewStyle = InHostTargetPreviewStyle;
	BadgeLayoutStaggerIndex = InBadgeStaggerIndex;
	BadgeLayoutStaggerOffset = InBadgeStaggerOffset;
	ApplyRuntimeFacadeAndPresentationState();
}

void AWacomBattleEnemyPartActor::NotifyRuntimePartTopologyChanged() const
{
	if (AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(GetAttachParentActor()))
	{
		Host->InvalidateRuntimePartTopology();
	}
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

int32 AWacomBattleEnemyPartActor::ApplyRuntimeDestroyedVisualState()
{
	return VisualLayerComponent
		? VisualLayerComponent->ApplyRuntimeDestroyedState(VisualLayers)
		: 0;
}

void AWacomBattleEnemyPartActor::ResetRuntimeDestroyedVisualState()
{
	if (VisualLayerComponent)
	{
		VisualLayerComponent->RestoreRuntimeAuthoredState(VisualLayers);
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

void AWacomBattleEnemyPartActor::SetHostImpactStyle(
	UWacomBattleEnemyPartImpactStyle* InHostImpactStyle)
{
	if (HostImpactStyle == InHostImpactStyle)
	{
		return;
	}

	HostImpactStyle = InHostImpactStyle;
	RefreshAuthoringState();
}

UWacomBattleEnemyPartImpactStyle* AWacomBattleEnemyPartActor::ResolveImpactStyle() const
{
	return ImpactStyleOverride ? ImpactStyleOverride.Get() : HostImpactStyle.Get();
}

void AWacomBattleEnemyPartActor::SetHostTargetPreviewStyle(
	UWacomBattleEnemyPartTargetPreviewStyle* InHostTargetPreviewStyle)
{
	if (HostTargetPreviewStyle == InHostTargetPreviewStyle)
	{
		return;
	}

	HostTargetPreviewStyle = InHostTargetPreviewStyle;
	RefreshAuthoringState();
}

UWacomBattleEnemyPartTargetPreviewStyle* AWacomBattleEnemyPartActor::ResolveTargetPreviewStyle() const
{
	return TargetPreviewStyleOverride
		? TargetPreviewStyleOverride.Get()
		: HostTargetPreviewStyle.Get();
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
	View.DestroyedVisualResourceCount = VisualLayerView.DestroyedVisualResourceCount;
	View.bDestroyedVisualStateApplied = VisualLayerView.bRuntimeDestroyedStateApplied;
	View.DestroyedVisualLayerCount = VisualLayerView.RuntimeDestroyedVisualLayerCount;
	View.DestroyedVisualApplyCount = VisualLayerView.RuntimeDestroyedVisualApplyCount;
	View.PartAnimationStyleAssetName = PartAnimationStyle
		? FName(*PartAnimationStyle->GetName())
		: NAME_None;
	View.PartAnimationTargetLayerId = PartAnimationStyle
		? PartAnimationStyle->TargetVisualLayerId
		: NAME_None;
	View.CurrentPartAnimationClipName = VisualLayerView.CurrentRuntimeActionClipName;
	View.CurrentPartAnimationIntentId = VisualLayerView.CurrentRuntimeActionIntentId;
	View.bPartAnimationPlaybackActive = VisualLayerView.bRuntimeActionPlaybackActive;
	View.PartAnimationPlayCount = VisualLayerView.RuntimeActionPlaybackCount;
	View.PartAnimationWatchdogCompletionCount =
		VisualLayerView.RuntimeActionWatchdogCompletionCount;
	View.PartAnimationImpactNormalizedTime =
		VisualLayerComponent->GetCurrentRuntimeActionImpactNormalizedTime();
	View.bPartAnimationImpactFired = VisualLayerComponent->HasRuntimeActionImpactFired();
	View.PartAnimationImpactCount = VisualLayerComponent->GetRuntimeActionImpactCount();
	View.PartAnimationWatchdogForcedImpactCount =
		VisualLayerComponent->GetRuntimeActionWatchdogForcedImpactCount();
	View.FeedbackTargetName = VisualLayersRoot ? FName(*VisualLayersRoot->GetName()) : NAME_None;
	View.bImpactAnchorReady = IsValid(ImpactAnchor)
		&& !ImpactAnchor->GetComponentLocation().ContainsNaN();
	View.ImpactAnchorName = ImpactAnchor ? FName(*ImpactAnchor->GetName()) : NAME_None;
	View.ImpactAnchorRelativeLocation = ImpactAnchor
		? ImpactAnchor->GetRelativeLocation()
		: ImpactAnchorRelativeLocation;
	View.ImpactAnchorWorldLocation = ImpactAnchor
		? ImpactAnchor->GetComponentLocation()
		: GetActorLocation();
	View.bImpactFeedbackEnabled = bEnableImpactFeedback;
	if (UWacomBattleEnemyPartImpactStyle* ResolvedStyle = ResolveImpactStyle())
	{
		View.ResolvedImpactStyleName = FName(*ResolvedStyle->GetName());
	}
	View.bTargetPreviewFeedbackEnabled = bEnableTargetPreviewFeedback;
	if (UWacomBattleEnemyPartTargetPreviewStyle* ResolvedStyle = ResolveTargetPreviewStyle())
	{
		View.ResolvedTargetPreviewStyleName = FName(*ResolvedStyle->GetName());
	}
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
	bAuthoringImpactAnchorReady = View.bImpactAnchorReady;
	AuthoringImpactAnchorName = View.ImpactAnchorName;
	AuthoringImpactAnchorWorldLocation = View.ImpactAnchorWorldLocation;
	AuthoringResolvedImpactStyleName = View.ResolvedImpactStyleName;
	AuthoringResolvedTargetPreviewStyleName = View.ResolvedTargetPreviewStyleName;
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
	EDataValidationResult Result = WacomBattleSceneEnemyAuthoring::ValidatePartPlacement(
		*this,
		Context,
		Super::IsDataValid(Context));
	if (ImpactAnchorRelativeLocation.ContainsNaN())
	{
		Context.AddError(FText::FromString(
			TEXT("ImpactAnchorRelativeLocation 包含非有限值，无法作为世界命中特效锚点。")));
		Result = EDataValidationResult::Invalid;
	}
	if (!FMath::IsFinite(DestroyedVisualSwapNormalizedTime)
		|| DestroyedVisualSwapNormalizedTime < 0.0f
		|| DestroyedVisualSwapNormalizedTime > 1.0f)
	{
		Context.AddError(FText::FromString(
			TEXT("DestroyedVisualSwapNormalizedTime 必须是 0 到 1 之间的有限归一化时间。")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
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
