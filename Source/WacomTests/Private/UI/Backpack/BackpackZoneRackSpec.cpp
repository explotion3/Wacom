// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackZoneRackWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackZoneRackSnapshotPresenterSpec,
	"Wacom.UI.Backpack.Workspace.ZoneRackSnapshotPresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackZoneRackSnapshotPresenterSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> OwnerDefinition(NewObject<UCardDefinition>());
	OwnerDefinition->DisplayName = FText::FromString(TEXT("蛛茧绒囊"));
	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.FluxContentCount = 7;
	Snapshot.FluxCapacity = 10;
	Snapshot.BattleDeckPhysicalCount = 4;
	Snapshot.BattleDeckCapacity = 6;
	Snapshot.BurdenCount = 2;
	FRunSpecialStorageView Special;
	Special.OwnerCard.Instance.InstanceId = FGuid(1, 2, 3, 4);
	Special.OwnerCard.Instance.Definition = OwnerDefinition.Get();
	Special.Capacity = 3;
	Special.ContentCards.SetNum(2);
	Snapshot.SpecialZones.Add(Special);

	const TArray<FWacomBackpackZoneRackEntryView> Entries =
		UWacomBackpackScreenPresenter::BuildZoneRackEntries(
			Snapshot,
			EZoneKind::BattleDeck,
			FGuid::NewGuid());
	TestEqual(TEXT("Rack includes flux, battle, special, and non-empty burden"), Entries.Num(), 4);
	int32 ActiveCount = 0;
	for (const FWacomBackpackZoneRackEntryView& Entry : Entries)
	{
		ActiveCount += Entry.bActive ? 1 : 0;
	}
	TestEqual(TEXT("Rack has exactly one active entry"), ActiveCount, 1);
	TestEqual(TEXT("Flux count comes from snapshot"), Entries[0].CardCount, 7);
	TestEqual(TEXT("Flux capacity comes from snapshot"), Entries[0].Capacity, 10);
	TestTrue(TEXT("Battle entry is active"), Entries[1].bActive);
	TestEqual(TEXT("Special title uses owner display name"), Entries[2].Title.ToString(), FString(TEXT("蛛茧绒囊")));
	TestEqual(TEXT("Special content count comes from snapshot"), Entries[2].CardCount, 2);
	TestEqual(TEXT("Burden entry exposes count without fake capacity"), Entries[3].bHasCapacity, false);

	TStrongObjectPtr<UWacomBackpackZoneRackWidget> Rack(NewObject<UWacomBackpackZoneRackWidget>());
	Rack->TakeWidget();
	Rack->SetZoneEntries(Entries);
	TestEqual(TEXT("Rack renders one entry per presenter record"), Rack->GetZoneEntryCount(), 4);
	const FWacomBackpackZoneRackEntryView* ActiveView = Rack->GetZoneEntryView(1);
	TestTrue(TEXT("Rendered battle entry preserves active state"), ActiveView && ActiveView->bActive);
	return true;
}

#endif
