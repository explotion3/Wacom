// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	int32 CountOwnedStorageCardsByDefinition(
		const FRunBackpackStorageSnapshot& Snapshot,
		const UCardDefinition* Card)
	{
		if (!Card)
		{
			return 0;
		}

		int32 Count = 0;
		const auto CountView = [&Count, Card](const FRunStorageCardView& View)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				++Count;
			}
		};

		for (const FRunStorageCardView& View : Snapshot.Flux.ContentCards)
		{
			CountView(View);
		}
		for (const FRunStorageCardView& View : Snapshot.BattleDeckPhysicalCards)
		{
			CountView(View);
		}
		for (const FRunStorageCardView& View : Snapshot.BurdenCards)
		{
			CountView(View);
		}
		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			CountView(Special.OwnerCard);
			for (const FRunStorageCardView& View : Special.ContentCards)
			{
				CountView(View);
			}
		}

		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPickupCollectGoldSpec,
	"Wacom.Run.Pickup.CollectGoldPickupAddsGoldAndMarksCollected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPickupCollectGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const FName PickupId(TEXT("Pickup.Gold.Basic"));

	TestTrue(TEXT("Gold pickup collects"), Run->CollectGoldPickup(PickupId, 3));
	TestEqual(TEXT("Gold added"), Run->GetGold(), 3);
	TestTrue(TEXT("Pickup marked collected"), Run->IsPickupCollected(PickupId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPickupRejectsMissingIdSpec,
	"Wacom.Run.Pickup.CollectGoldPickupRejectsMissingId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPickupRejectsMissingIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());

	TestFalse(TEXT("Missing id is rejected"), Run->CollectGoldPickup(NAME_None, 3));
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), 0);
	TestFalse(TEXT("None id never reports collected"), Run->IsPickupCollected(NAME_None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPickupRejectsRepeatSpec,
	"Wacom.Run.Pickup.CollectGoldPickupRejectsRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPickupRejectsRepeatSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const FName PickupId(TEXT("Pickup.Gold.Repeat"));

	TestTrue(TEXT("First pickup collects"), Run->CollectGoldPickup(PickupId, 2));
	TestFalse(TEXT("Second pickup rejected"), Run->CollectGoldPickup(PickupId, 2));
	TestEqual(TEXT("Gold added only once"), Run->GetGold(), 2);
	TestTrue(TEXT("Pickup remains collected"), Run->IsPickupCollected(PickupId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunPickupRejectsNonPositiveGoldSpec,
	"Wacom.Run.Pickup.CollectGoldPickupRejectsNonPositiveGold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunPickupRejectsNonPositiveGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const FName PickupId(TEXT("Pickup.Gold.Invalid"));

	TestFalse(TEXT("Zero gold rejected"), Run->CollectGoldPickup(PickupId, 0));
	TestFalse(TEXT("Negative gold rejected"), Run->CollectGoldPickup(PickupId, -1));
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), 0);
	TestFalse(TEXT("Invalid pickup not marked collected"), Run->IsPickupCollected(PickupId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPickupCollectsSpec,
	"Wacom.Run.Pickup.CollectCardPickupAddsCardAndMarksCollected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPickupCollectsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	const FName PickupId(TEXT("Pickup.Card.Basic"));
	Card->CardId = TEXT("Pickup.Card.Reward");

	TestTrue(TEXT("Card pickup collects"), Run->CollectCardPickup(PickupId, Card.Get()));
	TestEqual(TEXT("Reward card added to owned storage"),
		CountOwnedStorageCardsByDefinition(Run->BuildBackpackStorageSnapshot(), Card.Get()), 1);
	TestTrue(TEXT("Pickup marked collected"), Run->IsPickupCollected(PickupId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPickupRejectsMissingIdSpec,
	"Wacom.Run.Pickup.CollectCardPickupRejectsMissingId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPickupRejectsMissingIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	TestFalse(TEXT("Missing id is rejected"), Run->CollectCardPickup(NAME_None, Card.Get()));
	TestEqual(TEXT("Backpack unchanged"), Run->GetBackpack().Num(), 0);
	TestFalse(TEXT("None id never reports collected"), Run->IsPickupCollected(NAME_None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPickupRejectsMissingCardSpec,
	"Wacom.Run.Pickup.CollectCardPickupRejectsMissingCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPickupRejectsMissingCardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const FName PickupId(TEXT("Pickup.Card.Missing"));

	TestFalse(TEXT("Missing card is rejected"), Run->CollectCardPickup(PickupId, nullptr));
	TestEqual(TEXT("Backpack unchanged"), Run->GetBackpack().Num(), 0);
	TestFalse(TEXT("Invalid pickup not marked collected"), Run->IsPickupCollected(PickupId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPickupRejectsRepeatSpec,
	"Wacom.Run.Pickup.CollectCardPickupRejectsRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPickupRejectsRepeatSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	const FName PickupId(TEXT("Pickup.Card.Repeat"));
	Card->CardId = TEXT("Pickup.Card.RepeatReward");

	TestTrue(TEXT("First pickup collects"), Run->CollectCardPickup(PickupId, Card.Get()));
	TestFalse(TEXT("Second pickup rejected"), Run->CollectCardPickup(PickupId, Card.Get()));
	TestEqual(TEXT("Card added only once"),
		CountOwnedStorageCardsByDefinition(Run->BuildBackpackStorageSnapshot(), Card.Get()), 1);
	TestTrue(TEXT("Pickup remains collected"), Run->IsPickupCollected(PickupId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardPickupSharesGoldPickupIdSpec,
	"Wacom.Run.Pickup.CollectCardPickupSharesCollectedStateWithGoldPickupId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardPickupSharesGoldPickupIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	const FName PickupId(TEXT("Pickup.Shared.Id"));

	TestTrue(TEXT("Gold pickup collects shared id"), Run->CollectGoldPickup(PickupId, 2));
	TestFalse(TEXT("Card pickup with shared id is rejected"),
		Run->CollectCardPickup(PickupId, Card.Get()));
	TestEqual(TEXT("Gold unchanged"), Run->GetGold(), 2);
	TestEqual(TEXT("Backpack unchanged"), Run->GetBackpack().Num(), 0);
	TestTrue(TEXT("Shared id is collected"), Run->IsPickupCollected(PickupId));

	return true;
}
