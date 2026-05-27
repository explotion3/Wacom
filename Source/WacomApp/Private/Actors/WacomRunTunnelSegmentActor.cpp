// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunTunnelSegmentActor.h"

#include "Components/SplineComponent.h"
#include "Components/WacomRunTunnelPrototypeComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "TimerManager.h"

AWacomRunTunnelSegmentActor::AWacomRunTunnelSegmentActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
	SetRootComponent(PathSpline);
	PathSpline->SetMobility(EComponentMobility::Movable);
}

void AWacomRunTunnelSegmentActor::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoActivateOnBeginPlay)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &AWacomRunTunnelSegmentActor::TryAutoActivateLocalPlayer);
			World->GetTimerManager().SetTimerForNextTick(Delegate);
		}
	}
}

float AWacomRunTunnelSegmentActor::GetSplineLength() const
{
	return PathSpline ? PathSpline->GetSplineLength() : 0.0f;
}

float AWacomRunTunnelSegmentActor::GetClampedDistance(float Distance) const
{
	return FMath::Clamp(Distance, 0.0f, GetSplineLength());
}

FTransform AWacomRunTunnelSegmentActor::GetSplineTransformAtDistance(float Distance) const
{
	if (!PathSpline)
	{
		return GetActorTransform();
	}

	return PathSpline->GetTransformAtDistanceAlongSpline(
		GetClampedDistance(Distance),
		ESplineCoordinateSpace::World,
		true);
}

void AWacomRunTunnelSegmentActor::TryAutoActivateLocalPlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	AWacomPlayerCharacter* Character = PC ? Cast<AWacomPlayerCharacter>(PC->GetPawn()) : nullptr;
	UWacomRunTunnelPrototypeComponent* TunnelComponent =
		Character ? Character->FindComponentByClass<UWacomRunTunnelPrototypeComponent>() : nullptr;
	if (!TunnelComponent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunTunnelSegment] AutoActivate failed: no local Wacom player character tunnel component for %s"),
			*GetName());
		return;
	}

	TunnelComponent->ActivateTunnelPrototype(this, AutoActivateStartDistance);
}

