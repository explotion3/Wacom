// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunPathTraversalComponent.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "Settings/WacomSettingsSubsystem.h"

UWacomRunPathTraversalComponent::UWacomRunPathTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWacomRunPathTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
	BindRuntimeSettings();
	SetComponentTickEnabled(false);
}

void UWacomRunPathTraversalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindRuntimeSettings();
	DeactivateTraversal();
	Super::EndPlay(EndPlayReason);
}

void UWacomRunPathTraversalComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (State != EWacomRunPathTraversalState::Traversing)
	{
		return;
	}

	AWacomRunPathSegmentActor* Path = ActivePath.Get();
	if (!Path || !Path->GetPathSpline())
	{
		DeactivateTraversal();
		return;
	}

	const float PreviousDistance = DistanceAlongSpline;
	DistanceAlongSpline = Path->GetClampedDistance(
		DistanceAlongSpline + MoveAxis * MoveSpeed * FMath::Max(0.0f, DeltaTime));
	const float ActualDistanceDeltaCm = FMath::Abs(DistanceAlongSpline - PreviousDistance);
	UpdateCursorLook(DeltaTime);
	ApplyViewTransform(DeltaTime, ActualDistanceDeltaCm);
	UpdateBoundaryLatches();
}

bool UWacomRunPathTraversalComponent::AnchorAtTransform(const FTransform& AnchorTransform)
{
	if (AnchorTransform.ContainsNaN())
	{
		return false;
	}
	ActivePath.Reset();
	AnchoredViewTransform = AnchorTransform;
	DistanceAlongSpline = 0.0f;
	MoveAxis = 0.0f;
	State = EWacomRunPathTraversalState::Anchored;
	StateBeforeSuspend = EWacomRunPathTraversalState::Inactive;
	bStartBoundaryBroadcast = false;
	bEndBoundaryBroadcast = false;
	bLeftStartBoundary = false;
	ResetMotionFeedback();
	SetComponentTickEnabled(false);
	ApplyInputProfile();
	TakeCharacterMovementOwnership();
	ApplyViewTransform(0.0f, 0.0f);
	return true;
}

bool UWacomRunPathTraversalComponent::BeginTraversal(AWacomRunPathSegmentActor* Segment)
{
	if (!IsValid(Segment) || !Segment->GetPathSpline() || Segment->GetSplineLength() <= UE_SMALL_NUMBER)
	{
		return false;
	}
	ActivePath = Segment;
	DistanceAlongSpline = 0.0f;
	MoveAxis = 0.0f;
	State = EWacomRunPathTraversalState::Traversing;
	StateBeforeSuspend = EWacomRunPathTraversalState::Inactive;
	bStartBoundaryBroadcast = false;
	bEndBoundaryBroadcast = false;
	bLeftStartBoundary = false;
	ResetMotionFeedback();
	SetComponentTickEnabled(true);
	ApplyInputProfile();
	TakeCharacterMovementOwnership();
	ApplyViewTransform(0.0f, 0.0f);
	return true;
}

void UWacomRunPathTraversalComponent::DeactivateTraversal()
{
	State = EWacomRunPathTraversalState::Inactive;
	StateBeforeSuspend = EWacomRunPathTraversalState::Inactive;
	ActivePath.Reset();
	DistanceAlongSpline = 0.0f;
	MoveAxis = 0.0f;
	ClearCursorLookOverride();
	ResetMotionFeedback();
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	SetComponentTickEnabled(false);
	ApplyInputProfile();
}

bool UWacomRunPathTraversalComponent::SuspendTraversal()
{
	if (State != EWacomRunPathTraversalState::Anchored
		&& State != EWacomRunPathTraversalState::Traversing)
	{
		return false;
	}
	StateBeforeSuspend = State;
	State = EWacomRunPathTraversalState::Suspended;
	MoveAxis = 0.0f;
	ClearCursorLookOverride();
	ResetMotionFeedback();
	SetComponentTickEnabled(false);
	return true;
}

bool UWacomRunPathTraversalComponent::ResumeTraversal(const bool bPreserveCursorLookOffset)
{
	if (State != EWacomRunPathTraversalState::Suspended
		|| StateBeforeSuspend == EWacomRunPathTraversalState::Inactive)
	{
		return false;
	}
	State = StateBeforeSuspend;
	StateBeforeSuspend = EWacomRunPathTraversalState::Inactive;
	if (!bPreserveCursorLookOffset)
	{
		if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
		{
			Driver->ResetLookOffset();
		}
	}
	SetComponentTickEnabled(State == EWacomRunPathTraversalState::Traversing);
	ApplyInputProfile();
	ApplyViewTransform(0.0f, 0.0f);
	return true;
}

bool UWacomRunPathTraversalComponent::HandleMoveInput(const FVector2D& Input)
{
	if (State != EWacomRunPathTraversalState::Traversing)
	{
		return false;
	}
	MoveAxis = FMath::Clamp(Input.Y, -1.0f, 1.0f);
	return true;
}

bool UWacomRunPathTraversalComponent::HandleLookInput(const FVector2D& Input)
{
	if (State != EWacomRunPathTraversalState::Anchored
		&& State != EWacomRunPathTraversalState::Traversing)
	{
		return false;
	}
	UpdateCursorLook(0.0f);
	ApplyViewTransform(0.0f, 0.0f);
	return true;
}

void UWacomRunPathTraversalComponent::SetCursorLookOverrideNormalized(
	FVector2D NormalizedCursor,
	const float Scale,
	const float InterpSpeedOverride)
{
	CursorLookOverrideNormalized.X = FMath::Clamp(NormalizedCursor.X, -1.0f, 1.0f);
	CursorLookOverrideNormalized.Y = FMath::Clamp(NormalizedCursor.Y, -1.0f, 1.0f);
	CursorLookOverrideScale = FMath::Max(0.0f, Scale);
	CursorLookOverrideInterpSpeed = InterpSpeedOverride;
	bHasCursorLookOverride = true;
}

void UWacomRunPathTraversalComponent::ClearCursorLookOverride()
{
	bHasCursorLookOverride = false;
	CursorLookOverrideNormalized = FVector2D::ZeroVector;
	CursorLookOverrideScale = 1.0f;
	CursorLookOverrideInterpSpeed = -1.0f;
}

bool UWacomRunPathTraversalComponent::TryGetCurrentViewTransform(FTransform& OutViewTransform) const
{
	if (State == EWacomRunPathTraversalState::Inactive)
	{
		return false;
	}
	if (State == EWacomRunPathTraversalState::Traversing
		|| (State == EWacomRunPathTraversalState::Suspended && ActivePath.IsValid()))
	{
		if (const AWacomRunPathSegmentActor* Path = ActivePath.Get())
		{
			OutViewTransform = Path->GetSplineTransformAtDistance(DistanceAlongSpline);
			return true;
		}
		return false;
	}
	OutViewTransform = AnchoredViewTransform;
	return true;
}

bool UWacomRunPathTraversalComponent::TryBuildReturnToRunPathStageRequest(
	FWacomFirstPersonViewStageRequest& OutRequest) const
{
	OutRequest = {};
	if (!TryGetCurrentViewTransform(OutRequest.ViewTransform))
	{
		return false;
	}
	OutRequest.bHasViewTransform = true;
	OutRequest.Reason = TEXT("RunPathReturn");
	OutRequest.DebugSource = ActivePath.IsValid()
		? FName(*ActivePath->GetName())
		: FName(TEXT("RunNodeAnchor"));
	OutRequest.BlendTimeSeconds = FMath::Max(0.0f, ReturnStageBlendTimeSeconds);
	OutRequest.BlendCurve = ReturnStageBlendCurve;
	OutRequest.BlendEasePower = FMath::Max(0.01f, ReturnStageBlendEasePower);
	return true;
}

void UWacomRunPathTraversalComponent::UpdateBoundaryLatches()
{
	const AWacomRunPathSegmentActor* Path = ActivePath.Get();
	if (!Path)
	{
		return;
	}
	const float Hysteresis = FMath::Max(0.0f, BoundaryHysteresisDistance);
	if (DistanceAlongSpline > Hysteresis)
	{
		bLeftStartBoundary = true;
	}
	if (bLeftStartBoundary && !bStartBoundaryBroadcast && DistanceAlongSpline <= UE_SMALL_NUMBER)
	{
		bStartBoundaryBroadcast = true;
		ReachedStartNative.Broadcast();
	}
	if (!bEndBoundaryBroadcast
		&& DistanceAlongSpline >= Path->GetSplineLength() - UE_SMALL_NUMBER)
	{
		bEndBoundaryBroadcast = true;
		ReachedEndNative.Broadcast();
	}
}

AWacomPlayerCharacter* UWacomRunPathTraversalComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

APlayerController* UWacomRunPathTraversalComponent::GetOwnerPlayerController() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

UWacomCursorLookDriverComponent* UWacomRunPathTraversalComponent::GetCursorLookDriver() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetCursorLookDriverComponent() : nullptr;
}

UWacomFirstPersonWalkBobComponent* UWacomRunPathTraversalComponent::GetWalkBob() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetWalkBobComponent() : nullptr;
}

void UWacomRunPathTraversalComponent::ApplyInputProfile()
{
	APlayerController* PC = GetOwnerPlayerController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (UWacomInputContextCoordinatorSubsystem* Coordinator = LocalPlayer
		? LocalPlayer->GetSubsystem<UWacomInputContextCoordinatorSubsystem>()
		: nullptr)
	{
		Coordinator->InitializeForPlayerController(PC);
		Coordinator->ApplyCurrentInputContext();
	}
}

void UWacomRunPathTraversalComponent::TakeCharacterMovementOwnership()
{
	if (AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
}

void UWacomRunPathTraversalComponent::UpdateCursorLook(const float DeltaTime)
{
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	if (!Driver)
	{
		return;
	}
	const float PitchDirection = bRuntimeInvertLookY ? -1.0f : 1.0f;
	if (bHasCursorLookOverride)
	{
		Driver->UpdateFromNormalizedCursor(
			CursorLookOverrideNormalized,
			DeltaTime,
			YawClampDegrees,
			PitchClampDegrees,
			LookYawScale * CursorLookOverrideScale * RuntimeLookResponseStrength,
			LookPitchScale * CursorLookOverrideScale * RuntimeLookResponseStrength * PitchDirection,
			CursorLookOverrideInterpSpeed >= 0.0f
				? CursorLookOverrideInterpSpeed
				: LookInterpSpeed);
		return;
	}
	Driver->UpdateFromPlayerCursor(
		GetOwnerPlayerController(),
		DeltaTime,
		YawClampDegrees,
		PitchClampDegrees,
		LookYawScale * RuntimeLookResponseStrength,
		LookPitchScale * RuntimeLookResponseStrength * PitchDirection,
		LookInterpSpeed);
}

void UWacomRunPathTraversalComponent::ApplyViewTransform(
	const float DeltaTime,
	const float ActualDistanceDeltaCm)
{
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	FTransform ViewTransform;
	if (!Character || !TryGetCurrentViewTransform(ViewTransform))
	{
		return;
	}
	FRotator ControlRotation = ViewTransform.Rotator();
	if (const UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		const FRotator Offset = Driver->GetCurrentLookOffset();
		ControlRotation.Pitch += Offset.Pitch;
		ControlRotation.Yaw += Offset.Yaw;
	}
	ControlRotation.Roll = 0.0f;
	const FRotator ActorRotation(0.0f, ViewTransform.Rotator().Yaw, 0.0f);
	FVector ViewLocation = ViewTransform.GetLocation();
	if (bUseWalkCameraShake && WalkCameraShakeClass && RuntimeCameraMotionStrength > KINDA_SMALL_NUMBER)
	{
		if (UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
		{
			WalkBob->ResetWalkBob(true);
		}
		UpdateWalkCameraShake(DeltaTime, ActualDistanceDeltaCm);
	}
	else if (UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		StopWalkCameraShake(false);
		WalkBob->UpdateWalkBobFromMovementDelta(
			DeltaTime,
			State == EWacomRunPathTraversalState::Traversing ? ActualDistanceDeltaCm : 0.0f,
			FMath::Max(MoveSpeed, KINDA_SMALL_NUMBER));
		ViewLocation += ControlRotation.RotateVector(WalkBob->GetCurrentLocationOffset());
		const FRotator BobRotation = WalkBob->GetCurrentRotationOffset();
		ControlRotation.Pitch += BobRotation.Pitch;
		ControlRotation.Roll += BobRotation.Roll;
	}

	FVector ActorLocation = ViewLocation;
	if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
	{
		ActorLocation -= ActorRotation.RotateVector(Camera->GetRelativeLocation());
	}
	Character->SetActorLocationAndRotation(
		ActorLocation,
		ActorRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(ControlRotation);
	}
}

void UWacomRunPathTraversalComponent::ResetMotionFeedback()
{
	StopWalkCameraShake(true);
	if (UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		WalkBob->ResetWalkBob(true);
	}
}

void UWacomRunPathTraversalComponent::UpdateWalkCameraShake(
	const float DeltaTime,
	const float ActualDistanceDeltaCm)
{
	const bool bMoving = State == EWacomRunPathTraversalState::Traversing
		&& ActualDistanceDeltaCm > 0.1f;
	if (!bMoving)
	{
		WalkCameraShakeStopGraceRemainingSeconds = FMath::Max(
			0.0f,
			WalkCameraShakeStopGraceRemainingSeconds - FMath::Max(0.0f, DeltaTime));
		if (WalkCameraShakeStopGraceRemainingSeconds <= 0.0f)
		{
			StopWalkCameraShake(false);
		}
		return;
	}
	WalkCameraShakeStopGraceRemainingSeconds = FMath::Max(0.0f, WalkCameraShakeStopGraceSeconds);
	if (ActiveWalkCameraShakeInstance && !ActiveWalkCameraShakeInstance->IsFinished())
	{
		return;
	}
	APlayerController* PC = GetOwnerPlayerController();
	ActiveWalkCameraShakeInstance = PC && PC->PlayerCameraManager
		? PC->PlayerCameraManager->StartCameraShake(
			WalkCameraShakeClass,
			FMath::Max(0.0f, WalkCameraShakeScale) * RuntimeCameraMotionStrength)
		: nullptr;
}

void UWacomRunPathTraversalComponent::StopWalkCameraShake(const bool bImmediately)
{
	WalkCameraShakeStopGraceRemainingSeconds = 0.0f;
	if (!ActiveWalkCameraShakeInstance)
	{
		return;
	}
	if (APlayerController* PC = GetOwnerPlayerController(); PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StopCameraShake(ActiveWalkCameraShakeInstance, bImmediately);
	}
	else
	{
		ActiveWalkCameraShakeInstance->StopShake(bImmediately);
	}
	ActiveWalkCameraShakeInstance = nullptr;
}

void UWacomRunPathTraversalComponent::BindRuntimeSettings()
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UWacomSettingsSubsystem* Settings = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!Settings)
	{
		return;
	}
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomRunPathTraversalComponent::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		Settings->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomRunPathTraversalComponent::UnbindRuntimeSettings()
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UWacomSettingsSubsystem* Settings = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr)
	{
		Settings->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomRunPathTraversalComponent::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason Reason)
{
	const float PreviousMotionStrength = RuntimeCameraMotionStrength;
	RuntimeLookResponseStrength = FMath::Clamp(Snapshot.LookResponseStrength, 0.0f, 3.0f);
	bRuntimeInvertLookY = Snapshot.bInvertLookY;
	RuntimeCameraMotionStrength = FMath::Clamp(Snapshot.CameraMotionStrength, 0.0f, 1.0f);
	if (UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		WalkBob->SetRuntimeMotionStrength(RuntimeCameraMotionStrength);
	}
	if (!FMath::IsNearlyEqual(PreviousMotionStrength, RuntimeCameraMotionStrength))
	{
		StopWalkCameraShake(true);
	}
}
