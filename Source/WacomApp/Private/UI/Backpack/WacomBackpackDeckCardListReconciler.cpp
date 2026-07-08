// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"

#include "Components/PanelWidget.h"
#include "Components/WrapBox.h"

namespace
{
struct FWacomBackpackCardWidgetKey
{
	FGuid InstanceId;
	FGuid OwnerInstanceId;
	EZoneKind PhysicalZone = EZoneKind::Backpack;
	EWacomBackpackDeckCardListReuseRole Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;

	friend bool operator==(const FWacomBackpackCardWidgetKey& A, const FWacomBackpackCardWidgetKey& B)
	{
		return A.InstanceId == B.InstanceId
			&& A.OwnerInstanceId == B.OwnerInstanceId
			&& A.PhysicalZone == B.PhysicalZone
			&& A.Role == B.Role;
	}
};

uint32 GetTypeHash(const FWacomBackpackCardWidgetKey& Key)
{
	uint32 Hash = Key.InstanceId.A;
	Hash = HashCombine(Hash, Key.InstanceId.B);
	Hash = HashCombine(Hash, Key.InstanceId.C);
	Hash = HashCombine(Hash, Key.InstanceId.D);
	Hash = HashCombine(Hash, Key.OwnerInstanceId.A);
	Hash = HashCombine(Hash, Key.OwnerInstanceId.B);
	Hash = HashCombine(Hash, Key.OwnerInstanceId.C);
	Hash = HashCombine(Hash, Key.OwnerInstanceId.D);
	Hash = HashCombine(Hash, static_cast<uint32>(Key.PhysicalZone));
	Hash = HashCombine(Hash, static_cast<uint32>(Key.Role));
	return Hash;
}

FWacomBackpackCardWidgetKey MakeBackpackCardWidgetKey(
	const FRunStorageCardView& CardView,
	EWacomBackpackDeckCardListReuseRole Role)
{
	FWacomBackpackCardWidgetKey Key;
	Key.InstanceId = CardView.Instance.InstanceId;
	Key.PhysicalZone = CardView.PhysicalZone;
	Key.OwnerInstanceId = (CardView.PhysicalZone == EZoneKind::SpecialZone)
		? CardView.ZoneOwnerInstanceId
		: FGuid();
	Key.Role = Role;
	return Key;
}

FWacomBackpackCardWidgetKey MakeBackpackCardWidgetKey(const UWacomDeckCardWidget& Widget)
{
	FWacomBackpackCardWidgetKey Key;
	Key.InstanceId = Widget.GetCardInstanceId();
	Key.PhysicalZone = Widget.GetFromZone();
	Key.OwnerInstanceId = (Key.PhysicalZone == EZoneKind::SpecialZone)
		? Widget.GetFromZoneOwnerInstanceId()
		: FGuid();
	Key.Role = Widget.GetBackpackListReuseRole();
	return Key;
}

UPanelWidget* GetPanelParent(UWacomDeckCardWidget* Widget)
{
	return Widget ? Cast<UPanelWidget>(Widget->GetParent()) : nullptr;
}

void AddCardWidgetToPanel(UPanelWidget* Panel, UWacomDeckCardWidget* Widget)
{
	if (!Panel || !Widget)
	{
		return;
	}

	if (GetPanelParent(Widget) != Panel)
	{
		Widget->RemoveFromParent();
	}

	if (UWrapBox* WrapBox = Cast<UWrapBox>(Panel))
	{
		WrapBox->AddChildToWrapBox(Widget);
	}
	else
	{
		Panel->AddChild(Widget);
	}
}

void MoveCardWidgetToPanelIndex(UPanelWidget* Panel, UWacomDeckCardWidget* Widget, int32 DesiredIndex)
{
	if (!Panel || !Widget)
	{
		return;
	}

	if (GetPanelParent(Widget) == Panel)
	{
		Panel->ShiftChild(DesiredIndex, Widget);
		return;
	}

	AddCardWidgetToPanel(Panel, Widget);
	Panel->ShiftChild(DesiredIndex, Widget);
}
}

void FWacomBackpackDeckCardListReconciler::Reconcile(
	UPanelWidget* Panel,
	TConstArrayView<FWacomBackpackDeckCardListItem> DesiredCards,
	TFunctionRef<UWacomDeckCardWidget*(const FRunStorageCardView&)> CreateWidget,
	TFunctionRef<void(UWacomDeckCardWidget*)> OnRemovedWidget,
	TArray<TObjectPtr<UWacomDeckCardWidget>>* OutOrderedWidgets)
{
	if (OutOrderedWidgets)
	{
		OutOrderedWidgets->Reset();
		OutOrderedWidgets->Reserve(DesiredCards.Num());
	}

	if (!Panel)
	{
		return;
	}

	TMap<FWacomBackpackCardWidgetKey, UWacomDeckCardWidget*> ExistingByKey;
	for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
	{
		UWacomDeckCardWidget* ChildWidget = Cast<UWacomDeckCardWidget>(Panel->GetChildAt(ChildIndex));
		if (!ChildWidget)
		{
			continue;
		}

		ExistingByKey.Add(MakeBackpackCardWidgetKey(*ChildWidget), ChildWidget);
	}

	TSet<UWacomDeckCardWidget*> UsedWidgets;
	for (int32 DesiredIndex = 0; DesiredIndex < DesiredCards.Num(); ++DesiredIndex)
	{
		const FWacomBackpackDeckCardListItem& Desired = DesiredCards[DesiredIndex];
		const FRunStorageCardView& CardView = Desired.CardView;
		const FWacomBackpackCardWidgetKey Key = MakeBackpackCardWidgetKey(CardView, Desired.Role);
		UWacomDeckCardWidget* Widget = ExistingByKey.FindRef(Key);
		if (!Widget)
		{
			Widget = CreateWidget(CardView);
		}
		if (!Widget)
		{
			continue;
		}

		Widget->PrepareForBackpackListReuse();
		Widget->SetStorageCardView(CardView);
		Widget->SetMoveEnabled(true);
		Widget->SetBackpackListReuseRole(Desired.Role);
		Widget->SetProjectedFromBadgeText(Desired.ProjectedBadgeText);
		UsedWidgets.Add(Widget);
		if (OutOrderedWidgets)
		{
			OutOrderedWidgets->Add(Widget);
		}
		MoveCardWidgetToPanelIndex(Panel, Widget, DesiredIndex);
	}

	for (const TPair<FWacomBackpackCardWidgetKey, UWacomDeckCardWidget*>& ExistingPair : ExistingByKey)
	{
		if (ExistingPair.Value && !UsedWidgets.Contains(ExistingPair.Value))
		{
			OnRemovedWidget(ExistingPair.Value);
			ExistingPair.Value->RemoveFromParent();
		}
	}
}
