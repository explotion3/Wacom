// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunMapNodeAnchorActor.h"

#include "Components/SceneComponent.h"

AWacomRunMapNodeAnchorActor::AWacomRunMapNodeAnchorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}
