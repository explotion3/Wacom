// Copyright Wacom. All Rights Reserved.

#include "Camera/WacomFirstPersonViewStageReturnFlow.h"

#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

bool FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
	AWacomPlayerCharacter& Pawn,
	APlayerController& PlayerController,
	TFunction<void()>&& OnCompleted)
{
	FWacomFirstPersonViewStageCoordinator::CancelActiveStage(Pawn);

	FWacomFirstPersonViewStageRequest ReturnRequest;
	bool bHasRunReturnRequest = false;
	if (const UWacomRunPathTraversalComponent* RunPath =
		Pawn.GetRunPathTraversalComponent())
	{
		bHasRunReturnRequest = RunPath->TryBuildReturnToRunPathStageRequest(ReturnRequest);
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
