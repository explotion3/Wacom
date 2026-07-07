// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"

class URunSession;

struct FWacomRunEventScreenDebugBuildContext
{
	const URunSession* Run = nullptr;
	const TArray<FRunEventChoiceSnapshot>* CachedChoices = nullptr;
	const TMap<FName, FName>* PaymentZoneToChoiceId = nullptr;
	FString LastPaymentResolveSummary;
	FString LastPaymentSubmitSummary;
};

struct FWacomRunEventScreenDebugBuilder
{
	static FWacomRunEventScreenDebugView BuildView(
		const FWacomRunEventScreenDebugBuildContext& Context);

	static FString BuildSummary(
		const FWacomRunEventScreenDebugView& View);

	static FString BuildDropResultSummary(
		const TCHAR* Prefix,
		const FWacomRunMenuCardDropResolveResult& Result);
};
