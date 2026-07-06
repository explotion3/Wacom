// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"

class UVerticalBox;
class UWacomShopOfferRowWidget;

struct FWacomShopOfferRowListReconciler
{
	static void Reconcile(
		UVerticalBox* Panel,
		TConstArrayView<FWacomShopOfferPresentationView> DesiredOffers,
		TFunctionRef<UWacomShopOfferRowWidget*(const FWacomShopOfferPresentationView&)> CreateWidget,
		TFunctionRef<void(UWacomShopOfferRowWidget&, const FWacomShopOfferPresentationView&)> ApplyWidget);
};
