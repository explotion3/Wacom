// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartActor.h"

#include "Components/WidgetComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/StaticMesh.h"
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

	PartVisual = CreateDefaultSubobject<UWacomBattleEnemyPartVisualComponent>(TEXT("PartVisual"));
	PartVisual->SetupAttachment(RootComponent);
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
	}

	if (InteractionTargetComponent)
	{
		InteractionTargetComponent->SetStableTargetId(PartId);
		InteractionTargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
	}

	if (WorldTargetBridgeComponent)
	{
		WorldTargetBridgeComponent->SetPartId(PartId);
		WorldTargetBridgeComponent->VisualTargetComponent = PartVisual;
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
		FVector(42.f, 38.f, 42.f),
		FVector(0.42f, 0.38f, 0.42f),
		FVector(0.f, 0.f, 0.f));
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeBodySample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Body"),
		FVector(62.f, 46.f, 42.f),
		FVector(0.62f, 0.46f, 0.42f),
		FVector(0.f, 0.f, 0.f));
}

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeTailSample()
{
	ConfigureDebugSnakeSample(
		TEXT("Snake.Tail"),
		FVector(48.f, 34.f, 34.f),
		FVector(0.48f, 0.34f, 0.34f),
		FVector(0.f, 0.f, 0.f));
}

FWacomBattleSceneEnemyPartDebugView
AWacomBattleEnemyPartActor::GetBattleSceneEnemyPartDebugView() const
{
	FWacomBattleSceneEnemyPartDebugView View;
	View.ActorName = GetName();
	View.PartId = PartId;
	View.HitBoundsExtent = HitBounds ? HitBounds->GetUnscaledBoxExtent() : FVector::ZeroVector;
	View.VisualName = PartVisual ? FName(*PartVisual->GetName()) : NAME_None;
	View.VisualMeshName = (PartVisual && PartVisual->GetStaticMesh())
		? FName(*PartVisual->GetStaticMesh()->GetName())
		: NAME_None;
	View.VisualScale = PartVisual ? PartVisual->GetRelativeScale3D() : FVector::ZeroVector;
	View.VisualRelativeLocation = PartVisual ? PartVisual->GetRelativeLocation() : FVector::ZeroVector;
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
			&& InteractionTargetComponent->GetStableTargetId() == PartId;
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
		TEXT("BattleSceneEnemyPart{Actor=%s PartId=%s HitBounds=%s Visual=%s VisualMesh=%s VisualScale=%s VisualLocation=%s PredictionWidget=%s StatusBadgeWidget=%s PredictionBadgeLocation=%s StatusBadgeLocation=%s PredictionBadgeDrawSize=%s StatusBadgeDrawSize=%s PredictionBadgeScale=%.2f StatusBadgeScale=%.2f StatusBadgeOpacity=%.2f DestroyedStatusBadgeOpacity=%.2f PredictionBadgeZOffset=%.1f BadgeStaggerIndex=%d BadgeStaggerOffset=%s InteractionConfigured=%s InteractionTargetId=%s InteractionStableId=%s BridgePartId=%s Bound=%s Registered=%s RuntimeFacts=%s RuntimePart=%s Hp=%d MaxHp=%d Shield=%d Initiative=%d Destroyed=%s Intent=%s IntentText=%s IntentInitiative=%d IntentResistance=%d StatusText=%s StatusBadgeVisible=%s Targetable=%s LastBind=%s LastCue=%s CueType=%d CueAmount=%d CueCount=%d DragPreview=%d DragPreviewActive=%s DragSource=%s DragCost=%d DragSwift=%s DragCanSubmit=%s DragReject=%s HoverActive=%s HoverReason=%s HoverStableId=%s HoverWorldTargetId=%s HoverScreen=%s PredictionVisible=%s PredictionMode=%d PredictedInitiative=%d PerfectCandidate=%s ActionRisk=%s PredictionReject=%s PredictionBadgeOffsetActive=%s CurrentStatusBadgeOpacity=%.2f}"),
		*View.ActorName,
		*View.PartId.ToString(),
		*View.HitBoundsExtent.ToCompactString(),
		*View.VisualName.ToString(),
		*View.VisualMeshName.ToString(),
		*View.VisualScale.ToCompactString(),
		*View.VisualRelativeLocation.ToCompactString(),
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

#if WITH_EDITOR
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

	if (PartId.IsNone())
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingPartId",
				"BattleEnemyPart 摆放配置错误：Actor={0} 缺少 PartId。"),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (HasAnyNonPositiveExtent(HitBoundsExtent))
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementInvalidHitBounds",
				"BattleEnemyPart 摆放配置错误：Actor={0} PartId={1} HitBoundsExtent 必须全部大于 0，当前为 {2}。"),
			FText::FromString(GetName()),
			FText::FromName(PartId),
			FText::FromString(HitBoundsExtent.ToCompactString())));
		Result = EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

void AWacomBattleEnemyPartActor::ConfigureDebugSnakeSample(
	FName InPartId,
	const FVector& InHitBoundsExtent,
	const FVector& InVisualScale,
	const FVector& InVisualRelativeLocation)
{
	PartId = InPartId;
	HitBoundsExtent = InHitBoundsExtent;
	VisualScale = InVisualScale;
	VisualRelativeLocation = InVisualRelativeLocation;
	if (!VisualMesh)
	{
		VisualMesh = LoadObject<UStaticMesh>(nullptr, DefaultPartMeshPath);
	}
	RefreshAuthoringState();
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
