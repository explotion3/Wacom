// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * App-private motion primitives shared by first-person Battle cards and Backpack cards.
 *
 * The kernel owns no gameplay or widget state. Callers provide semantic targets and retain
 * lifecycle ownership; these helpers only make position/rotation motion frame-rate independent,
 * interruptible, and safe to retarget from the current visual pose.
 */
class WACOMAPP_API FWacomCardMotionKernel
{
public:
	static float ComputeExponentialAlpha(float ResponseSpeed, float DeltaTime);
	static float ComputeEaseOutAlpha(float LinearAlpha, float EasePower = 3.0f);
	static float LerpAngleShortest(float FromDegrees, float ToDegrees, float Alpha);
	static FVector2D StepExponential(
		FVector2D Current,
		FVector2D Target,
		float ResponseSpeed,
		float DeltaTime);
	static FVector2D StepExponentialWithMaximumLag(
		FVector2D Current,
		FVector2D Target,
		float ResponseSpeed,
		float MaximumLagPixels,
		float DeltaTime);
	static bool IsNear(
		FVector2D CurrentPosition,
		FVector2D TargetPosition,
		float CurrentAngleDegrees,
		float TargetAngleDegrees,
		float PositionTolerancePixels = 0.5f,
		float AngleToleranceDegrees = 0.05f);
};
