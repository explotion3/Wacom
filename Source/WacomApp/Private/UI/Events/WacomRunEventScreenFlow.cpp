// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventScreenFlow.h"

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

void FWacomRunEventScreenFlow::EndRunEventOnDeactivate(URunSession* Run, bool& bDidEndRunEvent)
{
	if (bDidEndRunEvent)
	{
		return;
	}

	if (Run)
	{
		Run->EndRunEvent();
	}
	bDidEndRunEvent = true;
}

bool FWacomRunEventScreenFlow::ChooseChoice(
	UWacomRunEventScreen& Screen,
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
		Run,
		ToastSubsystem,
		Run->ChooseRunEventOptionWithResult(ChoiceId),
		bDidEndRunEvent);
}

bool FWacomRunEventScreenFlow::ApplyChoiceResult(
	UWacomRunEventScreen& Screen,
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

	if (!Run->IsRunEventActive())
	{
		bDidEndRunEvent = true;
		Screen.DeactivateWidget();
		return true;
	}

	Screen.RefreshEvent();
	return true;
}
