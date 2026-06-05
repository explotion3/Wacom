// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunTunnelMovementComponent.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"

UWacomRunTunnelMovementComponent::UWacomRunTunnelMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWacomRunTunnelMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

void UWacomRunTunnelMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateRunTunnel();
	Super::EndPlay(EndPlayReason);
}

void UWacomRunTunnelMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bRunTunnelActive)
	{
		return;
	}
	if (bRunTunnelSuspended)
	{
		return;
	}

	if (AWacomRunTunnelSegmentActor* Segment = ActiveSegment.Get())
	{
		DistanceAlongSpline = Segment->GetClampedDistance(DistanceAlongSpline + MoveAxis * MoveSpeed * DeltaTime);
		UpdateCursorLook(DeltaTime);
		ApplyTunnelTransform();
	}
	else
	{
		DeactivateRunTunnel();
	}
}

bool UWacomRunTunnelMovementComponent::ActivateRunTunnel(
	AWacomRunTunnelSegmentActor* InitialSegment,
	float StartDistance)
{
	if (!SwitchToSegment(InitialSegment, StartDistance))
	{
		return false;
	}

	bRunTunnelActive = true;
	bRunTunnelSuspended = false;
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

void UWacomRunTunnelMovementComponent::DeactivateRunTunnel()
{
	if (!bRunTunnelActive && !ActiveSegment.IsValid())
	{
		return;
	}

	bRunTunnelActive = false;
	bRunTunnelSuspended = false;
	ClearCursorLookOverride();
	ActiveSegment.Reset();
	DistanceAlongSpline = 0.0f;
	MoveAxis = 0.0f;
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	SetComponentTickEnabled(false);
	ApplyInputProfile();
}

bool UWacomRunTunnelMovementComponent::SuspendRunTunnel()
{
	if (!bRunTunnelActive || bRunTunnelSuspended)
	{
		return false;
	}

	bRunTunnelSuspended = true;
	MoveAxis = 0.0f;
	ClearCursorLookOverride();
	SetComponentTickEnabled(false);
	return true;
}

bool UWacomRunTunnelMovementComponent::ResumeRunTunnel()
{
	if (!bRunTunnelActive || !bRunTunnelSuspended || !ActiveSegment.IsValid())
	{
		return false;
	}

	bRunTunnelSuspended = false;
	SetComponentTickEnabled(true);
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	ApplyInputProfile();
	ApplyTunnelTransform();
	return true;
}

bool UWacomRunTunnelMovementComponent::SwitchToSegment(
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
	ClearCursorLookOverride();
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}

	if (bRunTunnelActive && !bRunTunnelSuspended)
	{
		ApplyTunnelTransform();
	}

	return true;
}

bool UWacomRunTunnelMovementComponent::HandleMoveInput(const FVector2D& Input)
{
	if (!bRunTunnelActive || bRunTunnelSuspended)
	{
		return false;
	}

	MoveAxis = FMath::Clamp(Input.Y, -1.0f, 1.0f);
	return true;
}

bool UWacomRunTunnelMovementComponent::HandleLookInput(const FVector2D& Input)
{
	if (!bRunTunnelActive || bRunTunnelSuspended)
	{
		return false;
	}

	UpdateCursorLook(0.0f);
	return true;
}

void UWacomRunTunnelMovementComponent::SetCursorLookOverrideNormalized(
	FVector2D NormalizedCursor,
	float Scale,
	float InterpSpeedOverride)
{
	CursorLookOverrideNormalized = FVector2D(
		FMath::Clamp(NormalizedCursor.X, -1.0f, 1.0f),
		FMath::Clamp(NormalizedCursor.Y, -1.0f, 1.0f));
	CursorLookOverrideScale = FMath::Max(0.0f, Scale);
	CursorLookOverrideInterpSpeed = InterpSpeedOverride;
	bHasCursorLookOverride = true;
}

void UWacomRunTunnelMovementComponent::ClearCursorLookOverride()
{
	bHasCursorLookOverride = false;
	CursorLookOverrideNormalized = FVector2D::ZeroVector;
	CursorLookOverrideScale = 1.0f;
	CursorLookOverrideInterpSpeed = -1.0f;
}

AWacomPlayerCharacter* UWacomRunTunnelMovementComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

APlayerController* UWacomRunTunnelMovementComponent::GetOwnerPlayerController() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

UWacomCursorLookDriverComponent* UWacomRunTunnelMovementComponent::GetCursorLookDriver() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetCursorLookDriverComponent() : nullptr;
}

void UWacomRunTunnelMovementComponent::ApplyInputProfile()
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
			InputCoordinator->ApplyCurrentInputContext();
		}
	}
}

void UWacomRunTunnelMovementComponent::UpdateCursorLook(float DeltaTime)
{
	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	if (!Driver)
	{
		return;
	}

	if (bHasCursorLookOverride)
	{
		const float Scale = FMath::Max(0.0f, CursorLookOverrideScale);
		const float InterpSpeed = CursorLookOverrideInterpSpeed >= 0.0f
			? CursorLookOverrideInterpSpeed
			: LookInterpSpeed;
		Driver->UpdateFromNormalizedCursor(
			CursorLookOverrideNormalized,
			DeltaTime,
			YawClampDegrees,
			PitchClampDegrees,
			LookYawScale * Scale,
			LookPitchScale * Scale,
			InterpSpeed);
		return;
	}

	Driver->UpdateFromPlayerCursor(
		PC,
		DeltaTime,
		YawClampDegrees,
		PitchClampDegrees,
		LookYawScale,
		LookPitchScale,
		LookInterpSpeed);
}

void UWacomRunTunnelMovementComponent::ApplyTunnelTransform()
{
	AWacomRunTunnelSegmentActor* Segment = ActiveSegment.Get();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!Segment || !Character)
	{
		return;
	}

	const FTransform SplineTransform = Segment->GetSplineTransformAtDistance(DistanceAlongSpline);
	const FRotator SplineRotation = SplineTransform.Rotator();
	const FRotator LookOffset = GetCursorLookDriver()
		? GetCursorLookDriver()->GetCurrentLookOffset()
		: FRotator::ZeroRotator;
	const FRotator ControlRotation(
		SplineRotation.Pitch + LookOffset.Pitch,
		SplineRotation.Yaw + LookOffset.Yaw,
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
