// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunTunnelBranchTargetActor.h"

#include "Components/BoxComponent.h"
#include "Components/WacomRunTunnelPrototypeComponent.h"

AWacomRunTunnelBranchTargetActor::AWacomRunTunnelBranchTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	SetRootComponent(ClickBounds);
	ClickBounds->SetBoxExtent(FVector(45.0f, 45.0f, 45.0f));
	ClickBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBounds->SetGenerateOverlapEvents(false);
}

bool AWacomRunTunnelBranchTargetActor::RequestBranch(UWacomRunTunnelPrototypeComponent* TunnelComponent) const
{
	return TunnelComponent && TunnelComponent->SwitchToSegment(TargetSegment, TargetStartDistance);
}

