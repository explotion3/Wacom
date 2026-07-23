// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../../WacomApp/Private/UI/Shop/WacomShopVisitPresentationFlow.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeWorldShopBagCard(
		FWacomBattleFixture& Fixture,
		const int32 Capacity)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(0);
		Card->Physique.Capacity = Capacity;
		Card->Keywords.AddTag(WacomTags::Card_Keyword_BagProvider);
		Card->Rarity = WacomTags::Card_Rarity_White;
		return Card;
	}

	int32 CountBackpackDefinition(
		const URunSession& Run,
		const UCardDefinition* Definition)
	{
		int32 Count = 0;
		for (const FCardInstance& Instance : Run.GetBackpack())
		{
			Count += Instance.Definition == Definition ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFreePurchaseFlowSpec,
	"Wacom.UI.WorldShop.PurchaseFlow.FreeOfferIsStoredExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFreePurchaseFlowSpec::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Bag = MakeWorldShopBagCard(Fixture, 5);
	UCardDefinition* ShopCard = Fixture.MakeNoopCard(0);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(1),
		Fixture.MakeNoopCard(1),
		{ Bag });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	if (!TestTrue(
		TEXT("shop run initializes"),
		InitializeRunSessionForTest(
			*Run,
			Character,
			EWacomMapNodeType::Shop).IsOk()))
	{
		return false;
	}

	TArray<FRunShopOfferInput> Offers;
	Offers.Add({ ShopCard, 0 });
	if (!TestTrue(
		TEXT("free-offer visit opens"),
		Run->BeginShopVisit(TEXT("WorldShop.PurchaseFlow"), Offers)))
	{
		return false;
	}

	const FRunShopSnapshot BeforeSnapshot = Run->BuildCurrentShopSnapshot();
	if (!TestEqual(TEXT("one offer exists"), BeforeSnapshot.Offers.Num(), 1))
	{
		return false;
	}
	const FGuid OfferId = BeforeSnapshot.Offers[0].OfferId;
	const int32 BackpackBefore = Run->GetBackpack().Num();
	const int32 DefinitionBefore = CountBackpackDefinition(*Run, ShopCard);
	const uint64 RevisionBefore = Run->GetBackpackStorageSnapshotRevision();
	const TArray<FWacomShopOfferPresentationView> Views =
		UWacomShopPresentationBuilder::BuildOfferPresentationViews(
			BeforeSnapshot,
			Run->GetGold());

	const FRunShopPurchaseResult FirstPurchase =
		FWacomShopVisitPresentationFlow::PurchaseOffer(
			nullptr,
			Run.Get(),
			nullptr,
			OfferId,
			Views);
	TestTrue(TEXT("free purchase succeeds"), FirstPurchase.bSucceeded);
	TestFalse(
		TEXT("successful purchase does not force-close the visit"),
		FirstPurchase.bVisitClosedAfterPurchase);
	TestTrue(
		TEXT("App purchase flow keeps the shop active"),
		Run->IsShopVisitActive());
	TestEqual(
		TEXT("backpack gains exactly one physical card"),
		Run->GetBackpack().Num(),
		BackpackBefore + 1);
	TestEqual(
		TEXT("purchased definition count advances exactly once"),
		CountBackpackDefinition(*Run, ShopCard),
		DefinitionBefore + 1);
	TestTrue(
		TEXT("storage revision advances"),
		Run->GetBackpackStorageSnapshotRevision() > RevisionBefore);
	TestTrue(
		TEXT("offer becomes purchased"),
		Run->BuildCurrentShopSnapshot().Offers[0].bPurchased);

	const uint64 RevisionAfterFirst = Run->GetBackpackStorageSnapshotRevision();
	const FRunShopPurchaseResult DuplicatePurchase =
		FWacomShopVisitPresentationFlow::PurchaseOffer(
			nullptr,
			Run.Get(),
			nullptr,
			OfferId,
			Views);
	TestFalse(TEXT("duplicate purchase is rejected"), DuplicatePurchase.bSucceeded);
	TestEqual(
		TEXT("duplicate does not add a second physical card"),
		CountBackpackDefinition(*Run, ShopCard),
		DefinitionBefore + 1);
	TestEqual(
		TEXT("duplicate keeps storage revision stable"),
		Run->GetBackpackStorageSnapshotRevision(),
		RevisionAfterFirst);
	return true;
}
