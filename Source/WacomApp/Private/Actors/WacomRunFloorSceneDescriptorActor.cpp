// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunFloorSceneDescriptorActor.h"

#include "Components/SceneComponent.h"

AWacomRunFloorSceneDescriptorActor::AWacomRunFloorSceneDescriptorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}
