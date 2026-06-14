// Copyright Wacom. All Rights Reserved.

#include "Components/WacomFirstPersonWalkBobComponent.h"

namespace
{
	constexpr float MaxStepPhaseAdvancePerFrame = 0.5f;

	float InterpWalkBobStrength(
		float Current,
		float Target,
		float DeltaTime,
		float InterpSpeed)
	{
		if (DeltaTime <= 0.0f || InterpSpeed <= 0.0f)
		{
			return Target;
		}
		return FMath::FInterpTo(Current, Target, DeltaTime, InterpSpeed);
	}

	float SmoothStep01(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float SmoothSegment(
		float Phase,
		float StartPhase,
		float EndPhase,
		float StartValue,
		float EndValue)
	{
		const float Denominator = FMath::Max(EndPhase - StartPhase, KINDA_SMALL_NUMBER);
		const float Alpha = SmoothStep01((Phase - StartPhase) / Denominator);
		return FMath::Lerp(StartValue, EndValue, Alpha);
	}

	float EvaluateFootstepVerticalOffset(
		float Phase,
		float VerticalAmplitudeCm,
		float FootPlantDropCm)
	{
		const float Up = FMath::Max(0.0f, VerticalAmplitudeCm);
		const float Down = -FMath::Max(0.0f, FootPlantDropCm);
		if (Phase < 0.12f)
		{
			return SmoothSegment(Phase, 0.0f, 0.12f, Down, 0.0f);
		}
		if (Phase < 0.50f)
		{
			return SmoothSegment(Phase, 0.12f, 0.50f, 0.0f, Up);
		}
		if (Phase < 0.82f)
		{
			return SmoothSegment(Phase, 0.50f, 0.82f, Up, 0.0f);
		}
		return SmoothSegment(Phase, 0.82f, 1.0f, 0.0f, Down);
	}

	float EvaluateFootstepPitchUnit(float Phase)
	{
		if (Phase < 0.12f)
		{
			return SmoothSegment(Phase, 0.0f, 0.12f, 1.0f, 0.0f);
		}
		if (Phase < 0.50f)
		{
			return SmoothSegment(Phase, 0.12f, 0.50f, 0.0f, -1.0f);
		}
		if (Phase < 0.82f)
		{
			return SmoothSegment(Phase, 0.50f, 0.82f, -1.0f, 0.0f);
		}
		return SmoothSegment(Phase, 0.82f, 1.0f, 0.0f, 1.0f);
	}
}

UWacomFirstPersonWalkBobComponent::UWacomFirstPersonWalkBobComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomFirstPersonWalkBobComponent::UpdateWalkBobFromMovementDelta(
	float DeltaTime,
	float DistanceDeltaCm,
	float ReferenceSpeedCmPerSecond)
{
	if (!bEnableWalkBob)
	{
		ResetWalkBob(/*bSnapToZero*/true);
		return;
	}

	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	const float SafeDistanceDelta = FMath::Max(0.0f, DistanceDeltaCm);
	const float DeadZone = FMath::Max(0.0f, MovementDeadZoneCm);
	const bool bActuallyMoved = SafeDeltaTime > KINDA_SMALL_NUMBER
		&& SafeDistanceDelta > DeadZone;
	const float ActualSpeed = bActuallyMoved
		? SafeDistanceDelta / SafeDeltaTime
		: 0.0f;
	const float SafeReferenceSpeed = FMath::Max(ReferenceSpeedCmPerSecond, KINDA_SMALL_NUMBER);
	const float SpeedRatio = FMath::Clamp(ActualSpeed / SafeReferenceSpeed, 0.0f, 1.0f);
	const float TargetStrength = SpeedRatio;
	const float InterpSpeed = TargetStrength > CurrentStrength
		? FMath::Max(0.0f, BlendInSpeed)
		: FMath::Max(0.0f, BlendOutSpeed);
	CurrentStrength = InterpWalkBobStrength(
		CurrentStrength,
		TargetStrength,
		SafeDeltaTime,
		InterpSpeed);

	if (bActuallyMoved && SpeedRatio > KINDA_SMALL_NUMBER)
	{
		const float SafeStepDistance = FMath::Max(StepDistanceCm, KINDA_SMALL_NUMBER);
		const float PhaseAdvance = FMath::Clamp(
			SafeDistanceDelta / SafeStepDistance,
			0.0f,
			MaxStepPhaseAdvancePerFrame);
		StepPhaseNormalized = FMath::Fmod(StepPhaseNormalized + PhaseAdvance, 1.0f);
	}

	RecalculateOffsets();
}

void UWacomFirstPersonWalkBobComponent::ResetWalkBob(bool bSnapToZero)
{
	if (bSnapToZero)
	{
		StepPhaseNormalized = 0.0f;
		CurrentStrength = 0.0f;
		ClearOffsets();
		return;
	}

	CurrentStrength = 0.0f;
	RecalculateOffsets();
}

void UWacomFirstPersonWalkBobComponent::RecalculateOffsets()
{
	if (CurrentStrength <= KINDA_SMALL_NUMBER)
	{
		ClearOffsets();
		return;
	}

	const float Vertical = EvaluateFootstepVerticalOffset(
		StepPhaseNormalized,
		VerticalAmplitudeCm,
		FootPlantDropCm) * CurrentStrength;
	const float Lateral = FMath::Sin(StepPhaseNormalized * UE_PI * 2.0f)
		* FMath::Max(0.0f, LateralAmplitudeCm)
		* CurrentStrength;
	const float Pitch = EvaluateFootstepPitchUnit(StepPhaseNormalized)
		* FMath::Max(0.0f, PitchAmplitudeDegrees)
		* CurrentStrength;
	const float Roll = FMath::Sin(StepPhaseNormalized * UE_PI * 2.0f)
		* FMath::Max(0.0f, RollAmplitudeDegrees)
		* CurrentStrength;

	CurrentLocationOffset = FVector(0.0f, Lateral, Vertical);
	CurrentRotationOffset = FRotator(Pitch, 0.0f, Roll);
}

void UWacomFirstPersonWalkBobComponent::ClearOffsets()
{
	CurrentLocationOffset = FVector::ZeroVector;
	CurrentRotationOffset = FRotator::ZeroRotator;
}
