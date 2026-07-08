// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "UI/Run/WacomRunMenuCardLeaseTypes.h"

struct FWacomRunEventPaymentLeaseBuildResult
{
	bool bHasCandidateCards = false;
	FWacomRunMenuCardLeaseRequest Request;
};

struct FWacomRunEventPaymentLeaseBuilder
{
	static FWacomRunEventPaymentLeaseBuildResult BuildRequest(
		TConstArrayView<FRunEventChoiceSnapshot> Choices);
};
