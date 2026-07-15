// Copyright Wacom. All Rights Reserved.

#include "UI/RunPathTraversalTestAccess.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/WacomRunPathTraversalComponent.h"

void FWacomRunPathTraversalTestAccess::Tick(
	UWacomRunPathTraversalComponent& Component,
	const float DeltaTime)
{
	Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
}

bool FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(
	UWacomRunPathTraversalComponent& Component,
	AWacomRunPathSegmentActor* Path,
	const float DistanceAlongSpline)
{
	if (!Component.BeginTraversal(Path))
	{
		return false;
	}
	Component.DistanceAlongSpline = Path
		? Path->GetClampedDistance(DistanceAlongSpline)
		: 0.0f;
	Component.ApplyViewTransform(0.0f, 0.0f);
	return true;
}

bool FWacomRunPathTraversalTestAccess::HasCursorLookOverride(
	const UWacomRunPathTraversalComponent& Component)
{
	return Component.bHasCursorLookOverride;
}

FVector2D FWacomRunPathTraversalTestAccess::GetCursorLookOverrideNormalized(
	const UWacomRunPathTraversalComponent& Component)
{
	return Component.CursorLookOverrideNormalized;
}
