// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

class URunSession;
class UWacomAppToastSubsystem;
class UWacomRunEventScreen;

/** Private workflow helper for RunEventScreen command/settlement behavior. */
struct FWacomRunEventScreenFlow
{
	static void EndRunEventOnDeactivate(URunSession* Run, FGuid VisitToken, bool& bDidEndRunEvent);

	static bool ChooseChoice(
		UWacomRunEventScreen& Screen,
		URunSession* Run,
		UWacomAppToastSubsystem* ToastSubsystem,
		FName ChoiceId,
		bool& bDidEndRunEvent);

	static bool ApplyChoiceResult(
		UWacomRunEventScreen& Screen,
		URunSession* Run,
		UWacomAppToastSubsystem* ToastSubsystem,
		const FRunEventChoiceResult& Result,
		bool& bDidEndRunEvent);
};
