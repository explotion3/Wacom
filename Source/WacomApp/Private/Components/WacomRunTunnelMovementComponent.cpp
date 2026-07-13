// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunTunnelMovementComponent.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/CapsuleComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Engine/Engine.h"
#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "Engine/GameInstance.h"
#include "Settings/WacomSettingsSubsystem.h"

UWacomRunTunnelMovementComponent::UWacomRunTunnelMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWacomRunTunnelMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	BindRuntimeSettings();
	SetComponentTickEnabled(false);
}

void UWacomRunTunnelMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindRuntimeSettings();
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
		const float OldDistanceAlongSpline = DistanceAlongSpline;
		DistanceAlongSpline = Segment->GetClampedDistance(DistanceAlongSpline + MoveAxis * MoveSpeed * DeltaTime);
		const float ActualDistanceDeltaCm = FMath::Abs(DistanceAlongSpline - OldDistanceAlongSpline);
		UpdateCursorLook(DeltaTime);
		ApplyTunnelTransform(DeltaTime, ActualDistanceDeltaCm);
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
	StopWalkCameraShake(/*bImmediately*/true);
	ResetWalkBob();
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
	StopWalkCameraShake(/*bImmediately*/true);
	ResetWalkBob();
	ApplyTunnelTransform();
	SetComponentTickEnabled(false);
	return true;
}

bool UWacomRunTunnelMovementComponent::ResumeRunTunnel()
{
	return ResumeRunTunnel(/*bPreserveCursorLookOffset*/false);
}

bool UWacomRunTunnelMovementComponent::ResumeRunTunnel(bool bPreserveCursorLookOffset)
{
	if (!bRunTunnelActive || !bRunTunnelSuspended || !ActiveSegment.IsValid())
	{
		return false;
	}

	bRunTunnelSuspended = false;
	SetComponentTickEnabled(true);
	if (bPreserveCursorLookOffset)
	{
		UpdateCursorLook(/*DeltaTime*/0.0f);
	}
	else if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	ResetWalkBob();
	StopWalkCameraShake(/*bImmediately*/true);
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
	StopWalkCameraShake(/*bImmediately*/true);
	ResetWalkBob();
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

bool UWacomRunTunnelMovementComponent::TryGetCurrentTunnelViewTransform(
	FTransform& OutViewTransform) const
{
	const AWacomRunTunnelSegmentActor* Segment = ActiveSegment.Get();
	if (!bRunTunnelActive || !Segment)
	{
		return false;
	}

	OutViewTransform = Segment->GetSplineTransformAtDistance(DistanceAlongSpline);
	return true;
}

bool UWacomRunTunnelMovementComponent::TryBuildReturnToRunTunnelStageRequest(
	FWacomFirstPersonViewStageRequest& OutRequest) const
{
	OutRequest = FWacomFirstPersonViewStageRequest();

	FTransform TunnelViewTransform = FTransform::Identity;
	if (!TryGetCurrentTunnelViewTransform(TunnelViewTransform))
	{
		return false;
	}

	const AWacomRunTunnelSegmentActor* Segment = ActiveSegment.Get();
	OutRequest.bHasViewTransform = true;
	OutRequest.ViewTransform = TunnelViewTransform;
	OutRequest.Reason = FName(TEXT("RunTunnelReturn"));
	OutRequest.DebugSource = Segment ? FName(*Segment->GetName()) : NAME_None;
	OutRequest.BlendTimeSeconds = FMath::Max(0.0f, ReturnStageBlendTimeSeconds);
	OutRequest.BlendCurve = ReturnStageBlendCurve;
	OutRequest.BlendEasePower = FMath::Max(0.01f, ReturnStageBlendEasePower);
	return true;
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

UWacomFirstPersonWalkBobComponent* UWacomRunTunnelMovementComponent::GetWalkBob() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetWalkBobComponent() : nullptr;
}

void UWacomRunTunnelMovementComponent::ResetWalkBob(bool bSnapToZero)
{
	if (UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		WalkBob->ResetWalkBob(bSnapToZero);
	}
}

bool UWacomRunTunnelMovementComponent::ShouldUseWalkCameraShake() const
{
	return bUseWalkCameraShake
		&& WalkCameraShakeClass != nullptr
		&& RuntimeCameraMotionStrength > KINDA_SMALL_NUMBER;
}

bool UWacomRunTunnelMovementComponent::IsWalkCameraShakeMovementActive(float ActualDistanceDeltaCm) const
{
	float DeadZoneCm = 0.1f;
	if (const UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		DeadZoneCm = FMath::Max(0.0f, WalkBob->MovementDeadZoneCm);
	}
	return bRunTunnelActive
		&& !bRunTunnelSuspended
		&& ActualDistanceDeltaCm > DeadZoneCm;
}

void UWacomRunTunnelMovementComponent::UpdateWalkCameraShake(float DeltaTime, float ActualDistanceDeltaCm)
{
	if (!ShouldUseWalkCameraShake())
	{
		ShowWalkCameraShakeDebug(TEXT("DisabledOrNoClass"), ActualDistanceDeltaCm);
		StopWalkCameraShake(/*bImmediately*/false);
		return;
	}

	if (ActiveWalkCameraShakeInstance && ActiveWalkCameraShakeInstance->IsFinished())
	{
		ShowWalkCameraShakeDebug(TEXT("InstanceFinished"), ActualDistanceDeltaCm);
		ActiveWalkCameraShakeInstance = nullptr;
	}

	if (!IsWalkCameraShakeMovementActive(ActualDistanceDeltaCm))
	{
		if (WalkCameraShakeStopGraceRemainingSeconds > 0.0f)
		{
			WalkCameraShakeStopGraceRemainingSeconds = FMath::Max(
				0.0f,
				WalkCameraShakeStopGraceRemainingSeconds - FMath::Max(0.0f, DeltaTime));
			if (WalkCameraShakeStopGraceRemainingSeconds > 0.0f)
			{
				ShowWalkCameraShakeDebug(TEXT("StopGrace"), ActualDistanceDeltaCm);
				return;
			}
		}

		ShowWalkCameraShakeDebug(TEXT("NoMovement"), ActualDistanceDeltaCm);
		StopWalkCameraShake(/*bImmediately*/false);
		return;
	}

	WalkCameraShakeStopGraceRemainingSeconds = FMath::Max(0.0f, WalkCameraShakeStopGraceSeconds);

	if (ActiveWalkCameraShakeInstance)
	{
		ShowWalkCameraShakeDebug(TEXT("Active"), ActualDistanceDeltaCm, ActiveWalkCameraShakeInstance);
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		ShowWalkCameraShakeDebug(TEXT("NoPlayerCameraManager"), ActualDistanceDeltaCm);
		return;
	}

	ActiveWalkCameraShakeInstance = PC->PlayerCameraManager->StartCameraShake(
		WalkCameraShakeClass,
		FMath::Max(0.0f, WalkCameraShakeScale) * RuntimeCameraMotionStrength);
	ShowWalkCameraShakeDebug(
		ActiveWalkCameraShakeInstance ? TEXT("StartOK") : TEXT("StartReturnedNull"),
		ActualDistanceDeltaCm,
		ActiveWalkCameraShakeInstance);
	if (bDebugWalkCameraShake)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[WacomRunTunnel][WalkCameraShake] Start Status=%s Class=%s Scale=%.3f Delta=%.3f Instance=%s PC=%s PCM=%s"),
			ActiveWalkCameraShakeInstance ? TEXT("OK") : TEXT("Null"),
			*GetNameSafe(WalkCameraShakeClass.Get()),
			FMath::Max(0.0f, WalkCameraShakeScale) * RuntimeCameraMotionStrength,
			ActualDistanceDeltaCm,
			*GetNameSafe(ActiveWalkCameraShakeInstance),
			*GetNameSafe(PC),
			*GetNameSafe(PC->PlayerCameraManager));
	}
}

void UWacomRunTunnelMovementComponent::StopWalkCameraShake(bool bImmediately)
{
	WalkCameraShakeStopGraceRemainingSeconds = 0.0f;

	if (!ActiveWalkCameraShakeInstance)
	{
		return;
	}

	if (APlayerController* PC = GetOwnerPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StopCameraShake(ActiveWalkCameraShakeInstance, bImmediately);
		}
		else
		{
			ActiveWalkCameraShakeInstance->StopShake(bImmediately);
		}
	}
	else
	{
		ActiveWalkCameraShakeInstance->StopShake(bImmediately);
	}

	if (bDebugWalkCameraShake)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[WacomRunTunnel][WalkCameraShake] Stop Class=%s Instance=%s Immediate=%s"),
			*GetNameSafe(WalkCameraShakeClass.Get()),
			*GetNameSafe(ActiveWalkCameraShakeInstance),
			bImmediately ? TEXT("true") : TEXT("false"));
	}
	ActiveWalkCameraShakeInstance = nullptr;
}

void UWacomRunTunnelMovementComponent::ShowWalkCameraShakeDebug(
	const TCHAR* Status,
	float ActualDistanceDeltaCm,
	const UCameraShakeBase* StartedInstance) const
{
	if (!bDebugWalkCameraShake || !GEngine)
	{
		return;
	}

	const APlayerController* PC = GetOwnerPlayerController();
	const APlayerCameraManager* PCM = PC ? PC->PlayerCameraManager : nullptr;
	float DeadZoneCm = 0.1f;
	if (const UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		DeadZoneCm = FMath::Max(0.0f, WalkBob->MovementDeadZoneCm);
	}
	const FString Message = FString::Printf(
		TEXT("WalkCameraShake %s | use=%s class=%s scale=%.2f delta=%.3f dead=%.3f grace=%.3f moveAxis=%.2f active=%s suspended=%s inst=%s PC=%s PCM=%s"),
		Status ? Status : TEXT("Unknown"),
		bUseWalkCameraShake ? TEXT("true") : TEXT("false"),
		*GetNameSafe(WalkCameraShakeClass.Get()),
		FMath::Max(0.0f, WalkCameraShakeScale),
		ActualDistanceDeltaCm,
		DeadZoneCm,
		WalkCameraShakeStopGraceRemainingSeconds,
		MoveAxis,
		bRunTunnelActive ? TEXT("true") : TEXT("false"),
		bRunTunnelSuspended ? TEXT("true") : TEXT("false"),
		*GetNameSafe(StartedInstance ? StartedInstance : ActiveWalkCameraShakeInstance.Get()),
		*GetNameSafe(PC),
		*GetNameSafe(PCM));
	const uint64 DebugKey = 740000ull + static_cast<uint64>(GetUniqueID() % 10000u);
	GEngine->AddOnScreenDebugMessage(DebugKey, 0.15f, FColor::Cyan, Message);
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
		const float Scale = FMath::Max(0.0f, CursorLookOverrideScale)
			* RuntimeLookResponseStrength;
		const float PitchDirection = bRuntimeInvertLookY ? -1.0f : 1.0f;
		const float InterpSpeed = CursorLookOverrideInterpSpeed >= 0.0f
			? CursorLookOverrideInterpSpeed
			: LookInterpSpeed;
		Driver->UpdateFromNormalizedCursor(
			CursorLookOverrideNormalized,
			DeltaTime,
			YawClampDegrees,
			PitchClampDegrees,
			LookYawScale * Scale,
			LookPitchScale * Scale * PitchDirection,
			InterpSpeed);
		return;
	}

	Driver->UpdateFromPlayerCursor(
		PC,
		DeltaTime,
		YawClampDegrees,
		PitchClampDegrees,
		LookYawScale * RuntimeLookResponseStrength,
		LookPitchScale * RuntimeLookResponseStrength * (bRuntimeInvertLookY ? -1.0f : 1.0f),
		LookInterpSpeed);
}

void UWacomRunTunnelMovementComponent::BindRuntimeSettings()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!SettingsSubsystem)
	{
		return;
	}

	RuntimeSettingsChangedHandle = SettingsSubsystem->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomRunTunnelMovementComponent::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		SettingsSubsystem->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomRunTunnelMovementComponent::UnbindRuntimeSettings()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr)
	{
		SettingsSubsystem->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomRunTunnelMovementComponent::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
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
		// Camera shake scale is captured when the instance starts. Restart it on the
		// next movement update so a changed accessibility value takes effect atomically.
		StopWalkCameraShake(/*bImmediately*/true);
		if (RuntimeCameraMotionStrength <= KINDA_SMALL_NUMBER)
		{
			ResetWalkBob(/*bSnapToZero*/true);
		}
	}
}

void UWacomRunTunnelMovementComponent::ApplyTunnelTransform(
	float DeltaTime,
	float ActualDistanceDeltaCm)
{
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	FTransform SplineTransform = FTransform::Identity;
	if (!Character || !TryGetCurrentTunnelViewTransform(SplineTransform))
	{
		return;
	}

	const FRotator SplineRotation = SplineTransform.Rotator();
	const FRotator LookOffset = GetCursorLookDriver()
		? GetCursorLookDriver()->GetCurrentLookOffset()
		: FRotator::ZeroRotator;
	FRotator ControlRotation(
		SplineRotation.Pitch + LookOffset.Pitch,
		SplineRotation.Yaw + LookOffset.Yaw,
		0.0f);
	const FRotator ActorRotation(0.0f, SplineRotation.Yaw, 0.0f);

	FVector ViewLocation = SplineTransform.GetLocation();
	const float BobDistanceDeltaCm = bRunTunnelActive && !bRunTunnelSuspended
		? ActualDistanceDeltaCm
		: 0.0f;
	if (ShouldUseWalkCameraShake())
	{
		ResetWalkBob();
		UpdateWalkCameraShake(DeltaTime, BobDistanceDeltaCm);
	}
	else if (UWacomFirstPersonWalkBobComponent* WalkBob = GetWalkBob())
	{
		StopWalkCameraShake(/*bImmediately*/false);
		WalkBob->UpdateWalkBobFromMovementDelta(
			DeltaTime,
			BobDistanceDeltaCm,
			FMath::Max(MoveSpeed, KINDA_SMALL_NUMBER));

		const FRotator WalkBobRotation = WalkBob->GetCurrentRotationOffset();
		ViewLocation += ControlRotation.RotateVector(WalkBob->GetCurrentLocationOffset());
		ControlRotation.Pitch += WalkBobRotation.Pitch;
		ControlRotation.Roll += WalkBobRotation.Roll;
	}

	FVector ActorLocation = ViewLocation;
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
