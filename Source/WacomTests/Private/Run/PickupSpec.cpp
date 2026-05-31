// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "RunSession.h"

#include "UObject/StrongObjectPtr.h"

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
