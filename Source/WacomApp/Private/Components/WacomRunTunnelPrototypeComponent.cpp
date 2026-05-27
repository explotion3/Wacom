// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunTunnelPrototypeComponent.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"

UWacomRunTunnelPrototypeComponent::UWacomRunTunnelPrototypeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWacomRunTunnelPrototypeComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

void UWacomRunTunnelPrototypeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateTunnelPrototype();
	Super::EndPlay(EndPlayReason);
}

void UWacomRunTunnelPrototypeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bTunnelPrototypeActive)
	{
		return;
	}
	if (bTunnelPrototypeSuspended)
	{
		return;
	}

	if (AWacomRunTunnelSegmentActor* Segment = ActiveSegment.Get())
	{
		DistanceAlongSpline = Segment->GetClampedDistance(DistanceAlongSpline + MoveAxis * MoveSpeed * DeltaTime);
		UpdateLookTargetFromCursor();
		UpdateSmoothedLook(DeltaTime);
		ApplyTunnelTransform();
	}
	else
	{
		DeactivateTunnelPrototype();
	}
}

bool UWacomRunTunnelPrototypeComponent::ActivateTunnelPrototype(
	AWacomRunTunnelSegmentActor* InitialSegment,
	float StartDistance)
{
	if (!SwitchToSegment(InitialSegment, StartDistance))
	{
		return false;
	}

	bTunnelPrototypeActive = true;
	bTunnelPrototypeSuspended = false;
	SetComponentTickEnabled(true);
	ApplyInputProfile();

	if (AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}

	ApplyTunnelTransform();
	return true;
}

void UWacomRunTunnelPrototypeComponent::DeactivateTunnelPrototype()
{
	if (!bTunnelPrototypeActive && !ActiveSegment.IsValid())
	{
		return;
	}

	bTunnelPrototypeActive = false;
	bTunnelPrototypeSuspended = false;
	ActiveSegment.Reset();
	DistanceAlongSpline = 0.0f;
	MoveAxis = 0.0f;
	LookYawOffset = 0.0f;
	LookPitchOffset = 0.0f;
	TargetLookYawOffset = 0.0f;
	TargetLookPitchOffset = 0.0f;
	SetComponentTickEnabled(false);
	ApplyInputProfile();

	if (AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
}

bool UWacomRunTunnelPrototypeComponent::SuspendTunnelPrototype()
{
	if (!bTunnelPrototypeActive || bTunnelPrototypeSuspended)
	{
		return false;
	}

	bTunnelPrototypeSuspended = true;
	MoveAxis = 0.0f;
	SetComponentTickEnabled(false);
	return true;
}

bool UWacomRunTunnelPrototypeComponent::ResumeTunnelPrototype()
{
	if (!bTunnelPrototypeActive || !bTunnelPrototypeSuspended || !ActiveSegment.IsValid())
	{
		return false;
	}

	bTunnelPrototypeSuspended = false;
	SetComponentTickEnabled(true);
	ApplyInputProfile();
	ApplyTunnelTransform();
	return true;
}

bool UWacomRunTunnelPrototypeComponent::SwitchToSegment(
	AWacomRunTunnelSegmentActor* TargetSegment,
	float StartDistance)
{
	if (!IsValid(TargetSegment) || !TargetSegment->GetPathSpline())
	{
		return false;
	}

	ActiveSegment = TargetSegment;
	DistanceAlongSpline = TargetSegment->GetClampedDistance(StartDistance);
	MoveAxis = 0.0f;
	LookYawOffset = 0.0f;
	LookPitchOffset = 0.0f;
	TargetLookYawOffset = 0.0f;
	TargetLookPitchOffset = 0.0f;

	if (bTunnelPrototypeActive && !bTunnelPrototypeSuspended)
	{
		ApplyTunnelTransform();
	}

	return true;
}

bool UWacomRunTunnelPrototypeComponent::HandleMoveInput(const FVector2D& Input)
{
	if (!bTunnelPrototypeActive || bTunnelPrototypeSuspended)
	{
		return false;
	}

	MoveAxis = FMath::Clamp(Input.Y, -1.0f, 1.0f);
	return true;
}

bool UWacomRunTunnelPrototypeComponent::HandleLookInput(const FVector2D& Input)
{
	if (!bTunnelPrototypeActive || bTunnelPrototypeSuspended)
	{
		return false;
	}

	UpdateLookTargetFromCursor();
	return true;
}

AWacomPlayerCharacter* UWacomRunTunnelPrototypeComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

APlayerController* UWacomRunTunnelPrototypeComponent::GetOwnerPlayerController() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

void UWacomRunTunnelPrototypeComponent::ApplyInputProfile()
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UWacomInputContextCoordinatorSubsystem* InputCoordinator =
			LP->GetSubsystem<UWacomInputContextCoordinatorSubsystem>())
		{
			InputCoordinator->InitializeForPlayerController(PC);
			InputCoordinator->SetExplorationProfile(
				(bTunnelPrototypeActive && !bTunnelPrototypeSuspended)
					? EWacomExplorationInputProfile::RunTunnel
					: EWacomExplorationInputProfile::FreeLook);
		}
	}
}

void UWacomRunTunnelPrototypeComponent::UpdateLookTargetFromCursor()
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		TargetLookYawOffset = 0.0f;
		TargetLookPitchOffset = 0.0f;
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (ViewportSizeX <= 0
		|| ViewportSizeY <= 0
		|| !PC->GetMousePosition(MouseX, MouseY))
	{
		TargetLookYawOffset = 0.0f;
		TargetLookPitchOffset = 0.0f;
		return;
	}

	const float NormalizedX = FMath::Clamp((MouseX / static_cast<float>(ViewportSizeX)) * 2.0f - 1.0f, -1.0f, 1.0f);
	const float NormalizedY = FMath::Clamp((MouseY / static_cast<float>(ViewportSizeY)) * 2.0f - 1.0f, -1.0f, 1.0f);
	TargetLookYawOffset = NormalizedX * FMath::Abs(YawClampDegrees) * LookYawScale;
	TargetLookPitchOffset = -NormalizedY * FMath::Abs(PitchClampDegrees) * LookPitchScale;
	TargetLookYawOffset = FMath::Clamp(TargetLookYawOffset, -FMath::Abs(YawClampDegrees), FMath::Abs(YawClampDegrees));
	TargetLookPitchOffset = FMath::Clamp(TargetLookPitchOffset, -FMath::Abs(PitchClampDegrees), FMath::Abs(PitchClampDegrees));
}

void UWacomRunTunnelPrototypeComponent::UpdateSmoothedLook(float DeltaTime)
{
	if (LookInterpSpeed <= 0.0f)
	{
		LookYawOffset = TargetLookYawOffset;
		LookPitchOffset = TargetLookPitchOffset;
		return;
	}

	LookYawOffset = FMath::FInterpTo(LookYawOffset, TargetLookYawOffset, DeltaTime, LookInterpSpeed);
	LookPitchOffset = FMath::FInterpTo(LookPitchOffset, TargetLookPitchOffset, DeltaTime, LookInterpSpeed);
}

void UWacomRunTunnelPrototypeComponent::ApplyTunnelTransform()
{
	AWacomRunTunnelSegmentActor* Segment = ActiveSegment.Get();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!Segment || !Character)
	{
		return;
	}

	const FTransform SplineTransform = Segment->GetSplineTransformAtDistance(DistanceAlongSpline);
	const FRotator SplineRotation = SplineTransform.Rotator();
	const FRotator ControlRotation(
		SplineRotation.Pitch + LookPitchOffset,
		SplineRotation.Yaw + LookYawOffset,
		0.0f);
	const FRotator ActorRotation(0.0f, SplineRotation.Yaw, 0.0f);

	FVector ActorLocation = SplineTransform.GetLocation();
	if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
	{
		ActorLocation -= ActorRotation.RotateVector(Camera->GetRelativeLocation());
	}

	Character->SetActorLocationAndRotation(ActorLocation, ActorRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(ControlRotation);
	}
}
