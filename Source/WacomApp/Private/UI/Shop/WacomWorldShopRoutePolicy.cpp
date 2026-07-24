// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopRoutePolicy.h"

#include "GameFramework/Actor.h"
#include "RunState.h"
#include "UI/Shop/WacomWorldShopPresentationHost.h"

FWacomWorldShopRouteDecision FWacomWorldShopRoutePolicy::Evaluate(
	const FRunShopVisitRequest& Request,
	const FWacomWorldShopPresentationHost& Host,
	const UWorld* ExpectedWorld)
{
	FWacomWorldShopRouteDecision Decision;
	const AActor* HostOwner = Host.GetOwner();
	if (!HostOwner)
	{
		Decision.Reason = TEXT("MissingHost");
		return Decision;
	}
	if (HostOwner->GetWorld() != ExpectedWorld)
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
		Host.ValidateForOfferCount(Request.Offers.Num());
	if (!Validation.bValid)
	{
		Decision.Reason = Validation.FailureReason;
		return Decision;
	}
	Decision.bUseWorldRoute = true;
	Decision.Reason = TEXT("Eligible");
	return Decision;
}
