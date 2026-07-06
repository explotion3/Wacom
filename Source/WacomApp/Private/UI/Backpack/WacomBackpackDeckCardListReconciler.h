// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

class UPanelWidget;

struct FWacomBackpackDeckCardListItem
{
	FRunStorageCardView CardView;
	EWacomBackpackDeckCardListReuseRole Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
	FText ProjectedBadgeText;
	bool bRightClickToggleEnabled = false;
};

struct FWacomBackpackDeckCardListReconciler
{
	static void Reconcile(
		UPanelWidget* Panel,
		TConstArrayView<FWacomBackpackDeckCardListItem> DesiredCards,
		TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
		TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
		TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets = nullptr);
};
