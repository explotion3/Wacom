// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "UI/Events/WacomRunEventPresentationState.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"

class URunSession;
class AWacomPlayerController;
class UWacomAppToastSubsystem;
class UWacomRunEventScreen;

struct FWacomRunEventPaymentDropFlowContext
{
	UWacomRunEventScreen* Screen = nullptr;
	AWacomPlayerController* PlayerController = nullptr;
	URunSession* Run = nullptr;
	UWacomAppToastSubsystem* ToastSubsystem = nullptr;
	FWacomRunEventPresentationStateView PresentationState;
	bool* bDidEndRunEvent = nullptr;
};

struct FWacomRunEventPaymentDropFlow
{
	static FWacomRunMenuCardDropResolveResult ResolveDropIntent(
		const FWacomRunEventPaymentDropFlowContext& Context,
		const FWacomRunMenuCardDropResolveResult& Candidate);

	static bool SubmitDropIntent(
		const FWacomRunEventPaymentDropFlowContext& Context,
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted);
};
