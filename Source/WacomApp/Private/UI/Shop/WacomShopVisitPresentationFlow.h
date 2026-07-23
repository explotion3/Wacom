// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

class AWacomPlayerController;
class UCardDefinition;
class URunSession;
class UWacomAppToastSubsystem;
struct FRunShopVisitRequest;
struct FWacomShopOfferPresentationView;

/**
 * Shop presentation 共享事务入口。
 * 只协调 Run 规则调用、resolution 顺序和 Toast；不持有 Screen/Host/Widget 生命周期。
 */
struct WACOMAPP_API FWacomShopVisitPresentationFlow
{
	static FRunShopVisitResult BeginVisit(
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		const FRunShopVisitRequest& Request);

	static FRunShopPurchaseResult PurchaseOffer(
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		UWacomAppToastSubsystem* ToastSubsystem,
		FGuid OfferId,
		const TArray<FWacomShopOfferPresentationView>& CachedOfferViews);

	static FRunShopVisitResult EndVisitIfOwned(
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		FGuid VisitToken);

	static FText BuildPurchaseFailureToastText(FName DisabledReason);
};
