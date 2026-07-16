// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackZonePileViewSpec,
	"Wacom.UI.Backpack.Workspace.ZonePileViewContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackZonePileViewSpec::RunTest(const FString& Parameters)
{
	UCardDefinition* OwnerDefinition = NewObject<UCardDefinition>();
	OwnerDefinition->DisplayName = FText::FromString(TEXT("蛛茧"));
	UCardDefinition* ContentDefinition = NewObject<UCardDefinition>();
	ContentDefinition->DisplayName = FText::FromString(TEXT("内容卡"));

	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.BattleDeckCapacity = 21;
	FRunSpecialStorageView Special;
	Special.Capacity = 2;
	Special.OwnerCard.Instance.InstanceId = FGuid(1, 2, 3, 4);
	Special.OwnerCard.Instance.Definition = OwnerDefinition;
	FRunStorageCardView Content;
	Content.Instance.InstanceId = FGuid(5, 6, 7, 8);
	Content.Instance.Definition = ContentDefinition;
	Content.PhysicalZone = EZoneKind::SpecialZone;
	Content.ZoneOwnerInstanceId = Special.OwnerCard.Instance.InstanceId;
	Special.ContentCards.Add(Content);
	Snapshot.SpecialZones.Add(Special);

	const TArray<FWacomBackpackZonePileView> Views =
		UWacomBackpackScreenPresenter::BuildWorkspacePileViews(
			Snapshot,
			EZoneKind::SpecialZone,
			Special.OwnerCard.Instance.InstanceId,
			true);
	TestEqual(TEXT("Battle and Special become embedded piles"), Views.Num(), 2);
	const FWacomBackpackZonePileView* SpecialPile = Views.FindByPredicate(
		[](const FWacomBackpackZonePileView& View)
		{
			return View.Zone == EZoneKind::SpecialZone;
		});
	TestNotNull(TEXT("Special pile is identified by its owner"), SpecialPile);
	if (SpecialPile)
	{
		TestEqual(TEXT("Special pile count contains only operable content"), SpecialPile->CardCount, 1);
		TestEqual(TEXT("Special pile keeps its owner identity"),
			SpecialPile->OwnerInstanceId, Special.OwnerCard.Instance.InstanceId);
		TestTrue(TEXT("Requested Special pile is the sole expanded pile"), SpecialPile->bExpanded);
	}
	return true;
}

#endif
