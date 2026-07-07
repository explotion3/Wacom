// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventPaymentLeaseBuilder.h"

namespace
{
	const FName RunEventCardPaymentLeaseId(TEXT("RunEventCardPayment"));
	const FName RunEventCardPaymentSourceId(TEXT("RunEventCardPaymentSource"));
}

FWacomRunEventPaymentLeaseBuildResult FWacomRunEventPaymentLeaseBuilder::BuildRequest(
	const TArray<FRunEventChoiceSnapshot>& Choices)
{
	FWacomRunEventPaymentLeaseBuildResult Result;
	Result.Request.LeaseId = RunEventCardPaymentLeaseId;
	Result.Request.SourceId = RunEventCardPaymentSourceId;

	TArray<FGuid>& CandidateInstanceIds = Result.Request.ExplicitCardInstanceIds;
	for (const FRunEventChoiceSnapshot& Choice : Choices)
	{
		if (!Choice.bRequiresOwnedCardPayment)
		{
			continue;
		}

		for (const FGuid& CandidateId : Choice.PaymentCandidateInstanceIds)
		{
			if (CandidateId.IsValid())
			{
				CandidateInstanceIds.AddUnique(CandidateId);
			}
		}
	}

	Result.bHasCandidateCards = !CandidateInstanceIds.IsEmpty();
	return Result;
}
