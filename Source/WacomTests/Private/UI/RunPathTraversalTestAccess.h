// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomRunPathTraversalComponent;
class AWacomRunPathSegmentActor;

struct FWacomRunPathTraversalTestAccess
{
	static void Tick(UWacomRunPathTraversalComponent& Component, float DeltaTime);
	static bool BeginTraversalAtDistance(
		UWacomRunPathTraversalComponent& Component,
		AWacomRunPathSegmentActor* Path,
		float DistanceAlongSpline);
	static bool HasCursorLookOverride(const UWacomRunPathTraversalComponent& Component);
	static FVector2D GetCursorLookOverrideNormalized(
		const UWacomRunPathTraversalComponent& Component);
	static float GetMoveAxis(const UWacomRunPathTraversalComponent& Component);
};
