// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"

class URunSession;
class UWacomAppToastSubsystem;
class UWacomRunEventScreen;

struct FWacomRunEventPaymentDropFlowContext
{
	UWacomRunEventScreen* Screen = nullptr;
	URunSession* Run = nullptr;
	UWacomAppToastSubsystem* ToastSubsystem = nullptr;
	const TArray<FRunEventChoiceSnapshot>* CachedChoices = nullptr;
	const TMap<FName, FName>* PaymentZoneToChoiceId = nullptr;
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
