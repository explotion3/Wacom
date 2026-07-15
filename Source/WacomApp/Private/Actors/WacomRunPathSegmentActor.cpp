// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunPathSegmentActor.h"

#include "Components/SplineComponent.h"

AWacomRunPathSegmentActor::AWacomRunPathSegmentActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
	SetRootComponent(PathSpline);
	PathSpline->SetMobility(EComponentMobility::Movable);
}

float AWacomRunPathSegmentActor::GetSplineLength() const
{
	return PathSpline ? PathSpline->GetSplineLength() : 0.0f;
}

float AWacomRunPathSegmentActor::GetClampedDistance(const float Distance) const
{
	return FMath::Clamp(Distance, 0.0f, GetSplineLength());
}

FTransform AWacomRunPathSegmentActor::GetSplineTransformAtDistance(const float Distance) const
{
	return PathSpline
		? PathSpline->GetTransformAtDistanceAlongSpline(
			GetClampedDistance(Distance),
			ESplineCoordinateSpace::World,
			true)
		: GetActorTransform();
}
