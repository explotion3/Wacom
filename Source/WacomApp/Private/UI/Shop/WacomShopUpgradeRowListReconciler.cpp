// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopUpgradeRowListReconciler.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Shop/WacomShopUpgradeRowWidget.h"

void FWacomShopUpgradeRowListReconciler::Reconcile(
	UVerticalBox* Panel,
	TConstArrayView<FWacomShopCardUpgradePresentationView> Desired,
	FGuid SelectedInstanceId,
	TFunctionRef<UWacomShopUpgradeRowWidget*(const FWacomShopCardUpgradePresentationView&)> CreateWidget)
{
	if (!Panel)
	{
		return;
	}

	TMap<FGuid, UWacomShopUpgradeRowWidget*> Existing;
	for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
	{
		if (UWacomShopUpgradeRowWidget* Row = Cast<UWacomShopUpgradeRowWidget>(Panel->GetChildAt(Index)))
		{
			Existing.Add(Row->GetPresentationView().InstanceId, Row);
		}
	}

	TSet<UWacomShopUpgradeRowWidget*> Used;
	for (int32 Index = 0; Index < Desired.Num(); ++Index)
	{
		const FWacomShopCardUpgradePresentationView& View = Desired[Index];
		UWacomShopUpgradeRowWidget* Row = Existing.FindRef(View.InstanceId);
		if (!Row)
		{
			Row = CreateWidget(View);
		}
		if (!Row)
		{
			continue;
		}
		Row->SetPresentationView(View, View.InstanceId == SelectedInstanceId);
		if (Row->GetParent() == Panel)
		{
			Panel->ShiftChild(Index, Row);
		}
		else if (UVerticalBoxSlot* Slot = Panel->AddChildToVerticalBox(Row))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			Panel->ShiftChild(Index, Row);
		}
		Used.Add(Row);
	}
	for (const TPair<FGuid, UWacomShopUpgradeRowWidget*>& Pair : Existing)
	{
		if (Pair.Value && !Used.Contains(Pair.Value))
		{
			Pair.Value->RemoveFromParent();
		}
	}
}
