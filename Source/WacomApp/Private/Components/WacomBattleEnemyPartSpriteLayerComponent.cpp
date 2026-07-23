// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"

#include "Components/WacomBattleEnemyPartComponent.h"
#include "Interaction/WacomInteractionCollisionChannels.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "Types/WacomInteractionTargetTypes.h"

UWacomBattleEnemyPartSpriteLayerComponent::UWacomBattleEnemyPartSpriteLayerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
}

void UWacomBattleEnemyPartSpriteLayerComponent::ConfigureInteractionCollision(
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

void UWacomBattleEnemyPartSpriteLayerComponent::ClearInteractionCollision()
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

bool UWacomBattleEnemyPartSpriteLayerComponent::IsInteractionCollisionReady() const
{
	const UBodySetup* BodySetup = StableInteractionCollisionSprite
		? StableInteractionCollisionSprite->BodySetup
		: nullptr;
	return IsValid(InteractionPart)
		&& IsValid(StableInteractionCollisionSprite)
		&& BodySetup
		&& BodySetup->AggGeom.GetElementCount() > 0;
}

UPaperSprite* UWacomBattleEnemyPartSpriteLayerComponent::GetStableInteractionCollisionSprite() const
{
	return StableInteractionCollisionSprite;
}

FWacomInteractionTargetHandle
UWacomBattleEnemyPartSpriteLayerComponent::BuildWorldTargetHandle() const
{
	return InteractionPart
		? InteractionPart->BuildWorldTargetHandle()
		: FWacomInteractionTargetHandle();
}

UBodySetup* UWacomBattleEnemyPartSpriteLayerComponent::GetBodySetup()
{
	return StableInteractionCollisionSprite && StableInteractionCollisionSprite->BodySetup
		? StableInteractionCollisionSprite->BodySetup.Get()
		: Super::GetBodySetup();
}

void UWacomBattleEnemyPartSpriteLayerComponent::OnRegister()
{
	Super::OnRegister();
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
}

void UWacomBattleEnemyPartSpriteLayerComponent::OnUnregister()
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
EDataValidationResult UWacomBattleEnemyPartSpriteLayerComponent::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (LayerId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part Sprite Layer 的 LayerId 不能为空。")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
