// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"

#include "Components/WacomBattleEnemyPartComponent.h"
#include "Interaction/WacomInteractionCollisionChannels.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "Types/WacomInteractionTargetTypes.h"

UWacomBattleEnemyPartFlipbookLayerComponent::UWacomBattleEnemyPartFlipbookLayerComponent()
{
	// UPaperFlipbookComponent advances playback from its component tick. Keep that
	// base contract in both PIE and editor viewports; runtime state controls
	// playback through Play/Stop instead of disabling the component tick.
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
}

void UWacomBattleEnemyPartFlipbookLayerComponent::ConfigureInteractionCollision(
	UWacomBattleEnemyPartComponent* InPart,
	UPaperSprite* InStableCollisionSprite,
	bool bEnableCollision)
{
	if (!IsRegistered())
	{
		InteractionPart = InPart;
		StableInteractionCollisionSprite = InStableCollisionSprite;
		return;
	}
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPart = InPart;
	StableInteractionCollisionSprite = InStableCollisionSprite;
	RecreatePhysicsState();
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(
		Wacom::Interaction::BattleEnemyPartTraceChannel,
		ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCollisionEnabled(
		bEnableCollision && IsInteractionCollisionReady()
			? ECollisionEnabled::QueryOnly
			: ECollisionEnabled::NoCollision);
}

void UWacomBattleEnemyPartFlipbookLayerComponent::ClearInteractionCollision()
{
	const bool bCanRecreatePhysics = IsRegistered();
	if (bCanRecreatePhysics)
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	InteractionPart = nullptr;
	StableInteractionCollisionSprite = nullptr;
	if (bCanRecreatePhysics)
	{
		RecreatePhysicsState();
	}
}

bool UWacomBattleEnemyPartFlipbookLayerComponent::IsInteractionCollisionReady() const
{
	const UBodySetup* BodySetup = StableInteractionCollisionSprite
		? StableInteractionCollisionSprite->BodySetup
		: nullptr;
	return IsValid(InteractionPart)
		&& IsValid(StableInteractionCollisionSprite)
		&& BodySetup
		&& BodySetup->AggGeom.GetElementCount() > 0;
}

UPaperSprite* UWacomBattleEnemyPartFlipbookLayerComponent::GetStableInteractionCollisionSprite() const
{
	return StableInteractionCollisionSprite;
}

FWacomInteractionTargetHandle
UWacomBattleEnemyPartFlipbookLayerComponent::BuildWorldTargetHandle() const
{
	return InteractionPart
		? InteractionPart->BuildWorldTargetHandle()
		: FWacomInteractionTargetHandle();
}

UBodySetup* UWacomBattleEnemyPartFlipbookLayerComponent::GetBodySetup()
{
	return StableInteractionCollisionSprite && StableInteractionCollisionSprite->BodySetup
		? StableInteractionCollisionSprite->BodySetup.Get()
		: Super::GetBodySetup();
}

void UWacomBattleEnemyPartFlipbookLayerComponent::OnRegister()
{
	Super::OnRegister();
	SetPlaybackPosition(FMath::Max(0.0f, InitialPlaybackPositionSeconds), false);
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
}

void UWacomBattleEnemyPartFlipbookLayerComponent::OnUnregister()
{
	InteractionPart = nullptr;
	StableInteractionCollisionSprite = nullptr;
	Super::OnUnregister();
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyPartFlipbookLayerComponent::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (LayerId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part Flipbook Layer 的 LayerId 不能为空。")));
		Result = EDataValidationResult::Invalid;
	}
	if (!FMath::IsFinite(InitialPlaybackPositionSeconds) || InitialPlaybackPositionSeconds < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("InitialPlaybackPositionSeconds 必须是非负有限秒数。")));
		Result = EDataValidationResult::Invalid;
	}
	if (DestroyedFlipbook
		&& (!FMath::IsFinite(DestroyedPlayRate) || DestroyedPlayRate <= 0.0f))
	{
		Context.AddError(FText::FromString(TEXT("DestroyedPlayRate 必须是有限正数。")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
