// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardMotionKernel.h"

float FWacomCardMotionKernel::ComputeExponentialAlpha(float ResponseSpeed, float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return 0.0f;
	}
	return ResponseSpeed <= 0.0f
		? 1.0f
		: 1.0f - FMath::Exp(-FMath::Max(0.0f, ResponseSpeed) * DeltaTime);
}

float FWacomCardMotionKernel::ComputeEaseOutAlpha(float LinearAlpha, float EasePower)
{
	const float Clamped = FMath::Clamp(LinearAlpha, 0.0f, 1.0f);
	return 1.0f - FMath::Pow(1.0f - Clamped, FMath::Max(1.0f, EasePower));
}

float FWacomCardMotionKernel::LerpAngleShortest(float FromDegrees, float ToDegrees, float Alpha)
{
	return FromDegrees
		+ FMath::FindDeltaAngleDegrees(FromDegrees, ToDegrees) * FMath::Clamp(Alpha, 0.0f, 1.0f);
}

FVector2D FWacomCardMotionKernel::StepExponential(
	FVector2D Current,
	FVector2D Target,
	float ResponseSpeed,
	float DeltaTime)
{
	return FMath::Lerp(Current, Target, ComputeExponentialAlpha(ResponseSpeed, DeltaTime));
}

FVector2D FWacomCardMotionKernel::StepExponentialWithMaximumLag(
	FVector2D Current,
	FVector2D Target,
	float ResponseSpeed,
	float MaximumLagPixels,
	float DeltaTime)
{
	FVector2D Next = StepExponential(Current, Target, ResponseSpeed, DeltaTime);
	const float SafeMaximumLag = FMath::Max(0.0f, MaximumLagPixels);
	if (SafeMaximumLag <= UE_SMALL_NUMBER)
	{
		return Target;
	}
	const FVector2D Lag = Next - Target;
	if (Lag.SizeSquared() > FMath::Square(SafeMaximumLag))
	{
		Next = Target + Lag.GetSafeNormal() * SafeMaximumLag;
	}
	return Next;
}

bool FWacomCardMotionKernel::IsNear(
	FVector2D CurrentPosition,
	FVector2D TargetPosition,
	float CurrentAngleDegrees,
	float TargetAngleDegrees,
	float PositionTolerancePixels,
	float AngleToleranceDegrees)
{
	return CurrentPosition.Equals(TargetPosition, FMath::Max(0.0f, PositionTolerancePixels))
		&& FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentAngleDegrees, TargetAngleDegrees))
			<= FMath::Max(0.0f, AngleToleranceDegrees);
}
