// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"

class URunSession;
class AWacomPlayerController;
class UWacomAppToastSubsystem;
class UWacomShopScreen;

/** Private workflow helper for ShopScreen command/settlement behavior. */
struct FWacomShopScreenFlow
{
	static void EndShopVisitOnDeactivate(
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		FGuid VisitToken,
		bool& bDidEndShopVisit);

	static bool PurchaseOffer(
		UWacomShopScreen& Screen,
		AWacomPlayerController* PlayerController,
		URunSession* Run,
		UWacomAppToastSubsystem* ToastSubsystem,
		FGuid OfferId,
		const TArray<FWacomShopOfferPresentationView>& CachedOfferViews);

	static FText BuildPurchaseFailureToastText(FName DisabledReason);
};
