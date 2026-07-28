// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	UCardDefinition* MakeUpgradeableCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("Card.Test.SingleDefinitionUpgrade");
		Card->Rarity = WacomTags::Card_Rarity_White;
		for (int32 Tier = 0; Tier < WacomCardUpgrade::TierCount; ++Tier)
		{
			FWacomCardTierProfile& Profile =
				Card->TierProfiles.AddDefaulted_GetRef();
			Profile.Description = FText::AsNumber(Tier);
			Profile.BaseCost = 4 - Tier;
		}
		return Card;
	}

	FRunShopVisitRequest MakeUpgradeShopRequest()
	{
		FRunShopVisitRequest Request;
		Request.ShopId = TEXT("Shop.Test.SingleDefinitionUpgrade");
		Request.CardUpgradeService.bEnabled = true;
		Request.CardUpgradeService.Prices =
		{
			{ WacomTags::Card_Rarity_White, 1 },
			{ WacomTags::Card_Rarity_Blue, 2 },
			{ WacomTags::Card_Rarity_Yellow, 3 },
		};
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardUpgradeSameDefinitionSpec,
	"Wacom.Run.CardUpgrade.SameDefinitionTierAndPersistentState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardUpgradeSameDefinitionSpec::RunTest(const FString&)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor =
		Fixture.MakeLinearFloor(TEXT("Upgrade.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Shop;
	URunSession* Session = Fixture.CreateInitializedSession(
		nullptr,
		Fixture.MakeJourney({ Floor })).Session;
	TestNotNull(TEXT("Run session"), Session);
	if (!Session)
	{
		return false;
	}
	FRunState& State =
		FWacomRunSessionTestAccess::GetMutableRunState(*Session);
	State.TimeState.CurrentTimePhase = ETimePhase::Day;
	State.TimeState.RemainingActionPoints = 5;
	UCardDefinition* Card = MakeUpgradeableCard(Session);

	FCardInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Definition = Card;
	Instance.UpgradeTier = EWacomCardUpgradeTier::White;
	Instance.PersistentModifiers.DurabilityBonus = 7;
	Instance.PersistentModifiers.EffectMagnitudeBonuses.Add(
		WacomTags::Effect_ApplyStatus_Burn,
		3);
	State.Backpack = { Instance };
	Session->AddGold(20);
	TestTrue(TEXT("Shop opens"),
		Session->BeginShopVisitRequest(MakeUpgradeShopRequest()));

	const FRunShopSnapshot Before = Session->BuildCurrentShopSnapshot();
	TestEqual(TEXT("One upgrade quote"), Before.CardUpgradeQuotes.Num(), 1);
	if (Before.CardUpgradeQuotes.IsEmpty())
	{
		return false;
	}
	const FRunShopCardUpgradeQuote& Quote = Before.CardUpgradeQuotes[0];
	TestTrue(TEXT("Quote keeps same definition"), Quote.Definition == Card);
	TestEqual(TEXT("Current tier White"), Quote.CurrentTier,
		EWacomCardUpgradeTier::White);
	TestEqual(TEXT("Next tier Blue"), Quote.NextTier,
		EWacomCardUpgradeTier::Blue);

	FRunShopCardUpgradeCommand Command;
	Command.InstanceId = Instance.InstanceId;
	Command.ExpectedDefinition = Card;
	Command.ExpectedCurrentTier = EWacomCardUpgradeTier::White;
	const FRunShopCardUpgradeResult Result =
		Session->UpgradeOwnedCardAtShop(Command);
	TestTrue(TEXT("Upgrade succeeds"), Result.bSucceeded);
	TestTrue(TEXT("Result keeps definition"), Result.Definition == Card);
	TestEqual(TEXT("Result tier Blue"), Result.NewTier,
		EWacomCardUpgradeTier::Blue);

	const FCardInstance& Upgraded = Session->GetRunState().Backpack[0];
	TestEqual(TEXT("InstanceId is stable"), Upgraded.InstanceId,
		Instance.InstanceId);
	TestTrue(TEXT("Definition is stable"), Upgraded.Definition == Card);
	TestEqual(TEXT("Tier increments"), Upgraded.UpgradeTier,
		EWacomCardUpgradeTier::Blue);
	TestEqual(TEXT("Persistent durability survives"),
		Upgraded.PersistentModifiers.DurabilityBonus, 7);
	TestEqual(TEXT("Persistent effect survives"),
		Upgraded.PersistentModifiers.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		3);

	const int32 GoldAfter = Session->GetGold();
	const FRunShopCardUpgradeResult Stale =
		Session->UpgradeOwnedCardAtShop(Command);
	TestFalse(TEXT("Stale tier request rejects"), Stale.bSucceeded);
	TestEqual(TEXT("Stale reason"), Stale.DisabledReason,
		FName(TEXT("StaleCurrentTier")));
	TestEqual(TEXT("Stale request spends no gold"),
		Session->GetGold(), GoldAfter);
	return true;
}
