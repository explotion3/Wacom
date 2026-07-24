// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomWorldShopActor.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/DrawFrustumComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomWorldShopLayoutAnchorComponent.h"
#include "Components/WacomWorldShopOfferAnchorComponent.h"
#include "Map/WacomMapTypes.h"
#include "Misc/DataValidation.h"
#include "UI/Shop/WacomWorldShopCardGeometry.h"

#define LOCTEXT_NAMESPACE "WacomWorldShopActor"

namespace
{
	constexpr int32 FormalCardDrawWidth =
		FWacomWorldShopCardGeometry::RenderDrawWidth;
	constexpr int32 FormalCardDrawHeight =
		FWacomWorldShopCardGeometry::RenderDrawHeight;
	constexpr int32 FormalLayoutColumns = 4;
	constexpr int32 FormalLayoutRows = 2;
	constexpr float FormalCardGapCm = 8.0f;
	constexpr float FormalInteractionDistanceCm = 2000.0f;
}

AWacomWorldShopActor::AWacomWorldShopActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PresentationRoot =
		CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetupAttachment(GetRootComponent());

	CardLayoutRoot =
		CreateDefaultSubobject<USceneComponent>(TEXT("CardLayoutRoot"));
	CardLayoutRoot->SetupAttachment(PresentationRoot);
	CardLayoutRoot->SetMobility(EComponentMobility::Movable);
	CardLayoutRoot->bEditableWhenInherited = true;

	const float CardWidthCm = FormalCardDrawWidth * CardWorldScale;
	const float CardHeightCm = FormalCardDrawHeight * CardWorldScale;
	const float HorizontalSpacingCm = CardWidthCm + FormalCardGapCm;
	const float VerticalSpacingCm = CardHeightCm + FormalCardGapCm;
	OfferLayoutAnchor01 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_01"));
	OfferLayoutAnchor02 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_02"));
	OfferLayoutAnchor03 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_03"));
	OfferLayoutAnchor04 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_04"));
	OfferLayoutAnchor05 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_05"));
	OfferLayoutAnchor06 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_06"));
	OfferLayoutAnchor07 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_07"));
	OfferLayoutAnchor08 =
		CreateDefaultSubobject<UWacomWorldShopLayoutAnchorComponent>(
			TEXT("OfferLayoutAnchor_08"));

	const TArray<UWacomWorldShopLayoutAnchorComponent*> LayoutAnchors =
	{
		OfferLayoutAnchor01,
		OfferLayoutAnchor02,
		OfferLayoutAnchor03,
		OfferLayoutAnchor04,
		OfferLayoutAnchor05,
		OfferLayoutAnchor06,
		OfferLayoutAnchor07,
		OfferLayoutAnchor08,
	};
	for (int32 Order = 0; Order < LayoutAnchors.Num(); ++Order)
	{
		const int32 Row = Order / FormalLayoutColumns;
		const int32 Column = Order % FormalLayoutColumns;
		UWacomWorldShopLayoutAnchorComponent* Anchor =
			LayoutAnchors[Order];
		Anchor->SetupAttachment(CardLayoutRoot);
		Anchor->ConfigureSlot(
			FName(*FString::Printf(TEXT("Offer.%02d"), Order + 1)),
			Order);
		Anchor->SetRelativeLocation(FVector(
			0.0f,
			(static_cast<float>(Column) - 1.5f)
				* HorizontalSpacingCm,
			(static_cast<float>(1 - Row) - 0.5f)
				* VerticalSpacingCm));
	}

	ShopFocusAnchor =
		CreateDefaultSubobject<USceneComponent>(TEXT("ShopFocusAnchor"));
	ShopFocusAnchor->SetupAttachment(CardLayoutRoot);
	ShopFocusAnchor->SetRelativeLocation(FVector(
		0.0f,
		-0.5f * HorizontalSpacingCm,
		0.0f));
	ShopFocusAnchor->SetMobility(EComponentMobility::Movable);
	ShopFocusAnchor->bEditableWhenInherited = true;

	ShopEntryViewpointComponent =
		CreateDefaultSubobject<UChildActorComponent>(TEXT("ShopEntryViewpoint"));
	ShopEntryViewpointComponent->SetupAttachment(GetRootComponent());
	ShopEntryViewpointComponent->SetRelativeLocation(FVector(320.0f, 0.0f, -20.0f));
	ShopEntryViewpointComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ShopEntryViewpointComponent->SetChildActorClass(
		AWacomFirstPersonViewpointActor::StaticClass());

#if WITH_EDITORONLY_DATA
	ShopViewFrustum =
		CreateEditorOnlyDefaultSubobject<UDrawFrustumComponent>(
			TEXT("ShopViewFrustum"));
	if (ShopViewFrustum)
	{
		ShopViewFrustum->SetupAttachment(ShopEntryViewpointComponent);
		ShopViewFrustum->SetHiddenInGame(true);
		ShopViewFrustum->bIsEditorOnly = true;
		ShopViewFrustum->FrustumAngle = 90.0f;
		ShopViewFrustum->FrustumAspectRatio = 16.0f / 9.0f;
		ShopViewFrustum->FrustumStartDist = 10.0f;
		ShopViewFrustum->FrustumEndDist = CloseBrowsePresetDistanceCm;
	}

	ShopFocusDirection =
		CreateEditorOnlyDefaultSubobject<UArrowComponent>(
			TEXT("ShopFocusDirection"));
	if (ShopFocusDirection)
	{
		ShopFocusDirection->SetupAttachment(ShopFocusAnchor);
		ShopFocusDirection->SetHiddenInGame(true);
		ShopFocusDirection->bIsEditorOnly = true;
		ShopFocusDirection->ArrowSize = 1.25f;
		ShopFocusDirection->ArrowColor = FColor(64, 220, 255);
	}
#endif

	RunMapNodeBinding =
		CreateDefaultSubobject<UWacomRunMapNodeBindingComponent>(
			TEXT("RunMapNodeBinding"));
	RunMapNodeBinding->NodeType = EWacomMapNodeType::Shop;

	TriggerRadius = 350.0f;
	if (USphereComponent* Sphere = GetTriggerSphere())
	{
		Sphere->InitSphereRadius(TriggerRadius);
	}
	if (UBoxComponent* Bounds = GetClickBounds())
	{
		Bounds->SetupAttachment(PresentationRoot);
		Bounds->SetBoxExtent(FVector(60.0f, 210.0f, 140.0f));
	}
}

void AWacomWorldShopActor::BeginPlay()
{
	ApplyInternalAuthoringDefaults();
	Super::BeginPlay();
}

void AWacomWorldShopActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyInternalAuthoringDefaults();
	RefreshEditorCompositionVisuals();
}

TArray<UWacomWorldShopLayoutAnchorComponent*>
AWacomWorldShopActor::GetOfferLayoutAnchorsSorted() const
{
	return {
		OfferLayoutAnchor01.Get(),
		OfferLayoutAnchor02.Get(),
		OfferLayoutAnchor03.Get(),
		OfferLayoutAnchor04.Get(),
		OfferLayoutAnchor05.Get(),
		OfferLayoutAnchor06.Get(),
		OfferLayoutAnchor07.Get(),
		OfferLayoutAnchor08.Get(),
	};
}

FWacomWorldShopPresentationHost
AWacomWorldShopActor::BuildPresentationHost() const
{
	TArray<UWacomWorldShopOfferAnchorComponent*> Anchors;
	for (UWacomWorldShopLayoutAnchorComponent* Anchor :
		GetOfferLayoutAnchorsSorted())
	{
		Anchors.Add(Anchor);
	}
	return FWacomWorldShopPresentationHost::Make(
		*const_cast<AWacomWorldShopActor*>(this),
		Anchors,
		FIntPoint(FormalCardDrawWidth, FormalCardDrawHeight),
		FVector2D(0.5f, 0.5f),
		CardWorldScale,
		FormalInteractionDistanceCm,
		/*bTwoSided*/ true,
		bOverrideCursorLookProfile,
		CursorLookProfileOverride);
}

AWacomFirstPersonViewpointActor*
AWacomWorldShopActor::GetInternalShopEntryViewpoint() const
{
	return ShopEntryViewpointComponent
		? Cast<AWacomFirstPersonViewpointActor>(
			ShopEntryViewpointComponent->GetChildActor())
		: nullptr;
}

UDrawFrustumComponent* AWacomWorldShopActor::GetShopViewFrustumComponent() const
{
#if WITH_EDITORONLY_DATA
	return ShopViewFrustum;
#else
	return nullptr;
#endif
}

UArrowComponent* AWacomWorldShopActor::GetShopFocusDirectionComponent() const
{
#if WITH_EDITORONLY_DATA
	return ShopFocusDirection;
#else
	return nullptr;
#endif
}

AWacomFirstPersonViewpointActor*
AWacomWorldShopActor::ResolveShopEntryViewpoint() const
{
	return GetInternalShopEntryViewpoint();
}

FWacomWorldShopPresentationHost
AWacomWorldShopActor::ResolveWorldShopHost() const
{
	return BuildPresentationHost();
}

void AWacomWorldShopActor::ApplyInternalAuthoringDefaults() const
{
	if (AWacomFirstPersonViewpointActor* Viewpoint =
		GetInternalShopEntryViewpoint())
	{
		Viewpoint->StageBlendTimeSeconds =
			FMath::Max(0.0f, ShopEntryBlendTimeSeconds);
		Viewpoint->StageBlendCurve = ShopEntryBlendCurve;
		Viewpoint->StageBlendEasePower =
			FMath::Max(0.01f, ShopEntryBlendEasePower);
	}
	NormalizeLayoutAnchorContracts();
}

void AWacomWorldShopActor::NormalizeLayoutAnchorContracts() const
{
	if (CardLayoutRoot)
	{
		CardLayoutRoot->SetRelativeScale3D(FVector::OneVector);
	}
	const TArray<UWacomWorldShopLayoutAnchorComponent*> Anchors =
		GetOfferLayoutAnchorsSorted();
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		UWacomWorldShopLayoutAnchorComponent* Anchor = Anchors[Index];
		if (!Anchor)
		{
			continue;
		}
		Anchor->SetRelativeScale3D(FVector::OneVector);
		Anchor->ConfigureSlot(
			FName(*FString::Printf(TEXT("Offer.%02d"), Index + 1)),
			Index);
	}
}

void AWacomWorldShopActor::RefreshEditorCompositionVisuals() const
{
#if WITH_EDITORONLY_DATA
	if (ShopViewFrustum)
	{
		ShopViewFrustum->FrustumEndDist =
			FMath::Max(1.0f, FMath::Abs(CloseBrowsePresetDistanceCm));
		ShopViewFrustum->MarkRenderStateDirty();
	}
#endif
}

void AWacomWorldShopActor::ApplyCloseBrowsePreset()
{
	if (!ShopEntryViewpointComponent || !ShopFocusAnchor
		|| !CardLayoutRoot)
	{
		return;
	}

	Modify();
	ShopEntryViewpointComponent->Modify();
	const FVector FocusLocation = ShopFocusAnchor->GetComponentLocation();
	const FVector BrowseDirection =
		CardLayoutRoot->GetForwardVector().GetSafeNormal();
	const float Distance = FMath::Max(
		1.0f,
		FMath::Abs(CloseBrowsePresetDistanceCm));
	ShopEntryViewpointComponent->SetWorldLocation(
		FocusLocation + BrowseDirection * Distance);
	AlignViewpointToShopFocus();
	RefreshEditorCompositionVisuals();
}

void AWacomWorldShopActor::AlignViewpointToShopFocus()
{
	if (!ShopEntryViewpointComponent || !ShopFocusAnchor)
	{
		return;
	}

	Modify();
	ShopEntryViewpointComponent->Modify();
	const FVector ToFocus =
		ShopFocusAnchor->GetComponentLocation()
		- ShopEntryViewpointComponent->GetComponentLocation();
	if (!ToFocus.IsNearlyZero())
	{
		ShopEntryViewpointComponent->SetWorldRotation(ToFocus.Rotation());
	}
}

void AWacomWorldShopActor::DumpShopComposition() const
{
	const FWacomWorldShopPresentationHost Host =
		BuildPresentationHost();
	if (!Host.IsSet() || !ShopEntryViewpointComponent || !ShopFocusAnchor)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldShopComposition] Dump 被拒绝：Host/Viewpoint/Focus 尚未就绪 Actor=%s"),
			*GetPathName());
		return;
	}

	const FVector ViewLocation =
		ShopEntryViewpointComponent->GetComponentLocation();
	const FTransform ViewTransform =
		ShopEntryViewpointComponent->GetComponentTransform();
	const float FocusDistance =
		FVector::Distance(ViewLocation, ShopFocusAnchor->GetComponentLocation());
	float MaxAbsYaw = 0.0f;
	float MaxAbsPitch = 0.0f;
	float FarthestInteractionDistance = 0.0f;
	int32 CenteredFullyVisibleCount = 0;
	constexpr float HorizontalHalfFovDegrees = 45.0f;
	const float VerticalHalfFovDegrees = FMath::RadiansToDegrees(
		FMath::Atan(
			FMath::Tan(FMath::DegreesToRadians(HorizontalHalfFovDegrees))
			* (9.0f / 16.0f)));
	const float CardHalfWidthCm =
		0.5f * Host.CardDrawSize.X * Host.CardWorldScale;
	const float CardHalfHeightCm =
		0.5f * Host.CardDrawSize.Y * Host.CardWorldScale;

	for (const UWacomWorldShopOfferAnchorComponent* Anchor :
		Host.GetEnabledOfferAnchorsSorted())
	{
		if (!Anchor)
		{
			continue;
		}
		const FVector LocalCenter = ViewTransform.InverseTransformPosition(
			Anchor->GetComponentLocation());
		const float ForwardDistance = FMath::Max(
			1.0f,
			FMath::Abs(LocalCenter.X));
		const float YawDegrees = FMath::RadiansToDegrees(
			FMath::Atan2(LocalCenter.Y, LocalCenter.X));
		const float PitchDegrees = FMath::RadiansToDegrees(
			FMath::Atan2(
				LocalCenter.Z,
				FMath::Sqrt(
					FMath::Square(LocalCenter.X)
					+ FMath::Square(LocalCenter.Y))));
		const float CardHalfYaw = FMath::RadiansToDegrees(
			FMath::Atan2(CardHalfWidthCm, ForwardDistance));
		const float CardHalfPitch = FMath::RadiansToDegrees(
			FMath::Atan2(CardHalfHeightCm, ForwardDistance));
		MaxAbsYaw = FMath::Max(MaxAbsYaw, FMath::Abs(YawDegrees));
		MaxAbsPitch = FMath::Max(MaxAbsPitch, FMath::Abs(PitchDegrees));
		FarthestInteractionDistance = FMath::Max(
			FarthestInteractionDistance,
			FVector::Distance(ViewLocation, Anchor->GetComponentLocation()));
		if (FMath::Abs(YawDegrees) + CardHalfYaw <= HorizontalHalfFovDegrees
			&& FMath::Abs(PitchDegrees) + CardHalfPitch
				<= VerticalHalfFovDegrees)
		{
			++CenteredFullyVisibleCount;
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WorldShopComposition] Actor=%s FocusDistance=%.1fcm MaxCenterYaw=%.2fdeg MaxCenterPitch=%.2fdeg CenterFullyVisible=%d/%d FarthestInteraction=%.1fcm Limit=%.1fcm"),
		*GetPathName(),
		FocusDistance,
		MaxAbsYaw,
		MaxAbsPitch,
		CenteredFullyVisibleCount,
		Host.GetEnabledOfferAnchorsSorted().Num(),
		FarthestInteractionDistance,
		Host.InteractionDistance);
}

#if WITH_EDITOR
EDataValidationResult AWacomWorldShopActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || IsTemplate())
	{
		return Result;
	}

	const UClass* ViewpointClass = ShopEntryViewpointComponent
		? ShopEntryViewpointComponent->GetChildActorClass()
		: nullptr;
	if (!ViewpointClass
		|| !ViewpointClass->IsChildOf(
			AWacomFirstPersonViewpointActor::StaticClass()))
	{
		Context.AddError(LOCTEXT(
			"InvalidInternalViewpointClass",
			"组合式实体商店缺少合法的内部 AWacomFirstPersonViewpointActor ChildActor。"));
		Result = EDataValidationResult::Invalid;
	}

	if (!RunMapNodeBinding
		|| RunMapNodeBinding->NodeId.IsNone()
		|| RunMapNodeBinding->NodeType != EWacomMapNodeType::Shop)
	{
		Context.AddError(LOCTEXT(
			"InvalidRunMapBinding",
			"组合式实体商店必须配置非空 NodeId，且 NodeType 必须为 Shop。"));
		Result = EDataValidationResult::Invalid;
	}

	if (!ShopFocusAnchor
		|| ShopFocusAnchor->GetAttachParent() != CardLayoutRoot
		|| ShopFocusAnchor->GetRelativeTransform().ContainsNaN())
	{
		Context.AddError(LOCTEXT(
			"InvalidShopFocus",
			"组合式实体商店必须具有附着在 CardLayoutRoot 下的合法 ShopFocusAnchor。"));
		Result = EDataValidationResult::Invalid;
	}

	if (!CardLayoutRoot
		|| CardLayoutRoot->GetAttachParent() != PresentationRoot
		|| CardLayoutRoot->GetRelativeTransform().ContainsNaN()
		|| !CardLayoutRoot->GetRelativeScale3D().Equals(
			FVector::OneVector,
			UE_KINDA_SMALL_NUMBER))
	{
		Context.AddError(LOCTEXT(
			"InvalidCardLayoutRoot",
			"组合式实体商店必须具有附着在 PresentationRoot 下、缩放为 1 的合法 CardLayoutRoot。"));
		Result = EDataValidationResult::Invalid;
	}

	if (!FMath::IsFinite(CardWorldScale) || CardWorldScale <= 0.0f)
	{
		Context.AddError(LOCTEXT(
			"InvalidCardWorldScale",
			"组合式实体商店的 CardWorldScale 必须是大于 0 的有限值。"));
		Result = EDataValidationResult::Invalid;
	}

	const TArray<UWacomWorldShopLayoutAnchorComponent*> LayoutAnchors =
		GetOfferLayoutAnchorsSorted();
	if (LayoutAnchors.Num() != FormalLayoutColumns * FormalLayoutRows)
	{
		Context.AddError(LOCTEXT(
			"InvalidLayoutAnchorCount",
			"组合式实体商店必须具有八个稳定的 OfferLayoutAnchor。"));
		Result = EDataValidationResult::Invalid;
	}
	for (int32 Index = 0; Index < LayoutAnchors.Num(); ++Index)
	{
		const UWacomWorldShopLayoutAnchorComponent* Anchor =
			LayoutAnchors[Index];
		const FName ExpectedSlotId(*FString::Printf(
			TEXT("Offer.%02d"),
			Index + 1));
		if (!Anchor
			|| Anchor->GetAttachParent() != CardLayoutRoot
			|| Anchor->GetSlotId() != ExpectedSlotId
			|| Anchor->GetSlotOrder() != Index
			|| Anchor->GetRelativeTransform().ContainsNaN()
			|| !Anchor->GetRelativeScale3D().Equals(
				FVector::OneVector,
				UE_KINDA_SMALL_NUMBER))
		{
			Context.AddError(FText::Format(
				LOCTEXT(
					"InvalidLayoutAnchor",
					"组合式实体商店的商品槽 {0} 配置无效；槽身份、父级、Transform 和单位缩放合同必须保持稳定。"),
				FText::AsNumber(Index + 1)));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (!FMath::IsFinite(CloseBrowsePresetDistanceCm)
		|| CloseBrowsePresetDistanceCm <= 0.0f)
	{
		Context.AddError(LOCTEXT(
			"InvalidCloseBrowseDistance",
			"组合式实体商店的 CloseBrowsePresetDistanceCm 必须是大于 0 的有限厘米值。"));
		Result = EDataValidationResult::Invalid;
	}

	if (bOverrideCursorLookProfile && !CursorLookProfileOverride.IsFinite())
	{
		Context.AddError(LOCTEXT(
			"InvalidCursorLookProfile",
			"组合式实体商店启用浏览覆盖时必须提供有限的 CursorLookProfileOverride。"));
		Result = EDataValidationResult::Invalid;
	}

	const FWacomWorldShopPresentationHost Host =
		BuildPresentationHost();
	if (Host.IsSet() && ShopFocusAnchor && ShopEntryViewpointComponent)
	{
		const float FocusDistance = FVector::Distance(
			ShopEntryViewpointComponent->GetComponentLocation(),
			ShopFocusAnchor->GetComponentLocation());
		if (!FMath::IsFinite(FocusDistance)
			|| FocusDistance <= UE_KINDA_SMALL_NUMBER
			|| FocusDistance > Host.InteractionDistance)
		{
			Context.AddError(LOCTEXT(
				"InvalidCompositionDistance",
				"组合式实体商店的 Viewpoint 到 ShopFocus 距离必须大于 0，且不得超过 Host InteractionDistance。"));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (WorldShopHost || ShopEntryViewpoint)
	{
		Context.AddError(LOCTEXT(
			"ExternalReferencesAreForbidden",
			"组合式实体商店不得填写旧 WorldShopHost 或 ShopEntryViewpoint 外部引用；正式 Actor 直接拥有商品 Anchor，并只保留内部 Viewpoint。"));
		Result = EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
