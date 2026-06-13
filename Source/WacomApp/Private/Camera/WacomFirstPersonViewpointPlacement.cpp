// Copyright Wacom. All Rights Reserved.

#include "Camera/WacomFirstPersonViewpointPlacement.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

namespace WacomFirstPersonViewpointPlacement
{
	bool CalculateActorTransformForView(
		const AWacomPlayerCharacter& Character,
		const FTransform& ViewTransform,
		FVector& OutActorLocation,
		FRotator& OutActorRotation,
		FRotator& OutControlRotation)
	{
		const FRotator ViewRotation = ViewTransform.Rotator();
		OutControlRotation = FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.0f).GetNormalized();
		OutActorRotation = FRotator(0.0f, OutControlRotation.Yaw, 0.0f);

		OutActorLocation = ViewTransform.GetLocation();
		if (const UCameraComponent* Camera = Character.GetFirstPersonCamera())
		{
			OutActorLocation -= OutActorRotation.RotateVector(Camera->GetRelativeLocation());
		}
		return true;
	}

	bool ApplyViewTransform(
		AWacomPlayerCharacter& Character,
		APlayerController& PlayerController,
		const FTransform& ViewTransform)
	{
		FVector ActorLocation = FVector::ZeroVector;
		FRotator ActorRotation = FRotator::ZeroRotator;
		FRotator ControlRotation = FRotator::ZeroRotator;
		if (!CalculateActorTransformForView(
			Character,
			ViewTransform,
			ActorLocation,
			ActorRotation,
			ControlRotation))
		{
			return false;
		}

		Character.SetActorLocationAndRotation(
			ActorLocation,
			ActorRotation,
			/*bSweep*/false,
			nullptr,
			ETeleportType::TeleportPhysics);
		PlayerController.SetControlRotation(ControlRotation);
		return true;
	}

	bool ApplyStageRequest(
		AWacomPlayerCharacter& Character,
		APlayerController& PlayerController,
		const FWacomFirstPersonViewStageRequest& Request)
	{
		if (!Request.bHasViewTransform)
		{
			return false;
		}

		return ApplyViewTransform(Character, PlayerController, Request.ViewTransform);
	}
}
