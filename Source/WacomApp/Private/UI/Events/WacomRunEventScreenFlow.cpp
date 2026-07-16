// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventScreenFlow.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Events/WacomRunEventPresentationBuilder.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomAppToastTypes.h"

namespace
{
	void ShowToasts(
		UWacomAppToastSubsystem* ToastSubsystem,
		const TArray<FWacomAppToastView>& ToastViews)
	{
		if (!ToastSubsystem)
		{
			return;
		}

		for (const FWacomAppToastView& ToastView : ToastViews)
		{
			ToastSubsystem->ShowToast(ToastView);
		}
	}
}

void FWacomRunEventScreenFlow::EndRunEventOnDeactivate(
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	FGuid VisitToken,
	bool& bDidEndRunEvent)
{
	if (bDidEndRunEvent)
	{
		return;
	}

	if (Run)
	{
		const FRunExplorationResolution Resolution =
			Run->EndRunEventIfOwnedWithExplorationResult(VisitToken);
		if (Resolution.IsOk() && Resolution.VersionAfter > 0
			&& PlayerController
			&& !PlayerController->ApplyRunNodeActivityResolutionForPresentation(
				Resolution))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomRunEventScreenFlow] RunEvent End 结果未按序应用到 Run 表现"));
		}
	}
	bDidEndRunEvent = true;
}

bool FWacomRunEventScreenFlow::ChooseChoice(
	UWacomRunEventScreen& Screen,
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	UWacomAppToastSubsystem* ToastSubsystem,
	FName ChoiceId,
	bool& bDidEndRunEvent)
{
	if (!Run)
	{
		return false;
	}

	return ApplyChoiceResult(
		Screen,
		PlayerController,
		Run,
		ToastSubsystem,
		Run->ChooseRunEventOptionWithResult(ChoiceId),
		bDidEndRunEvent);
}

bool FWacomRunEventScreenFlow::ApplyChoiceResult(
	UWacomRunEventScreen& Screen,
	AWacomPlayerController* PlayerController,
	URunSession* Run,
	UWacomAppToastSubsystem* ToastSubsystem,
	const FRunEventChoiceResult& Result,
	bool& bDidEndRunEvent)
{
	if (!Run)
	{
		return false;
	}

	ShowToasts(
		ToastSubsystem,
		UWacomRunEventPresentationBuilder::BuildToastViewsFromChoiceResult(Result));
	if (!Result.bSucceeded)
	{
		Screen.RefreshEvent();
		return false;
	}
	if (Result.ExplorationResolution.IsOk()
		&& Result.ExplorationResolution.VersionAfter > 0
		&& PlayerController
		&& !PlayerController->ApplyRunNodeActivityResolutionForPresentation(
			Result.ExplorationResolution))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRunEventScreenFlow] RunEvent Choice 结果未按序应用到 Run 表现"));
	}

	if (!Run->IsRunEventActive())
	{
		bDidEndRunEvent = true;
		Screen.DeactivateWidget();
		return true;
	}

	Screen.RefreshEvent();
	return true;
}
