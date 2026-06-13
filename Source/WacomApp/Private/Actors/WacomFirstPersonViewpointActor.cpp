// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomFirstPersonViewpointActor.h"

#include "Components/SceneComponent.h"

#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#endif

AWacomFirstPersonViewpointActor::AWacomFirstPersonViewpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ViewRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ViewRoot"));
	RootComponent = ViewRootComponent;

#if WITH_EDITORONLY_DATA
	ViewDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ViewDirection"));
	ViewDirectionArrow->SetupAttachment(RootComponent);
	ViewDirectionArrow->ArrowColor = FColor(80, 180, 255);
	ViewDirectionArrow->ArrowSize = 1.5f;
	ViewDirectionArrow->bTreatAsASprite = true;
#endif
}
