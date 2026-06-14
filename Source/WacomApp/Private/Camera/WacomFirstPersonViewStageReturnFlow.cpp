// Copyright Wacom. All Rights Reserved.

#include "Camera/WacomFirstPersonViewStageReturnFlow.h"

#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

bool FWacomFirstPersonViewStageReturnFlow::ReturnToRunTunnel(
	AWacomPlayerCharacter& Pawn,
	APlayerController& PlayerController,
	TFunction<void()>&& OnCompleted)
{
	FWacomFirstPersonViewStageCoordinator::CancelActiveStage(Pawn);

	FWacomFirstPersonViewStageRequest ReturnRequest;
	if (const UWacomRunTunnelMovementComponent* RunTunnel =
		Pawn.GetRunTunnelMovementComponent())
	{
		RunTunnel->TryBuildReturnToRunTunnelStageRequest(ReturnRequest);
	}

	const TWeakObjectPtr<AWacomPlayerCharacter> WeakPawn(&Pawn);
	const TSharedRef<TFunction<void()>> Completion =
		MakeShared<TFunction<void()>>(MoveTemp(OnCompleted));
	auto CompleteReturn = [WeakPawn, Completion]()
	{
		if (AWacomPlayerCharacter* StrongPawn = WeakPawn.Get())
		{
			StrongPawn->SetExplorationInputEnabled(
				/*bEnabled*/true,
				/*bPreserveCursorLookOffset*/true);
		}
		if (*Completion)
		{
			(*Completion)();
		}
	};
	TFunction<void()> DeferredCompleteReturn = CompleteReturn;

	const bool bDeferred =
		FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
			Pawn,
			PlayerController,
			ReturnRequest,
			MoveTemp(DeferredCompleteReturn));
	if (bDeferred)
	{
		return true;
	}

	CompleteReturn();
	return false;
}
