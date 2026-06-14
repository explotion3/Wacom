// Copyright Wacom. All Rights Reserved.

#include "Camera/WacomFirstPersonViewStageCoordinator.h"

#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

bool FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
	AWacomPlayerCharacter& Pawn,
	APlayerController& PlayerController,
	const FWacomFirstPersonViewStageRequest& Request,
	TFunction<void()>&& OnDeferredCompleted)
{
	if (Request.bHasViewTransform
		&& Request.BlendTimeSeconds > KINDA_SMALL_NUMBER)
	{
		if (UWacomFirstPersonViewStageBlendComponent* StageBlend =
			Pawn.GetFirstPersonViewStageBlendComponent())
		{
			if (StageBlend->StartBlendToStageRequest(
				PlayerController,
				Request,
				MoveTemp(OnDeferredCompleted)))
			{
				return true;
			}
		}
	}

	if (Request.bHasViewTransform)
	{
		WacomFirstPersonViewpointPlacement::ApplyStageRequest(
			Pawn,
			PlayerController,
			Request);
	}
	return false;
}

bool FWacomFirstPersonViewStageCoordinator::StageFirstPersonViewAndActivateBattleCameraLook(
	AWacomPlayerCharacter& Pawn,
	APlayerController& PlayerController,
	const FWacomFirstPersonViewStageRequest& Request,
	TFunction<void()>&& OnDeferredBattleCameraActivated)
{
	TFunction<void()> DeferredCompletion =
		[WeakPawn = TWeakObjectPtr<AWacomPlayerCharacter>(&Pawn),
		 DeferredStageRequest = Request,
		 OnDeferredBattleCameraActivated = MoveTemp(OnDeferredBattleCameraActivated)]() mutable
		{
			if (AWacomPlayerCharacter* StrongPawn = WeakPawn.Get())
			{
				FWacomFirstPersonViewStageCoordinator::ActivateBattleCameraLookForStageRequest(
					*StrongPawn,
					DeferredStageRequest);
			}
			if (OnDeferredBattleCameraActivated)
			{
				OnDeferredBattleCameraActivated();
			}
		};

	const bool bDeferred = StageFirstPersonView(
		Pawn,
		PlayerController,
		Request,
		MoveTemp(DeferredCompletion));
	if (bDeferred)
	{
		return true;
	}

	ActivateBattleCameraLook(Pawn);
	return false;
}

bool FWacomFirstPersonViewStageCoordinator::ActivateBattleCameraLook(
	AWacomPlayerCharacter& Pawn)
{
	if (UWacomBattleCameraLookComponent* BattleCameraLook =
		Pawn.GetBattleCameraLookComponent())
	{
		return BattleCameraLook->ActivateBattleCameraLook();
	}
	return false;
}

bool FWacomFirstPersonViewStageCoordinator::ActivateBattleCameraLookForStageRequest(
	AWacomPlayerCharacter& Pawn,
	const FWacomFirstPersonViewStageRequest& Request)
{
	UWacomBattleCameraLookComponent* BattleCameraLook =
		Pawn.GetBattleCameraLookComponent();
	if (!BattleCameraLook)
	{
		return false;
	}
	if (!Request.bHasViewTransform)
	{
		return BattleCameraLook->ActivateBattleCameraLook();
	}

	FVector ActorLocation = FVector::ZeroVector;
	FRotator ActorRotation = FRotator::ZeroRotator;
	FRotator ControlRotation = FRotator::ZeroRotator;
	if (!WacomFirstPersonViewpointPlacement::CalculateActorTransformForView(
		Pawn,
		Request.ViewTransform,
		ActorLocation,
		ActorRotation,
		ControlRotation))
	{
		return BattleCameraLook->ActivateBattleCameraLook();
	}

	return BattleCameraLook->ActivateBattleCameraLookFromBaseRotation(
		ControlRotation,
		ActorRotation,
		/*bPreserveCurrentCursorLookOffset*/true);
}

void FWacomFirstPersonViewStageCoordinator::CancelActiveStage(
	AWacomPlayerCharacter& Pawn)
{
	if (UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Pawn.GetFirstPersonViewStageBlendComponent())
	{
		StageBlend->CancelActiveBlend();
	}
}
