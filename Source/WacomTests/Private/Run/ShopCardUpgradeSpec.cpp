// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomRunShopCardUpgradeSpec
{
	struct FShopCardUpgradeFixture
	{
		FWacomRunExplorationFixture Exploration;
		URunSession* Session = nullptr;

		FShopCardUpgradeFixture()
		{
			UWacomFloorMapDefinition* Floor =
				Exploration.MakeLinearFloor(TEXT("Shop.Upgrade.Floor"), 1);
			Floor->Nodes[0].NodeType = EWacomMapNodeType::Shop;
			Session = Exploration.CreateInitializedSession(
				nullptr,
				Exploration.MakeJourney({ Floor }, TEXT("Shop.Upgrade.Journey"))).Session;
			FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Session);
			State.TimeState.CurrentTimePhase = ETimePhase::Day;
			State.TimeState.RemainingActionPoints = 5;
		}

		UCardDefinition* MakeCard(
			const TCHAR* CardId,
			const TCHAR* FamilyId,
			const FGameplayTag& Rarity,
			const int32 Cost)
		{
			UCardDefinition* Card = NewObject<UCardDefinition>(Session);
			Card->CardId = CardId;
			Card->UpgradeFamilyId = FamilyId;
			Card->Rarity = Rarity;
			Card->BaseCost = Cost;
			return Card;
		}

		FRunShopVisitRequest MakeRequest(const FName ShopId, const int32 Price = 1) const
		{
			FRunShopVisitRequest Request;
			Request.ShopId = ShopId;
			Request.CardUpgradeService.bEnabled = true;
			Request.CardUpgradeService.Prices.Add(
				{ WacomTags::Card_Rarity_White, Price });
			Request.CardUpgradeService.Prices.Add(
				{ WacomTags::Card_Rarity_Blue, Price + 1 });
			Request.CardUpgradeService.Prices.Add(
				{ WacomTags::Card_Rarity_Yellow, Price + 2 });
			return Request;
		}

		FCardInstance MakeInstance(UCardDefinition* Definition, const bool bSpecialEnabled = false) const
		{
			FCardInstance Instance;
			Instance.InstanceId = FGuid::NewGuid();
			Instance.Definition = Definition;
			Instance.bBattleEnabledInSpecialZone = bSpecialEnabled;
			return Instance;
		}
	};

	FRunShopCardUpgradeCommand MakeCommand(
		const FCardInstance& Instance,
		UCardDefinition* ExpectedNext)
	{
		FRunShopCardUpgradeCommand Command;
		Command.InstanceId = Instance.InstanceId;
		Command.ExpectedCurrentDefinition = Instance.Definition;
		Command.ExpectedNextDefinition = ExpectedNext;
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopCardUpgradeExactInstanceAndZonesSpec,
	"Wacom.Run.Shop.CardUpgrade.ExactInstanceAndAllOwnedZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopCardUpgradeExactInstanceAndZonesSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunShopCardUpgradeSpec;
	FShopCardUpgradeFixture Fixture;
	UCardDefinition* White = Fixture.MakeCard(
		TEXT("Upgrade.Zone.White"), TEXT("Upgrade.Zone"), WacomTags::Card_Rarity_White, 2);
	UCardDefinition* Blue = Fixture.MakeCard(
		TEXT("Upgrade.Zone.Blue"), TEXT("Upgrade.Zone"), WacomTags::Card_Rarity_Blue, 1);
	White->NextUpgradeDefinition = Blue;

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session);
	const FCardInstance UntargetedDuplicate = Fixture.MakeInstance(White);
	const FCardInstance BackpackCard = Fixture.MakeInstance(White);
	const FCardInstance BattleDeckCard = Fixture.MakeInstance(White);
	const FCardInstance BurdenCard = Fixture.MakeInstance(White);
	const FCardInstance SpecialCard = Fixture.MakeInstance(White, true);
	State.Backpack = { UntargetedDuplicate, BackpackCard };
	State.BattleDeck = { BattleDeckCard };
	State.BurdenZone = { BurdenCard };
	FSpecialZone SpecialZone;
	SpecialZone.OwnerInstanceId = FGuid::NewGuid();
	SpecialZone.Cards = { SpecialCard };
	State.SpecialZones = { SpecialZone };
	Fixture.Session->AddGold(10);

	const FRunShopVisitResult Begin = Fixture.Session->BeginShopVisitWithResult(
		Fixture.MakeRequest(TEXT("Shop.Upgrade.Zones")));
	TestTrue(TEXT("Canonical upgrade visit begins"), Begin.bSucceeded);
	const FRunShopSnapshot OpenSnapshot = Fixture.Session->BuildCurrentShopSnapshot();
	TestEqual(TEXT("Snapshot quotes every physical card"), OpenSnapshot.CardUpgradeQuotes.Num(), 5);

	int32 BroadcastCount = 0;
	const FDelegateHandle Handle = Fixture.Session->OnRunStateChangedNative.AddLambda(
		[&BroadcastCount]() { ++BroadcastCount; });
	const uint64 StorageRevisionBefore = Fixture.Session->GetBackpackStorageSnapshotRevision();
	const uint64 ShopRevisionBefore = Fixture.Session->GetShopSnapshotRevision();
	const uint64 EconomyRevisionBefore = Fixture.Session->GetEconomySnapshotRevision();

	const TArray<FCardInstance> Targets = { BackpackCard, BattleDeckCard, BurdenCard, SpecialCard };
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const FRunShopCardUpgradeResult Result = Fixture.Session->UpgradeOwnedCardAtShop(
			MakeCommand(Targets[Index], Blue));
		TestTrue(FString::Printf(TEXT("Target %d upgrades"), Index), Result.bSucceeded);
		TestEqual(FString::Printf(TEXT("Target %d keeps id"), Index), Result.InstanceId, Targets[Index].InstanceId);
		TestEqual(FString::Printf(TEXT("Target %d uses next definition"), Index), Result.NewDefinition.Get(), Blue);
		TestEqual(FString::Printf(TEXT("Only first target costs AP %d"), Index), Result.ActionPointCost, Index == 0 ? 1 : 0);
	}

	const FRunState& After = Fixture.Session->GetRunState();
	TestEqual(TEXT("Same-definition untargeted instance is untouched"), After.Backpack[0].Definition.Get(), White);
	TestEqual(TEXT("Backpack target keeps index"), After.Backpack[1].Definition.Get(), Blue);
	TestEqual(TEXT("BattleDeck target keeps zone"), After.BattleDeck[0].Definition.Get(), Blue);
	TestEqual(TEXT("Burden target keeps zone"), After.BurdenZone[0].Definition.Get(), Blue);
	TestEqual(TEXT("Special target keeps zone"), After.SpecialZones[0].Cards[0].Definition.Get(), Blue);
	TestTrue(TEXT("Special-zone battle flag is preserved"), After.SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);
	TestEqual(TEXT("Four upgrades spend four gold"), After.Gold, 6);
	TestEqual(TEXT("Each upgrade broadcasts exactly once"), BroadcastCount, 4);
	TestEqual(TEXT("Storage revision advances once per upgrade"), Fixture.Session->GetBackpackStorageSnapshotRevision(), StorageRevisionBefore + 4);
	TestEqual(TEXT("Shop revision advances once per upgrade"), Fixture.Session->GetShopSnapshotRevision(), ShopRevisionBefore + 4);
	TestEqual(TEXT("Economy revision advances once per upgrade"), Fixture.Session->GetEconomySnapshotRevision(), EconomyRevisionBefore + 4);
	Fixture.Session->OnRunStateChangedNative.Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopCardUpgradeChainAndRollbackSpec,
	"Wacom.Run.Shop.CardUpgrade.ChainStaleAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopCardUpgradeChainAndRollbackSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunShopCardUpgradeSpec;
	FShopCardUpgradeFixture Fixture;
	UCardDefinition* White = Fixture.MakeCard(TEXT("Upgrade.Chain.White"), TEXT("Upgrade.Chain"), WacomTags::Card_Rarity_White, 3);
	UCardDefinition* Blue = Fixture.MakeCard(TEXT("Upgrade.Chain.Blue"), TEXT("Upgrade.Chain"), WacomTags::Card_Rarity_Blue, 2);
	UCardDefinition* Yellow = Fixture.MakeCard(TEXT("Upgrade.Chain.Yellow"), TEXT("Upgrade.Chain"), WacomTags::Card_Rarity_Yellow, 1);
	UCardDefinition* Purple = Fixture.MakeCard(TEXT("Upgrade.Chain.Purple"), TEXT("Upgrade.Chain"), WacomTags::Card_Rarity_Purple, 0);
	White->NextUpgradeDefinition = Blue;
	Blue->NextUpgradeDefinition = Yellow;
	Yellow->NextUpgradeDefinition = Purple;

	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session);
	const FCardInstance Instance = Fixture.MakeInstance(White);
	State.Backpack = { Instance };
	Fixture.Session->AddGold(10);
	TestTrue(TEXT("Upgrade chain shop begins"), Fixture.Session->BeginShopVisitRequest(
		Fixture.MakeRequest(TEXT("Shop.Upgrade.Chain"))));

	int32 BroadcastCount = 0;
	const FDelegateHandle Handle = Fixture.Session->OnRunStateChangedNative.AddLambda(
		[&BroadcastCount]() { ++BroadcastCount; });
	const FRunShopCardUpgradeCommand WhiteToBlue = MakeCommand(Instance, Blue);
	TestTrue(TEXT("White to Blue succeeds"), Fixture.Session->UpgradeOwnedCardAtShop(WhiteToBlue).bSucceeded);
	const int32 GoldAfterFirst = Fixture.Session->GetGold();
	const int32 VersionAfterFirst = Fixture.Session->BuildExplorationSnapshot().StateVersion;
	TestFalse(TEXT("Repeated stale request fails"), Fixture.Session->UpgradeOwnedCardAtShop(WhiteToBlue).bSucceeded);
	TestEqual(TEXT("Stale request keeps gold"), Fixture.Session->GetGold(), GoldAfterFirst);
	TestEqual(TEXT("Stale request keeps version"), Fixture.Session->BuildExplorationSnapshot().StateVersion, VersionAfterFirst);
	TestEqual(TEXT("Stale request does not broadcast"), BroadcastCount, 1);

	FCardInstance Current = Fixture.Session->GetRunState().Backpack[0];
	TestTrue(TEXT("Blue to Yellow succeeds"), Fixture.Session->UpgradeOwnedCardAtShop(MakeCommand(Current, Yellow)).bSucceeded);
	Current = Fixture.Session->GetRunState().Backpack[0];
	TestTrue(TEXT("Yellow to Purple succeeds"), Fixture.Session->UpgradeOwnedCardAtShop(MakeCommand(Current, Purple)).bSucceeded);
	Current = Fixture.Session->GetRunState().Backpack[0];
	const FRunShopCardUpgradeCommand PurpleCommand = MakeCommand(Current, nullptr);
	const int32 GoldBeforePurpleRetry = Fixture.Session->GetGold();
	TestFalse(TEXT("Purple cannot upgrade"), Fixture.Session->UpgradeOwnedCardAtShop(PurpleCommand).bSucceeded);
	TestEqual(TEXT("Purple rejection keeps gold"), Fixture.Session->GetGold(), GoldBeforePurpleRetry);
	TestEqual(TEXT("Only three successes broadcast"), BroadcastCount, 3);

	FRunShopCardUpgradeCommand Unknown;
	Unknown.InstanceId = FGuid::NewGuid();
	Unknown.ExpectedCurrentDefinition = White;
	Unknown.ExpectedNextDefinition = Blue;
	TestFalse(TEXT("Non-owned instance rejects"), Fixture.Session->UpgradeOwnedCardAtShop(Unknown).bSucceeded);
	TestEqual(TEXT("Non-owned rejection remains silent"), BroadcastCount, 3);
	Fixture.Session->OnRunStateChangedNative.Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopCardUpgradeCommerceMatrixSpec,
	"Wacom.Run.Shop.CardUpgrade.PurchaseUpgradeActionPointMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopCardUpgradeCommerceMatrixSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunShopCardUpgradeSpec;
	auto Configure = [](FShopCardUpgradeFixture& Fixture, const FName Suffix)
	{
		UCardDefinition* White = Fixture.MakeCard(*FString::Printf(TEXT("Upgrade.Matrix.%s.White"), *Suffix.ToString()), TEXT("Upgrade.Matrix"), WacomTags::Card_Rarity_White, 2);
		UCardDefinition* Blue = Fixture.MakeCard(*FString::Printf(TEXT("Upgrade.Matrix.%s.Blue"), *Suffix.ToString()), TEXT("Upgrade.Matrix"), WacomTags::Card_Rarity_Blue, 1);
		White->NextUpgradeDefinition = Blue;
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session);
		const FCardInstance Instance = Fixture.MakeInstance(White);
		State.Backpack = { Instance };
		Fixture.Session->AddGold(10);
		FRunShopVisitRequest Request = Fixture.MakeRequest(*FString::Printf(TEXT("Shop.Matrix.%s"), *Suffix.ToString()));
		FRunShopOfferInput Offer;
		Offer.CardDefinition = Blue;
		Offer.Price = 1;
		Request.Offers = { Offer };
		return TTuple<FRunShopVisitRequest, FCardInstance, UCardDefinition*>(Request, Instance, Blue);
	};

	{
		FShopCardUpgradeFixture Fixture;
		auto [Request, Instance, Blue] = Configure(Fixture, TEXT("UpgradeFirst"));
		TestTrue(TEXT("Upgrade-first visit begins"), Fixture.Session->BeginShopVisitRequest(Request));
		const FGuid OfferId = Fixture.Session->BuildCurrentShopSnapshot().Offers[0].OfferId;
		const FRunShopCardUpgradeResult Upgrade = Fixture.Session->UpgradeOwnedCardAtShop(MakeCommand(Instance, Blue));
		const FRunShopPurchaseResult Purchase = Fixture.Session->PurchaseShopOffer(OfferId);
		TestEqual(TEXT("Upgrade-first costs one AP"), Upgrade.ActionPointCost, 1);
		TestEqual(TEXT("Purchase after upgrade costs zero AP"), Purchase.ActionPointCost, 0);
	}

	{
		FShopCardUpgradeFixture Fixture;
		auto [Request, Instance, Blue] = Configure(Fixture, TEXT("PurchaseFirst"));
		TestTrue(TEXT("Purchase-first visit begins"), Fixture.Session->BeginShopVisitRequest(Request));
		const FGuid OfferId = Fixture.Session->BuildCurrentShopSnapshot().Offers[0].OfferId;
		const FRunShopPurchaseResult Purchase = Fixture.Session->PurchaseShopOffer(OfferId);
		const FRunShopCardUpgradeResult Upgrade = Fixture.Session->UpgradeOwnedCardAtShop(MakeCommand(Instance, Blue));
		TestEqual(TEXT("Purchase-first costs one AP"), Purchase.ActionPointCost, 1);
		TestEqual(TEXT("Upgrade after purchase costs zero AP"), Upgrade.ActionPointCost, 0);
	}

	{
		FShopCardUpgradeFixture Fixture;
		auto [Request, Instance, Blue] = Configure(Fixture, TEXT("PhaseAdvance"));
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session);
		State.TimeState.CurrentTimePhase = ETimePhase::Morning;
		State.TimeState.RemainingActionPoints = 1;
		const FRunShopVisitResult Begin =
			Fixture.Session->BeginShopVisitWithResult(Request);
		TestTrue(TEXT("Phase-advance visit begins"), Begin.bSucceeded);
		const FRunShopCardUpgradeResult Upgrade = Fixture.Session->UpgradeOwnedCardAtShop(MakeCommand(Instance, Blue));
		TestTrue(TEXT("Phase-ending upgrade succeeds"), Upgrade.bSucceeded);
		TestFalse(
			TEXT("Phase-ending upgrade defers visit close"),
			Upgrade.bVisitClosedAfterUpgrade);
		TestTrue(TEXT("Visit stays active at zero AP"), Fixture.Session->IsShopVisitActive());
		TestEqual(
			TEXT("Morning remains active inside shop"),
			Fixture.Session->BuildExplorationSnapshot().Time.CurrentTimePhase,
			ETimePhase::Morning);
		TestEqual(
			TEXT("Upgrade consumes the last AP"),
			Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
			0);
		TestTrue(
			TEXT("Owned close succeeds"),
			Fixture.Session->EndShopVisitIfOwnedWithResult(Begin.VisitToken).bSucceeded);
		TestFalse(TEXT("Owned close ends visit"), Fixture.Session->IsShopVisitActive());
		TestEqual(
			TEXT("Leaving shop advances Morning to Day"),
			Fixture.Session->BuildExplorationSnapshot().Time.CurrentTimePhase,
			ETimePhase::Day);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopCardUpgradeRejectedStatesSpec,
	"Wacom.Run.Shop.CardUpgrade.RejectedStatesAreSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopCardUpgradeRejectedStatesSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunShopCardUpgradeSpec;
	auto MakeEligibleCard = [](FShopCardUpgradeFixture& Fixture)
	{
		UCardDefinition* White = Fixture.MakeCard(
			TEXT("Upgrade.Rejected.White"), TEXT("Upgrade.Rejected"), WacomTags::Card_Rarity_White, 2);
		UCardDefinition* Blue = Fixture.MakeCard(
			TEXT("Upgrade.Rejected.Blue"), TEXT("Upgrade.Rejected"), WacomTags::Card_Rarity_Blue, 1);
		White->NextUpgradeDefinition = Blue;
		const FCardInstance Instance = Fixture.MakeInstance(White);
		FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session).Backpack = { Instance };
		return TTuple<FCardInstance, UCardDefinition*>(Instance, Blue);
	};
	auto VerifySilentRejection = [this](
		FShopCardUpgradeFixture& Fixture,
		const FCardInstance& Instance,
		UCardDefinition* Next,
		const FName ExpectedReason)
	{
		int32 BroadcastCount = 0;
		const FDelegateHandle Handle = Fixture.Session->OnRunStateChangedNative.AddLambda(
			[&BroadcastCount]() { ++BroadcastCount; });
		const int32 GoldBefore = Fixture.Session->GetGold();
		const int32 VersionBefore = Fixture.Session->BuildExplorationSnapshot().StateVersion;
		const UCardDefinition* DefinitionBefore =
			Fixture.Session->GetRunState().Backpack[0].Definition;
		const FRunShopCardUpgradeResult Result = Fixture.Session->UpgradeOwnedCardAtShop(
			MakeCommand(Instance, Next));
		TestFalse(TEXT("Rejected upgrade fails"), Result.bSucceeded);
		TestEqual(TEXT("Rejected upgrade reports the expected reason"), Result.DisabledReason, ExpectedReason);
		TestEqual(TEXT("Rejected upgrade keeps gold"), Fixture.Session->GetGold(), GoldBefore);
		TestEqual(TEXT("Rejected upgrade keeps state version"), Fixture.Session->BuildExplorationSnapshot().StateVersion, VersionBefore);
		TestTrue(TEXT("Rejected upgrade keeps card definition"),
			Fixture.Session->GetRunState().Backpack[0].Definition.Get() == DefinitionBefore);
		TestEqual(TEXT("Rejected upgrade does not broadcast"), BroadcastCount, 0);
		Fixture.Session->OnRunStateChangedNative.Remove(Handle);
	};

	{
		FShopCardUpgradeFixture Fixture;
		auto [Instance, Blue] = MakeEligibleCard(Fixture);
		Fixture.Session->AddGold(10);
		VerifySilentRejection(Fixture, Instance, Blue, TEXT("ShopVisitNotActive"));
	}

	{
		FShopCardUpgradeFixture Fixture;
		auto [Instance, Blue] = MakeEligibleCard(Fixture);
		Fixture.Session->AddGold(10);
		FRunShopVisitRequest Request = Fixture.MakeRequest(TEXT("Shop.Upgrade.Disabled"));
		Request.CardUpgradeService.bEnabled = false;
		TestTrue(TEXT("Disabled-service visit still opens for ordinary purchases"),
			Fixture.Session->BeginShopVisitRequest(Request));
		TestEqual(TEXT("Disabled service quote is passive and explicit"),
			Fixture.Session->BuildCurrentShopSnapshot().CardUpgradeQuotes[0].DisabledReason,
			FName(TEXT("CardUpgradeServiceDisabled")));
		VerifySilentRejection(Fixture, Instance, Blue, TEXT("CardUpgradeServiceDisabled"));
	}

	{
		FShopCardUpgradeFixture Fixture;
		auto [Instance, Blue] = MakeEligibleCard(Fixture);
		Fixture.Session->AddGold(10);
		FRunShopVisitRequest Request;
		Request.ShopId = TEXT("Shop.Upgrade.MissingPrice");
		Request.CardUpgradeService.bEnabled = true;
		TestTrue(TEXT("Missing-price visit opens"), Fixture.Session->BeginShopVisitRequest(Request));
		TestEqual(TEXT("Missing-price quote explains rejection"),
			Fixture.Session->BuildCurrentShopSnapshot().CardUpgradeQuotes[0].DisabledReason,
			FName(TEXT("UpgradePriceMissing")));
		VerifySilentRejection(Fixture, Instance, Blue, TEXT("UpgradePriceMissing"));
	}

	{
		FShopCardUpgradeFixture Fixture;
		auto [Instance, Blue] = MakeEligibleCard(Fixture);
		Fixture.Session->AddGold(1);
		TestTrue(TEXT("Insufficient-gold visit opens"), Fixture.Session->BeginShopVisitRequest(
			Fixture.MakeRequest(TEXT("Shop.Upgrade.InsufficientGold"), 3)));
		TestEqual(TEXT("Insufficient-gold quote explains rejection"),
			Fixture.Session->BuildCurrentShopSnapshot().CardUpgradeQuotes[0].DisabledReason,
			FName(TEXT("InsufficientGold")));
		VerifySilentRejection(Fixture, Instance, Blue, TEXT("InsufficientGold"));
	}

	{
		FShopCardUpgradeFixture Fixture;
		auto [Instance, Blue] = MakeEligibleCard(Fixture);
		Fixture.Session->AddGold(10);
		TestTrue(TEXT("Terminal-state fixture opens before ending the Run"),
			Fixture.Session->BeginShopVisitRequest(Fixture.MakeRequest(TEXT("Shop.Upgrade.Terminal"))));
		FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Session).Outcome = ERunOutcome::Failed;
		VerifySilentRejection(Fixture, Instance, Blue, TEXT("RunNotActive"));
	}

	return true;
}
