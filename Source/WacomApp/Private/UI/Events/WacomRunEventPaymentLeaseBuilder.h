// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "RunState.h"

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
