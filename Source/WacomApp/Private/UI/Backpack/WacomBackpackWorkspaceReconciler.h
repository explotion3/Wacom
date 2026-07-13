// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "WacomBackpackWorkspaceTypes.h"

class UWacomBackpackWorkspaceStyle;
class UWacomBackpackWorkspaceWidget;
class UWacomDeckCardWidget;
struct FWacomBackpackWorkspaceStateStore;

/** InstanceId 驱动的活动工作台卡牌协调器。 */
struct FWacomBackpackWorkspaceReconciler
{
	static void Reconcile(
		UWacomBackpackWorkspaceWidget& Workspace,
		const FRunBackpackStorageSnapshot& Snapshot,
		const FWacomBackpackZoneKey& ActiveZone,
		FWacomBackpackWorkspaceStateStore& StateStore,
		const UWacomBackpackWorkspaceStyle* Style,
		TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
		TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
		TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets = nullptr);
};
