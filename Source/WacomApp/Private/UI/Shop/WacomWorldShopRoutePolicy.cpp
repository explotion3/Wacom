// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopRoutePolicy.h"

#include "Actors/WacomWorldShopHostActor.h"
#include "RunState.h"

FWacomWorldShopRouteDecision FWacomWorldShopRoutePolicy::Evaluate(
	const FRunShopVisitRequest& Request,
	const AWacomWorldShopHostActor* Host,
	const UWorld* ExpectedWorld)
{
	FWacomWorldShopRouteDecision Decision;
	if (!Host)
	{
		Decision.Reason = TEXT("MissingHost");
		return Decision;
	}
	if (Host->GetWorld() != ExpectedWorld)
	{
		Decision.Reason = TEXT("DifferentWorld");
		return Decision;
	}
	if (Request.ShopId.IsNone() || Request.Offers.IsEmpty())
	{
		Decision.Reason = TEXT("InvalidRequest");
		return Decision;
	}
	if (Request.CardUpgradeService.bEnabled)
	{
		Decision.Reason = TEXT("UpgradeRequiresScreen");
		return Decision;
	}
	const FWacomWorldShopHostValidationResult Validation =
		Host->ValidateForOfferCount(Request.Offers.Num());
	if (!Validation.bValid)
	{
		Decision.Reason = Validation.FailureReason;
		return Decision;
	}
	Decision.bUseWorldRoute = true;
	Decision.Reason = TEXT("Eligible");
	return Decision;
}
