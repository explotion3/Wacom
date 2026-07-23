// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomWorldShopHostActor;
class UWorld;
struct FRunShopVisitRequest;

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
		const AWacomWorldShopHostActor* Host,
		const UWorld* ExpectedWorld);
};
