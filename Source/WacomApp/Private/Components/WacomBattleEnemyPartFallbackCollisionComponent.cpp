// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartFallbackCollisionComponent.h"

#include "Components/WacomBattleEnemyPartComponent.h"
#include "Interaction/WacomInteractionCollisionChannels.h"

UWacomBattleEnemyPartFallbackCollisionComponent::
UWacomBattleEnemyPartFallbackCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(55.0f, 45.0f, 55.0f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(
		Wacom::Interaction::BattleEnemyPartTraceChannel,
		ECR_Block);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
	SetLineThickness(1.0f);
}

void UWacomBattleEnemyPartFallbackCollisionComponent::InitializeForPart(
	UWacomBattleEnemyPartComponent& InPart)
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPart = &InPart;
}

void UWacomBattleEnemyPartFallbackCollisionComponent::ConfigureFallbackBounds(
	const FVector& RelativeCenter,
	const FVector& HalfExtent,
	bool bEnableCollision)
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRelativeLocation(RelativeCenter);
	SetRelativeRotation(FRotator::ZeroRotator);
	SetRelativeScale3D(FVector::OneVector);
	SetBoxExtent(HalfExtent.GetAbs().ComponentMax(FVector(6.0f)), false);
	SetCollisionEnabled(
		bEnableCollision
			? ECollisionEnabled::QueryOnly
			: ECollisionEnabled::NoCollision);
}

void UWacomBattleEnemyPartFallbackCollisionComponent::DisableFallbackCollision()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FWacomInteractionTargetHandle
UWacomBattleEnemyPartFallbackCollisionComponent::BuildWorldTargetHandle() const
{
	return InteractionPart
		? InteractionPart->BuildWorldTargetHandle()
		: FWacomInteractionTargetHandle();
}
