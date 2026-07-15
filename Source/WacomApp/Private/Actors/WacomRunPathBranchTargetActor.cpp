// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunPathBranchTargetActor.h"

#include "Components/BoxComponent.h"

AWacomRunPathBranchTargetActor::AWacomRunPathBranchTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	SetRootComponent(ClickBounds);
	ClickBounds->SetBoxExtent(FVector(45.0f));
	ClickBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBounds->SetGenerateOverlapEvents(false);
}

bool AWacomRunPathBranchTargetActor::RequestBranch() const
{
	if (EdgeId.IsNone())
	{
		return false;
	}
	BranchRequestedNative.Broadcast(EdgeId);
	return true;
}
