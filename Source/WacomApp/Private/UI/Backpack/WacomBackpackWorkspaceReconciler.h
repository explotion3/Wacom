// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "WacomBackpackWorkspaceTypes.h"

class UWacomBackpackWorkspaceStyle;
class UWacomBackpackWorkspaceWidget;
class UWacomDeckCardWidget;
class FWacomBackpackWorkspaceInteractionModel;
struct FWacomBackpackWorkspaceStateStore;

/** InstanceId 驱动的活动工作台卡牌协调器。 */
struct WACOMAPP_API FWacomBackpackWorkspaceReconciler
{
	static void Reconcile(
		UWacomBackpackWorkspaceWidget& Workspace,
		const FRunBackpackStorageSnapshot& Snapshot,
		FWacomBackpackWorkspaceStateStore& StateStore,
		const FWacomBackpackWorkspaceInteractionModel* InteractionModel,
		const UWacomBackpackWorkspaceStyle* Style,
		TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
		TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
		TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets = nullptr);
};
