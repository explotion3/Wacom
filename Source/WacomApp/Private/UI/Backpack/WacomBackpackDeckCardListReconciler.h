// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "WacomBackpackWorkspaceTypes.h"

class UPanelWidget;

struct FWacomBackpackDeckCardListItem
{
	FRunStorageCardView CardView;
	EWacomBackpackDeckCardListReuseRole Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
	FText ProjectedBadgeText;
	FWacomBackpackZoneKey DisplayZone;
	bool bWorkspaceInteractive = true;
	EWacomBackpackWorkspaceCardReadOnlyKind ReadOnlyKind =
		EWacomBackpackWorkspaceCardReadOnlyKind::None;
};

struct WACOMAPP_API FWacomBackpackDeckCardListReconciler
{
	static void Reconcile(
		UPanelWidget* Panel,
		TConstArrayView<FWacomBackpackDeckCardListItem> DesiredCards,
		TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
		TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
		TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets = nullptr);

	static void ReconcileAcrossPanels(
		TConstArrayView<UPanelWidget*> SearchPanels,
		UPanelWidget* DestinationPanel,
		TConstArrayView<FWacomBackpackDeckCardListItem> DesiredCards,
		TFunctionRef<bool(const UWacomDeckCardWidget*)> PreserveCurrentParent,
		TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
		TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
		TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets = nullptr);
};
