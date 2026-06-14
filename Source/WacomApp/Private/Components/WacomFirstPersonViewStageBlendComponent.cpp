// Copyright Wacom. All Rights Reserved.

#include "Components/WacomFirstPersonViewStageBlendComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

namespace
{
	float SmoothStepStageBlendAlpha(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float EvaluatePoweredEaseInOut(float Alpha, float EasePower)
	{
		if (Alpha <= 0.0f)
		{
			return 0.0f;
		}
		if (Alpha >= 1.0f)
		{
			return 1.0f;
		}
		if (Alpha < 0.5f)
		{
			return 0.5f * FMath::Pow(Alpha * 2.0f, EasePower);
		}
		return 1.0f - 0.5f * FMath::Pow((1.0f - Alpha) * 2.0f, EasePower);
	}

	float EvaluateStageBlendAlpha(
		const FWacomFirstPersonViewStageRequest& Request,
		float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float EasePower = FMath::Max(0.01f, Request.BlendEasePower);
		switch (Request.BlendCurve)
		{
		case EWacomFirstPersonViewStageBlendCurve::Linear:
			return ClampedAlpha;
		case EWacomFirstPersonViewStageBlendCurve::EaseIn:
			return FMath::Pow(ClampedAlpha, EasePower);
		case EWacomFirstPersonViewStageBlendCurve::EaseOut:
			return 1.0f - FMath::Pow(1.0f - ClampedAlpha, EasePower);
		case EWacomFirstPersonViewStageBlendCurve::EaseInOut:
			return EvaluatePoweredEaseInOut(ClampedAlpha, EasePower);
		case EWacomFirstPersonViewStageBlendCurve::SmoothStep:
		default:
			return SmoothStepStageBlendAlpha(ClampedAlpha);
		}
	}
}

UWacomFirstPersonViewStageBlendComponent::UWacomFirstPersonViewStageBlendComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWacomFirstPersonViewStageBlendComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

void UWacomFirstPersonViewStageBlendComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	CancelActiveBlend();
	Super::EndPlay(EndPlayReason);
}

void UWacomFirstPersonViewStageBlendComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bStageBlendActive)
	{
		return;
	}

	APlayerController* PC = ActivePlayerController.Get();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!PC || !Character || !ActiveRequest.bHasViewTransform)
	{
		CancelActiveBlend();
		return;
	}

	BlendElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float RawAlpha = GetStageBlendAlpha();
	const float SmoothedAlpha = EvaluateStageBlendAlpha(ActiveRequest, RawAlpha);

	const FVector ViewLocation = FMath::Lerp(
		StartViewTransform.GetLocation(),
		ActiveRequest.ViewTransform.GetLocation(),
		SmoothedAlpha);
	const FQuat ViewRotation = FQuat::Slerp(
		StartViewTransform.GetRotation(),
		ActiveRequest.ViewTransform.GetRotation(),
		SmoothedAlpha).GetNormalized();
	const FTransform InterpolatedViewTransform(
		ViewRotation,
		ViewLocation,
		FVector::OneVector);
	CurrentBaseViewTransform = InterpolatedViewTransform;
	bHasCurrentBaseViewTransform = true;
	const FRotator StageLookOffset = UpdateStageLookOffset(*PC, DeltaTime);
	FVector ActorLocation = FVector::ZeroVector;
	FRotator ActorRotation = FRotator::ZeroRotator;
	FRotator BaseControlRotation = FRotator::ZeroRotator;
	WacomFirstPersonViewpointPlacement::CalculateActorTransformForView(
		*Character,
		InterpolatedViewTransform,
		ActorLocation,
		ActorRotation,
		BaseControlRotation);

	Character->SetActorLocationAndRotation(
		ActorLocation,
		ActorRotation,
		/*bSweep*/false,
		nullptr,
		ETeleportType::TeleportPhysics);
	PC->SetControlRotation(FRotator(
		BaseControlRotation.Pitch + StageLookOffset.Pitch,
		BaseControlRotation.Yaw + StageLookOffset.Yaw,
		0.0f));

	if (RawAlpha >= 1.0f)
	{
		FinishActiveBlend();
	}
}

bool UWacomFirstPersonViewStageBlendComponent::StartBlendToStageRequest(
	APlayerController& PlayerController,
	const FWacomFirstPersonViewStageRequest& Request,
	TFunction<void()>&& OnFinished)
{
	if (!Request.bHasViewTransform || Request.BlendTimeSeconds <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	const UCameraComponent* Camera = Character ? Character->GetFirstPersonCamera() : nullptr;
	if (!Character || !Camera)
	{
		return false;
	}

	CancelActiveBlend();

	ActivePlayerController = &PlayerController;
	ActiveRequest = Request;
	StartViewTransform = Camera->GetComponentTransform();
	CurrentBaseViewTransform = StartViewTransform;
	bHasCurrentBaseViewTransform = true;
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	OnBlendFinished = MoveTemp(OnFinished);
	BlendDurationSeconds = FMath::Max(Request.BlendTimeSeconds, KINDA_SMALL_NUMBER);
	BlendElapsedSeconds = 0.0f;
	bStageBlendActive = true;
	SetComponentTickEnabled(true);
	return true;
}

bool UWacomFirstPersonViewStageBlendComponent::TryGetCurrentBaseViewTransform(
	FTransform& OutViewTransform) const
{
	if (!bStageBlendActive || !bHasCurrentBaseViewTransform)
	{
		return false;
	}

	OutViewTransform = CurrentBaseViewTransform;
	return true;
}

void UWacomFirstPersonViewStageBlendComponent::CancelActiveBlend()
{
	if (!bStageBlendActive && !OnBlendFinished)
	{
		return;
	}

	ResetActiveBlendState();
}

float UWacomFirstPersonViewStageBlendComponent::GetStageBlendAlpha() const
{
	if (BlendDurationSeconds <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	return FMath::Clamp(BlendElapsedSeconds / BlendDurationSeconds, 0.0f, 1.0f);
}

AWacomPlayerCharacter* UWacomFirstPersonViewStageBlendComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

UWacomCursorLookDriverComponent* UWacomFirstPersonViewStageBlendComponent::GetCursorLookDriver() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetCursorLookDriverComponent() : nullptr;
}

FRotator UWacomFirstPersonViewStageBlendComponent::UpdateStageLookOffset(
	APlayerController& PlayerController,
	float DeltaTime)
{
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	const UWacomBattleCameraLookComponent* BattleCamera =
		Character ? Character->GetBattleCameraLookComponent() : nullptr;
	if (!Driver || !BattleCamera)
	{
		return FRotator::ZeroRotator;
	}

	const float AbsYawClamp = FMath::Max(0.0f, BattleCamera->YawClampDegrees);
	const float AbsPitchClamp = FMath::Max(0.0f, BattleCamera->PitchClampDegrees);
	if (AbsYawClamp <= KINDA_SMALL_NUMBER && AbsPitchClamp <= KINDA_SMALL_NUMBER)
	{
		Driver->ResetLookOffset();
		return FRotator::ZeroRotator;
	}

#if WITH_AUTOMATION_TESTS
	if (bHasStageLookNormalizedCursorOverrideForTest)
	{
		Driver->UpdateFromNormalizedCursor(
			StageLookNormalizedCursorOverrideForTest,
			DeltaTime,
			AbsYawClamp,
			AbsPitchClamp,
			BattleCamera->LookYawScale,
			BattleCamera->LookPitchScale,
			BattleCamera->LookInterpSpeed);
		return Driver->GetCurrentLookOffset();
	}
#endif

	Driver->UpdateFromPlayerCursor(
		&PlayerController,
		DeltaTime,
		AbsYawClamp,
		AbsPitchClamp,
		BattleCamera->LookYawScale,
		BattleCamera->LookPitchScale,
		BattleCamera->LookInterpSpeed);
	return Driver->GetCurrentLookOffset();
}

void UWacomFirstPersonViewStageBlendComponent::FinishActiveBlend()
{
	if (!bStageBlendActive)
	{
		return;
	}

	APlayerController* PC = ActivePlayerController.Get();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	TFunction<void()> FinishedCallback = MoveTemp(OnBlendFinished);

	ResetActiveBlendState(/*bResetCursorLookOffset*/false);

	if (Character && PC && FinishedCallback)
	{
		FinishedCallback();
	}
}

void UWacomFirstPersonViewStageBlendComponent::ResetActiveBlendState(
	bool bResetCursorLookOffset)
{
	bStageBlendActive = false;
	SetComponentTickEnabled(false);
	if (bResetCursorLookOffset)
	{
		if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
		{
			Driver->ResetLookOffset();
		}
	}
	ActivePlayerController.Reset();
	ActiveRequest = FWacomFirstPersonViewStageRequest();
	StartViewTransform = FTransform::Identity;
	CurrentBaseViewTransform = FTransform::Identity;
	OnBlendFinished = nullptr;
	BlendDurationSeconds = 0.0f;
	BlendElapsedSeconds = 0.0f;
	bHasCurrentBaseViewTransform = false;
#if WITH_AUTOMATION_TESTS
	bHasStageLookNormalizedCursorOverrideForTest = false;
	StageLookNormalizedCursorOverrideForTest = FVector2D::ZeroVector;
#endif
}
