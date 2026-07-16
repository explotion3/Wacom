// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

class URunSession;
class AWacomPlayerController;
class UWacomAppToastSubsystem;
class UWacomRunEventScreen;

/** Private workflow helper for RunEventScreen command/settlement behavior. */
struct FWacomRunEventScreenFlow
{
	static void EndRunEventOnDeactivate(
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		FGuid VisitToken,
		bool& bDidEndRunEvent);

	static bool ChooseChoice(
		UWacomRunEventScreen& Screen,
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		UWacomAppToastSubsystem* ToastSubsystem,
		FName ChoiceId,
		bool& bDidEndRunEvent);

	static bool ApplyChoiceResult(
		UWacomRunEventScreen& Screen,
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		UWacomAppToastSubsystem* ToastSubsystem,
		const FRunEventChoiceResult& Result,
		bool& bDidEndRunEvent);
};
