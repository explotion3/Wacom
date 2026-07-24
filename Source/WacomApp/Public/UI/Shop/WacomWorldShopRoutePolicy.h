// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;
struct FRunShopVisitRequest;
struct FWacomWorldShopPresentationHost;

struct WACOMAPP_API FWacomWorldShopRouteDecision
{
	bool bUseWorldRoute = false;
	FName Reason = NAME_None;
};

/** 无副作用 World Shop presentation eligibility；不会提前 Begin visit。 */
struct WACOMAPP_API FWacomWorldShopRoutePolicy
{
	static FWacomWorldShopRouteDecision Evaluate(
		const FRunShopVisitRequest& Request,
		const FWacomWorldShopPresentationHost& Host,
		const UWorld* ExpectedWorld);
};
