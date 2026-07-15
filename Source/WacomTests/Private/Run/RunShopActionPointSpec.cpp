// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"

namespace
{
	struct FShopActivityFixture
	{
		FWacomRunExplorationFixture Exploration;
		URunSession* Session = nullptr;

		FShopActivityFixture()
		{
			UWacomFloorMapDefinition* Floor =
				Exploration.MakeLinearFloor(TEXT("Shop.Policy.Floor"), 1);
			Floor->Nodes[0].NodeType = EWacomMapNodeType::Shop;
			Session = Exploration.CreateInitializedSession(
				nullptr,
				Exploration.MakeJourney({ Floor }, TEXT("Shop.Policy.Journey"))).Session;
		}

		UCardDefinition* MakeCard(FName CardId) const
		{
			UCardDefinition* Card = NewObject<UCardDefinition>(Session);
			Card->CardId = CardId;
			return Card;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopFirstPurchaseActionPointTest,
	"Wacom.Run.NodeActivity.Shop.BrowseFirstPurchaseAndSameVisit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopFirstPurchaseActionPointTest::RunTest(const FString& /*Parameters*/)
{
	FShopActivityFixture Fixture;
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session);
	State.TimeState.CurrentTimePhase = ETimePhase::Day;
	State.TimeState.RemainingActionPoints = 3;
	Fixture.Session->AddGold(2);

	FRunShopOfferInput First;
	First.CardDefinition = Fixture.MakeCard(TEXT("Shop.Policy.CardA"));
	First.Price = 1;
	FRunShopOfferInput Second;
	Second.CardDefinition = Fixture.MakeCard(TEXT("Shop.Policy.CardB"));
	Second.Price = 1;
	TestTrue(TEXT("Shop begins"),
		Fixture.Session->BeginShopVisit(TEXT("Shop.Policy.Actor"), { First, Second }));
	const FRunShopSnapshot OpenSnapshot = Fixture.Session->BuildCurrentShopSnapshot();
	TestEqual(TEXT("Browsing is free"),
		Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
		3);

	const FRunShopPurchaseResult FirstPurchase =
		Fixture.Session->PurchaseShopOffer(OpenSnapshot.Offers[0].OfferId);
	TestTrue(TEXT("First purchase succeeds"), FirstPurchase.bSucceeded);
	TestTrue(TEXT("First purchase is identified"), FirstPurchase.bFirstPurchaseThisVisit);
	TestEqual(TEXT("First purchase costs one AP"), FirstPurchase.ActionPointCost, 1);
	TestTrue(TEXT("First purchase has explicit exploration result"),
		FirstPurchase.ExplorationResolution.IsOk());
	TestFalse(TEXT("Shop remains open when phase does not advance"),
		FirstPurchase.bVisitClosedAfterPurchase);
	TestTrue(TEXT("Shop visit remains active"), Fixture.Session->IsShopVisitActive());
	TestEqual(TEXT("First purchase commits AP"),
		Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
		2);

	const FRunShopPurchaseResult SecondPurchase =
		Fixture.Session->PurchaseShopOffer(OpenSnapshot.Offers[1].OfferId);
	TestTrue(TEXT("Second purchase succeeds"), SecondPurchase.bSucceeded);
	TestFalse(TEXT("Second purchase is not first"), SecondPurchase.bFirstPurchaseThisVisit);
	TestEqual(TEXT("Second purchase is free"), SecondPurchase.ActionPointCost, 0);
	TestEqual(TEXT("Second purchase preserves AP"),
		Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
		2);

	Fixture.Session->EndShopVisit();
	TestFalse(TEXT("End closes visit"), Fixture.Session->IsShopVisitActive());
	TestEqual(TEXT("End visit adds no delayed AP cost"),
		Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
		2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopPhaseAdvanceAndFailureTest,
	"Wacom.Run.NodeActivity.Shop.PhaseAdvanceClosesAndFailureRollsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopPhaseAdvanceAndFailureTest::RunTest(const FString& /*Parameters*/)
{
	{
		FShopActivityFixture Fixture;
		UCardDefinition* Card = Fixture.MakeCard(TEXT("Shop.Policy.PhaseCard"));
		FRunShopOfferInput Offer;
		Offer.CardDefinition = Card;
		Offer.Price = 1;
		Fixture.Session->AddGold(1);
		TestTrue(TEXT("Phase shop begins"),
			Fixture.Session->BeginShopVisit(TEXT("Shop.Policy.Phase.Actor"), { Offer }));
		const FGuid OfferId = Fixture.Session->BuildCurrentShopSnapshot().Offers[0].OfferId;
		const FRunShopPurchaseResult Purchase = Fixture.Session->PurchaseShopOffer(OfferId);
		TestTrue(TEXT("Phase-ending purchase succeeds"), Purchase.bSucceeded);
		TestTrue(TEXT("Phase-ending purchase closes visit"), Purchase.bVisitClosedAfterPurchase);
		TestFalse(TEXT("Phase-ending visit is inactive"), Fixture.Session->IsShopVisitActive());
		TestEqual(TEXT("Morning exhaustion advances to Day"),
			Fixture.Session->BuildExplorationSnapshot().Time.CurrentTimePhase,
			ETimePhase::Day);
	}

	{
		FShopActivityFixture Fixture;
		FRunShopOfferInput Offer;
		Offer.CardDefinition = Fixture.MakeCard(TEXT("Shop.Policy.RejectedCard"));
		Offer.Price = 2;
		TestTrue(TEXT("Rejected shop begins"),
			Fixture.Session->BeginShopVisit(TEXT("Shop.Policy.Rejected.Actor"), { Offer }));
		const FGuid OfferId = Fixture.Session->BuildCurrentShopSnapshot().Offers[0].OfferId;
		const FRunExplorationSnapshot Before = Fixture.Session->BuildExplorationSnapshot();
		const FRunShopPurchaseResult Purchase = Fixture.Session->PurchaseShopOffer(OfferId);
		TestFalse(TEXT("Insufficient gold purchase fails"), Purchase.bSucceeded);
		TestEqual(TEXT("Failed purchase preserves AP"),
			Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
			Before.Time.RemainingActionPoints);
		TestEqual(TEXT("Failed purchase preserves version"),
			Fixture.Session->BuildExplorationSnapshot().StateVersion,
			Before.StateVersion);
		TestTrue(TEXT("Failed purchase keeps visit active"), Fixture.Session->IsShopVisitActive());
		TestFalse(TEXT("Failed offer remains available"),
			Fixture.Session->BuildCurrentShopSnapshot().Offers[0].bPurchased);
	}

	return true;
}
