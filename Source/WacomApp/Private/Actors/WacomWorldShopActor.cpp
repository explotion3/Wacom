// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomWorldShopActor.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomWorldShopHostActor.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Map/WacomMapTypes.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "WacomWorldShopActor"

AWacomWorldShopActor::AWacomWorldShopActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PresentationRoot =
		CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetupAttachment(GetRootComponent());

	WorldShopHostComponent =
		CreateDefaultSubobject<UChildActorComponent>(TEXT("WorldShopHost"));
	WorldShopHostComponent->SetupAttachment(PresentationRoot);
	WorldShopHostComponent->SetChildActorClass(
		AWacomWorldShopHostActor::StaticClass());

	ShopEntryViewpointComponent =
		CreateDefaultSubobject<UChildActorComponent>(TEXT("ShopEntryViewpoint"));
	ShopEntryViewpointComponent->SetupAttachment(GetRootComponent());
	ShopEntryViewpointComponent->SetRelativeLocation(FVector(320.0f, 0.0f, -20.0f));
	ShopEntryViewpointComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ShopEntryViewpointComponent->SetChildActorClass(
		AWacomFirstPersonViewpointActor::StaticClass());

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
	ApplyInternalViewpointDefaults();
	Super::BeginPlay();
}

void AWacomWorldShopActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyInternalViewpointDefaults();
}

AWacomWorldShopHostActor* AWacomWorldShopActor::GetInternalWorldShopHost() const
{
	return WorldShopHostComponent
		? Cast<AWacomWorldShopHostActor>(
			WorldShopHostComponent->GetChildActor())
		: nullptr;
}

AWacomFirstPersonViewpointActor*
AWacomWorldShopActor::GetInternalShopEntryViewpoint() const
{
	return ShopEntryViewpointComponent
		? Cast<AWacomFirstPersonViewpointActor>(
			ShopEntryViewpointComponent->GetChildActor())
		: nullptr;
}

AWacomFirstPersonViewpointActor*
AWacomWorldShopActor::ResolveShopEntryViewpoint() const
{
	return GetInternalShopEntryViewpoint();
}

AWacomWorldShopHostActor* AWacomWorldShopActor::ResolveWorldShopHost() const
{
	return GetInternalWorldShopHost();
}

void AWacomWorldShopActor::ApplyInternalViewpointDefaults() const
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

	const UClass* HostClass = WorldShopHostComponent
		? WorldShopHostComponent->GetChildActorClass()
		: nullptr;
	if (!HostClass || !HostClass->IsChildOf(AWacomWorldShopHostActor::StaticClass()))
	{
		Context.AddError(LOCTEXT(
			"InvalidInternalHostClass",
			"组合式实体商店缺少合法的内部 AWacomWorldShopHostActor ChildActor。"));
		Result = EDataValidationResult::Invalid;
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

	if (WorldShopHost || ShopEntryViewpoint)
	{
		Context.AddError(LOCTEXT(
			"ExternalReferencesAreForbidden",
			"组合式实体商店不得填写旧 WorldShopHost 或 ShopEntryViewpoint 外部引用；正式 Actor 只使用内部 ChildActor。"));
		Result = EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
