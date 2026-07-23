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
	const int32 VersionBeforeVisit =
		Fixture.Session->BuildExplorationSnapshot().StateVersion;
	const FRunShopVisitResult BeginVisit =
		Fixture.Session->BeginShopVisitWithResult(
			TEXT("Shop.Policy.Actor"), { First, Second });
	TestTrue(TEXT("Shop begins"), BeginVisit.bSucceeded);
	TestTrue(TEXT("Shop begin exposes an explicit exploration result"),
		BeginVisit.ExplorationResolution.IsOk());
	TestEqual(TEXT("Shop begin advances the exploration version once"),
		BeginVisit.ExplorationResolution.VersionAfter,
		VersionBeforeVisit + 1);
	TestTrue(TEXT("Shop begin returns visit ownership"),
		BeginVisit.VisitToken.IsValid());
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

	const FRunShopVisitResult EndVisit =
		Fixture.Session->EndShopVisitIfOwnedWithResult(BeginVisit.VisitToken);
	TestTrue(TEXT("Owned Shop end succeeds"), EndVisit.bSucceeded);
	TestTrue(TEXT("Shop end exposes an explicit exploration result"),
		EndVisit.ExplorationResolution.IsOk());
	TestFalse(TEXT("End closes visit"), Fixture.Session->IsShopVisitActive());
	TestEqual(TEXT("End visit adds no delayed AP cost"),
		Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
		2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopDeferredPhaseAdvanceAndFailureTest,
	"Wacom.Run.NodeActivity.Shop.PhaseAdvanceDefersUntilCloseAndFailureRollsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopDeferredPhaseAdvanceAndFailureTest::RunTest(
	const FString& /*Parameters*/)
{
	{
		FShopActivityFixture Fixture;
		FRunShopOfferInput FirstOffer;
		FirstOffer.CardDefinition = Fixture.MakeCard(TEXT("Shop.Policy.PhaseCardA"));
		FirstOffer.Price = 1;
		FRunShopOfferInput SecondOffer;
		SecondOffer.CardDefinition = Fixture.MakeCard(TEXT("Shop.Policy.PhaseCardB"));
		SecondOffer.Price = 1;
		Fixture.Session->AddGold(2);
		const FRunShopVisitResult Begin =
			Fixture.Session->BeginShopVisitWithResult(
				TEXT("Shop.Policy.Phase.Actor"),
				{ FirstOffer, SecondOffer });
		TestTrue(TEXT("Phase shop begins"), Begin.bSucceeded);
		const FRunShopSnapshot OpenSnapshot =
			Fixture.Session->BuildCurrentShopSnapshot();
		const FRunShopPurchaseResult Purchase =
			Fixture.Session->PurchaseShopOffer(OpenSnapshot.Offers[0].OfferId);
		TestTrue(TEXT("Phase-ending purchase succeeds"), Purchase.bSucceeded);
		TestFalse(
			TEXT("phase-ending purchase defers visit close"),
			Purchase.bVisitClosedAfterPurchase);
		TestTrue(
			TEXT("shop remains active at zero AP"),
			Fixture.Session->IsShopVisitActive());
		TestEqual(
			TEXT("phase remains Morning while shop owns the activity"),
			Fixture.Session->BuildExplorationSnapshot().Time.CurrentTimePhase,
			ETimePhase::Morning);
		TestEqual(
			TEXT("first purchase records zero remaining AP"),
			Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
			0);

		const FRunShopPurchaseResult SecondPurchase =
			Fixture.Session->PurchaseShopOffer(OpenSnapshot.Offers[1].OfferId);
		TestTrue(
			TEXT("second purchase remains available before leaving"),
			SecondPurchase.bSucceeded);
		TestEqual(
			TEXT("second purchase adds no action point cost"),
			SecondPurchase.ActionPointCost,
			0);
		TestTrue(
			TEXT("shop remains active after second purchase"),
			Fixture.Session->IsShopVisitActive());

		const FRunShopVisitResult End =
			Fixture.Session->EndShopVisitIfOwnedWithResult(Begin.VisitToken);
		TestTrue(TEXT("owned close succeeds"), End.bSucceeded);
		TestFalse(TEXT("owned close ends visit"), Fixture.Session->IsShopVisitActive());
		TestEqual(
			TEXT("leaving shop advances deferred Morning exhaustion to Day"),
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
