// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackZoneRackWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UWacomBackpackZoneRackWidget::RebuildWidget()
{
	EnsureEntriesHost();
	return Super::RebuildWidget();
}

UVerticalBox* UWacomBackpackZoneRackWidget::EnsureEntriesHost()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!EntriesHost)
	{
		EntriesHost = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("EntriesHost")));
	}
	if (!EntriesHost && !WidgetTree->RootWidget)
	{
		EntriesHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntriesHost"));
		WidgetTree->RootWidget = EntriesHost;
	}
	return EntriesHost;
}

void UWacomBackpackZoneRackWidget::SetZoneEntries(
	TConstArrayView<FWacomBackpackZoneRackEntryView> DesiredEntries)
{
	UVerticalBox* Host = EnsureEntriesHost();
	if (!Host)
	{
		return;
	}

	TMap<FString, UWacomBackpackZoneRackEntryWidget*> ExistingByIdentity;
	for (UWacomBackpackZoneRackEntryWidget* EntryWidget : EntryWidgets)
	{
		if (!EntryWidget)
		{
			continue;
		}
		const FWacomBackpackZoneRackEntryView& View = EntryWidget->GetEntryView();
		ExistingByIdentity.Add(
			FString::Printf(TEXT("%d:%s"), static_cast<int32>(View.Zone), *View.OwnerInstanceId.ToString(EGuidFormats::Digits)),
			EntryWidget);
	}

	TArray<TObjectPtr<UWacomBackpackZoneRackEntryWidget>> NewOrder;
	NewOrder.Reserve(DesiredEntries.Num());
	TSet<UWacomBackpackZoneRackEntryWidget*> Used;
	UClass* ClassToUse = EntryWidgetClass
		? EntryWidgetClass.Get()
		: UWacomBackpackZoneRackEntryWidget::StaticClass();
	for (int32 Index = 0; Index < DesiredEntries.Num(); ++Index)
	{
		const FWacomBackpackZoneRackEntryView& Desired = DesiredEntries[Index];
		const FGuid NormalizedOwner = Desired.Zone == EZoneKind::SpecialZone
			? Desired.OwnerInstanceId
			: FGuid();
		const FString Key = FString::Printf(
			TEXT("%d:%s"),
			static_cast<int32>(Desired.Zone),
			*NormalizedOwner.ToString(EGuidFormats::Digits));
		UWacomBackpackZoneRackEntryWidget* EntryWidget = ExistingByIdentity.FindRef(Key);
		if (!EntryWidget)
		{
			EntryWidget = WidgetTree->ConstructWidget<UWacomBackpackZoneRackEntryWidget>(ClassToUse);
			if (EntryWidget)
			{
				EntryWidget->OnZoneActivatedNative.AddUObject(
					this,
					&UWacomBackpackZoneRackWidget::HandleEntryActivated);
			}
		}
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetEntryView(Desired);
		if (EntryWidget->GetParent() != Host)
		{
			Host->AddChildToVerticalBox(EntryWidget);
		}
		Host->ShiftChild(Index, EntryWidget);
		if (UVerticalBoxSlot* RackEntrySlot = Cast<UVerticalBoxSlot>(EntryWidget->Slot))
		{
			RackEntrySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		Used.Add(EntryWidget);
		NewOrder.Add(EntryWidget);
	}

	for (UWacomBackpackZoneRackEntryWidget* Existing : EntryWidgets)
	{
		if (Existing && !Used.Contains(Existing))
		{
			Existing->RemoveFromParent();
		}
	}
	EntryWidgets = MoveTemp(NewOrder);
}

const FWacomBackpackZoneRackEntryView* UWacomBackpackZoneRackWidget::GetZoneEntryView(
	int32 Index) const
{
	return EntryWidgets.IsValidIndex(Index) && EntryWidgets[Index]
		? &EntryWidgets[Index]->GetEntryView()
		: nullptr;
}

void UWacomBackpackZoneRackWidget::HandleEntryActivated(
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	OnZoneActivatedNative.Broadcast(Zone, OwnerInstanceId);
}

bool UWacomBackpackZoneRackWidget::FindZoneAtAbsolutePosition(
	FVector2D AbsolutePosition,
	EZoneKind& OutZone,
	FGuid& OutOwnerInstanceId) const
{
	for (UWacomBackpackZoneRackEntryWidget* EntryWidget : EntryWidgets)
	{
		if (EntryWidget && EntryWidget->GetCachedGeometry().IsUnderLocation(AbsolutePosition))
		{
			const FWacomBackpackZoneRackEntryView& View = EntryWidget->GetEntryView();
			OutZone = View.Zone;
			OutOwnerInstanceId = View.OwnerInstanceId;
			return true;
		}
	}
	return false;
}

void UWacomBackpackZoneRackWidget::SetDropPreviewForZone(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	bool bVisible,
	bool bRejected)
{
	for (UWacomBackpackZoneRackEntryWidget* EntryWidget : EntryWidgets)
	{
		if (!EntryWidget)
		{
			continue;
		}
		const bool bMatches = EntryWidget->GetEntryView().HasSameIdentity(Zone, OwnerInstanceId);
		EntryWidget->SetDropPreviewState(bVisible && bMatches, bRejected);
	}
}
