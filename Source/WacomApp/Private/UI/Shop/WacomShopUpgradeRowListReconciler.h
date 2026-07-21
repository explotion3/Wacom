// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Shop/WacomShopUpgradePresentationBuilder.h"

class UVerticalBox;
class UWacomShopUpgradeRowWidget;

struct FWacomShopUpgradeRowListReconciler
{
	static void Reconcile(
		UVerticalBox* Panel,
		TConstArrayView<FWacomShopCardUpgradePresentationView> Desired,
		FGuid SelectedInstanceId,
		TFunctionRef<UWacomShopUpgradeRowWidget*(const FWacomShopCardUpgradePresentationView&)> CreateWidget);
};
