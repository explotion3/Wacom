// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackSpecialZoneListReconciler.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"

namespace
{
void PlaceSpecialZoneWidget(UVerticalBox& Panel, UWacomSpecialZoneWidget& Widget, int32 DesiredIndex)
{
	if (Widget.GetParent() == &Panel)
	{
		Panel.ShiftChild(DesiredIndex, &Widget);
	}
	else if (UVerticalBoxSlot* ZoneSlot = Panel.AddChildToVerticalBox(&Widget))
	{
		ZoneSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		Panel.ShiftChild(DesiredIndex, &Widget);
		return;
	}

	if (UVerticalBoxSlot* ZoneSlot = Cast<UVerticalBoxSlot>(Widget.Slot))
	{
		ZoneSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
}
}

void FWacomBackpackSpecialZoneListReconciler::Reconcile(
	UVerticalBox* Panel,
	TConstArrayView<FRunSpecialStorageView> DesiredZones,
	TFunctionRef<UWacomSpecialZoneWidget*(const FRunSpecialStorageView&)> CreateWidget,
	TFunctionRef<void(UWacomSpecialZoneWidget&, const FRunSpecialStorageView&)> ApplyWidget,
	TFunctionRef<void(UWacomSpecialZoneWidget*)> OnRemovedWidget)
{
	if (!Panel)
	{
		return;
	}

	TMap<FGuid, UWacomSpecialZoneWidget*> ExistingByOwnerId;
	for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
	{
		UWacomSpecialZoneWidget* ExistingWidget = Cast<UWacomSpecialZoneWidget>(Panel->GetChildAt(ChildIndex));
		if (!ExistingWidget)
		{
			continue;
		}

		const FGuid OwnerInstanceId = ExistingWidget->GetOwnerCardInstanceId();
		if (OwnerInstanceId.IsValid())
		{
			ExistingByOwnerId.Add(OwnerInstanceId, ExistingWidget);
		}
	}

	TSet<UWacomSpecialZoneWidget*> UsedZoneWidgets;
	for (int32 DesiredIndex = 0; DesiredIndex < DesiredZones.Num(); ++DesiredIndex)
	{
		const FRunSpecialStorageView& SpecialView = DesiredZones[DesiredIndex];
		UWacomSpecialZoneWidget* ZoneWidget = ExistingByOwnerId.FindRef(SpecialView.OwnerCard.Instance.InstanceId);
		if (!ZoneWidget)
		{
			ZoneWidget = CreateWidget(SpecialView);
		}
		if (!ZoneWidget)
		{
			continue;
		}

		ApplyWidget(*ZoneWidget, SpecialView);
		UsedZoneWidgets.Add(ZoneWidget);
		PlaceSpecialZoneWidget(*Panel, *ZoneWidget, DesiredIndex);
	}

	for (const TPair<FGuid, UWacomSpecialZoneWidget*>& ExistingPair : ExistingByOwnerId)
	{
		if (ExistingPair.Value && !UsedZoneWidgets.Contains(ExistingPair.Value))
		{
			OnRemovedWidget(ExistingPair.Value);
			ExistingPair.Value->RemoveFromParent();
		}
	}
}
