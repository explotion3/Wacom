// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

class UVerticalBox;
class UWacomSpecialZoneWidget;

struct FWacomBackpackSpecialZoneListReconciler
{
	static void Reconcile(
		UVerticalBox* Panel,
		TConstArrayView<FRunSpecialStorageView> DesiredZones,
		TFunctionRef<UWacomSpecialZoneWidget*(const FRunSpecialStorageView&)> CreateWidget,
		TFunctionRef<void(UWacomSpecialZoneWidget&, const FRunSpecialStorageView&)> ApplyWidget,
		TFunctionRef<void(UWacomSpecialZoneWidget*)> OnRemovedWidget);
};
