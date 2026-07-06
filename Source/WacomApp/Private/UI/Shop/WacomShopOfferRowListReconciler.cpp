// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopOfferRowListReconciler.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Shop/WacomShopOfferRowWidget.h"

namespace
{
void PlaceShopOfferRowWidget(UVerticalBox& Panel, UWacomShopOfferRowWidget& Widget, int32 DesiredIndex)
{
	if (Widget.GetParent() == &Panel)
	{
		Panel.ShiftChild(DesiredIndex, &Widget);
	}
	else if (UVerticalBoxSlot* RowSlot = Panel.AddChildToVerticalBox(&Widget))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Panel.ShiftChild(DesiredIndex, &Widget);
		return;
	}

	if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Widget.Slot))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
}
}

void FWacomShopOfferRowListReconciler::Reconcile(
	UVerticalBox* Panel,
	TConstArrayView<FWacomShopOfferPresentationView> DesiredOffers,
	TFunctionRef<UWacomShopOfferRowWidget*(const FWacomShopOfferPresentationView&)> CreateWidget,
	TFunctionRef<void(UWacomShopOfferRowWidget&, const FWacomShopOfferPresentationView&)> ApplyWidget)
{
	if (!Panel)
	{
		return;
	}

	TMap<FGuid, UWacomShopOfferRowWidget*> ExistingRowsByOfferId;
	for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
	{
		UWacomShopOfferRowWidget* ExistingRow = Cast<UWacomShopOfferRowWidget>(Panel->GetChildAt(ChildIndex));
		if (!ExistingRow)
		{
			continue;
		}

		const FGuid ExistingOfferId = ExistingRow->GetOfferPresentationView().OfferId;
		if (ExistingOfferId.IsValid())
		{
			ExistingRowsByOfferId.Add(ExistingOfferId, ExistingRow);
		}
	}

	TSet<UWacomShopOfferRowWidget*> UsedRows;
	for (int32 DesiredIndex = 0; DesiredIndex < DesiredOffers.Num(); ++DesiredIndex)
	{
		const FWacomShopOfferPresentationView& OfferView = DesiredOffers[DesiredIndex];
		UWacomShopOfferRowWidget* RowWidget = ExistingRowsByOfferId.FindRef(OfferView.OfferId);
		if (!RowWidget)
		{
			RowWidget = CreateWidget(OfferView);
		}
		if (!RowWidget)
		{
			continue;
		}

		ApplyWidget(*RowWidget, OfferView);
		UsedRows.Add(RowWidget);
		PlaceShopOfferRowWidget(*Panel, *RowWidget, DesiredIndex);
	}

	for (const TPair<FGuid, UWacomShopOfferRowWidget*>& ExistingPair : ExistingRowsByOfferId)
	{
		if (ExistingPair.Value && !UsedRows.Contains(ExistingPair.Value))
		{
			ExistingPair.Value->RemoveFromParent();
		}
	}
}
