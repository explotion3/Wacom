// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartActor.h"

#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Tags/WacomGameplayTags.h"

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

	RefreshAuthoringState();
}

void AWacomBattleEnemyPartActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshAuthoringState();
	if (!PartId.IsNone() && HasDuplicatePartIdInWorld())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WacomBattleEnemyPartActor] %s: PartId %s 与同关卡其他 BattleEnemyPart 重复；当前单敌人战斗会共享同一 runtime part 绑定。"),
			*GetName(),
			*PartId.ToString());
	}
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
		WorldTargetBridgeComponent->TargetConfirmPulseScale = TargetConfirmPulseScale;
		WorldTargetBridgeComponent->DamagePulseScale = DamagePulseScale;
		WorldTargetBridgeComponent->DestroyedPulseScale = DestroyedPulseScale;
		WorldTargetBridgeComponent->TargetableAffordanceScale = TargetableAffordanceScale;
		WorldTargetBridgeComponent->DragTargetPreviewScale = DragTargetPreviewScale;
		WorldTargetBridgeComponent->HoverProbeScale = HoverProbeScale;
		WorldTargetBridgeComponent->CueHoldSeconds = CueHoldSeconds;
	}
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
		TEXT("BattleSceneEnemyPart{Actor=%s PartId=%s HitBounds=%s Visual=%s VisualMesh=%s VisualScale=%s VisualLocation=%s InteractionConfigured=%s InteractionTargetId=%s InteractionStableId=%s BridgePartId=%s Bound=%s Registered=%s RuntimeFacts=%s RuntimePart=%s Initiative=%d Destroyed=%s Intent=%s IntentInitiative=%d IntentResistance=%d Targetable=%s LastBind=%s LastCue=%s CueType=%d CueAmount=%d CueCount=%d DragPreview=%d DragPreviewActive=%s DragSource=%s DragCost=%d DragSwift=%s DragCanSubmit=%s DragReject=%s HoverActive=%s HoverReason=%s HoverStableId=%s HoverWorldTargetId=%s HoverScreen=%s}"),
		*View.ActorName,
		*View.PartId.ToString(),
		*View.HitBoundsExtent.ToCompactString(),
		*View.VisualName.ToString(),
		*View.VisualMeshName.ToString(),
		*View.VisualScale.ToCompactString(),
		*View.VisualRelativeLocation.ToCompactString(),
		View.bInteractionTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.InteractionTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*View.InteractionTargetStableId.ToString(),
		*View.BridgeDebugView.PartId.ToString(),
		View.BridgeDebugView.bBoundToSnapshot ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.bRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
		View.BridgeDebugView.bHasRuntimePartFacts ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.RuntimePartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.BridgeDebugView.CurrentInitiative,
		View.BridgeDebugView.bRuntimePartDestroyed ? TEXT("true") : TEXT("false"),
		*View.BridgeDebugView.CurrentIntentId.ToString(),
		View.BridgeDebugView.CurrentIntentInitiative,
		View.BridgeDebugView.CurrentIntentResistanceValue,
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
		*View.BridgeDebugView.HoverScreenPosition.ToString());
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

	if (!PartId.IsNone() && HasDuplicatePartIdInWorld())
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementDuplicatePartId",
				"BattleEnemyPart 摆放警告：Actor={0} PartId={1} 与同关卡其他 BattleEnemyPart 重复；当前单敌人战斗会共享同一 runtime part 绑定。"),
			FText::FromString(GetName()),
			FText::FromName(PartId)));
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

bool AWacomBattleEnemyPartActor::HasDuplicatePartIdInWorld() const
{
	if (PartId.IsNone())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AWacomBattleEnemyPartActor> It(World); It; ++It)
	{
		const AWacomBattleEnemyPartActor* Other = *It;
		if (!Other || Other == this)
		{
			continue;
		}
		if (!Other->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !Other->IsTemplate()
			&& Other->PartId == PartId)
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
