// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"

namespace
{
	struct FTreasureActivityFixture
	{
		FWacomRunExplorationFixture Exploration;
		URunSession* Session = nullptr;

		explicit FTreasureActivityFixture(UCharacterDefinition* Character = nullptr)
		{
			UWacomFloorMapDefinition* Floor =
				Exploration.MakeLinearFloor(TEXT("Treasure.Policy.Floor"), 1);
			Floor->Nodes[0].NodeType = EWacomMapNodeType::Treasure;
			Session = Exploration.CreateInitializedSession(
				Character,
				Exploration.MakeJourney({ Floor }, TEXT("Treasure.Policy.Journey"))).Session;
		}
	};

	FGuid FindOwnedCardInstance(const FRunState& State, const UCardDefinition* Definition)
	{
		auto FindInPile = [Definition](const TArray<FCardInstance>& Pile)
		{
			const FCardInstance* Found = Pile.FindByPredicate(
				[Definition](const FCardInstance& Instance)
				{
					return Instance.Definition == Definition;
				});
			return Found ? Found->InstanceId : FGuid();
		};

		if (const FGuid Found = FindInPile(State.BattleDeck); Found.IsValid())
		{
			return Found;
		}
		if (const FGuid Found = FindInPile(State.Backpack); Found.IsValid())
		{
			return Found;
		}
		if (const FGuid Found = FindInPile(State.BurdenZone); Found.IsValid())
		{
			return Found;
		}
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			if (const FGuid Found = FindInPile(Zone.Cards); Found.IsValid())
			{
				return Found;
			}
		}
		return FGuid();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTreasureGoldAtomicSettlementTest,
	"Wacom.Run.NodeActivity.Treasure.GoldPickupCommitsRewardTimeAndLifecycleOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTreasureGoldAtomicSettlementTest::RunTest(const FString& /*Parameters*/)
{
	FTreasureActivityFixture Fixture;
	const FRunExplorationSnapshot Before = Fixture.Session->BuildExplorationSnapshot();
	const FRunTreasureSettlementResult Result =
		Fixture.Session->CollectGoldPickup(TEXT("Treasure.Policy.Gold"), 3);

	TestTrue(TEXT("Gold settlement succeeds"), Result.bSucceeded);
	TestEqual(TEXT("Treasure costs one AP"), Result.ActionPointCost, 1);
	TestTrue(TEXT("Formal exploration result succeeds"), Result.ExplorationResolution.IsOk());
	TestEqual(TEXT("Settlement commits one exploration version"),
		Result.ExplorationResolution.VersionAfter,
		Before.StateVersion + 1);
	TestEqual(TEXT("Session matches explicit post snapshot"),
		Fixture.Session->BuildExplorationSnapshot().StateVersion,
		Result.ExplorationResolution.PostSnapshot.StateVersion);
	TestEqual(TEXT("Treasure node resolves"),
		Result.ExplorationResolution.PostSnapshot.Nodes[0].Lifecycle,
		ERunMapNodeLifecycle::Resolved);
	TestEqual(TEXT("Morning exhaustion advances to Day"),
		Result.ExplorationResolution.PostSnapshot.Time.CurrentTimePhase,
		ETimePhase::Day);
	TestEqual(TEXT("Gold reward commits"), Fixture.Session->GetGold(), 3);

	const FRunExplorationSnapshot BeforeDuplicate = Fixture.Session->BuildExplorationSnapshot();
	const FRunTreasureSettlementResult Duplicate =
		Fixture.Session->CollectGoldPickup(TEXT("Treasure.Policy.Gold"), 3);
	TestFalse(TEXT("Duplicate is rejected"), Duplicate.bSucceeded);
	TestEqual(TEXT("Duplicate preserves version"),
		Fixture.Session->BuildExplorationSnapshot().StateVersion,
		BeforeDuplicate.StateVersion);
	TestEqual(TEXT("Duplicate grants no reward"), Fixture.Session->GetGold(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTreasureCardAtomicSettlementTest,
	"Wacom.Run.NodeActivity.Treasure.CardPickupCommitsCardAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTreasureCardAtomicSettlementTest::RunTest(const FString& /*Parameters*/)
{
	FTreasureActivityFixture Fixture;
	UCardDefinition* Reward = NewObject<UCardDefinition>(Fixture.Session);
	Reward->CardId = TEXT("Treasure.Policy.RewardCard");
	const FRunTreasureSettlementResult Result =
		Fixture.Session->CollectCardPickup(TEXT("Treasure.Policy.Card"), Reward);

	TestTrue(TEXT("Card settlement succeeds"), Result.bSucceeded);
	TestEqual(TEXT("Card treasure costs one AP"), Result.ActionPointCost, 1);
	TestTrue(TEXT("Formal card result succeeds"), Result.ExplorationResolution.IsOk());
	TestTrue(TEXT("Reward card enters owned state"),
		FindOwnedCardInstance(Fixture.Session->GetRunState(), Reward).IsValid());
	TestEqual(TEXT("Card treasure resolves node"),
		Fixture.Session->BuildExplorationSnapshot().Nodes[0].Lifecycle,
		ERunMapNodeLifecycle::Resolved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTreasureWorldInteractionSettlementTest,
	"Wacom.Run.NodeActivity.Treasure.WorldCardInteractionCommitsAtomically",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTreasureWorldInteractionSettlementTest::RunTest(const FString& /*Parameters*/)
{
	FWacomRunExplorationFixture CharacterFixture;
	UCardDefinition* Key = NewObject<UCardDefinition>(GetTransientPackage());
	Key->CardId = TEXT("Treasure.Policy.Key");
	UCharacterDefinition* Character = CharacterFixture.MakeCharacter(TEXT("Treasure.Policy.Character"));
	Character->StarterDeck = { Key };
	FTreasureActivityFixture Fixture(Character);

	FRunWorldCardInteractionRequest Request;
	Request.PersistentId = TEXT("Treasure.Policy.WorldInteraction");
	Request.SourceCardInstanceId = FindOwnedCardInstance(Fixture.Session->GetRunState(), Key);
	Request.AllowedCardDefinitions = { Key };
	Request.bConsumeCardOnSuccess = false;
	FWacomRunWorldCardInteractionReward& Reward = Request.Rewards.AddDefaulted_GetRef();
	Reward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
	Reward.GoldAmount = 4;

	const FRunExplorationSnapshot Before = Fixture.Session->BuildExplorationSnapshot();
	const FRunTreasureSettlementResult Result =
		Fixture.Session->SubmitRunWorldCardInteraction(Request);
	TestTrue(TEXT("World interaction succeeds"), Result.bSucceeded);
	TestEqual(TEXT("World interaction costs one AP"), Result.ActionPointCost, 1);
	TestEqual(TEXT("World interaction commits one exploration version"),
		Fixture.Session->BuildExplorationSnapshot().StateVersion,
		Before.StateVersion + 1);
	TestEqual(TEXT("World interaction grants reward"), Fixture.Session->GetGold(), 4);
	TestTrue(TEXT("World interaction is completed"),
		Fixture.Session->IsRunWorldInteractionCompleted(Request.PersistentId));

	const FRunExplorationSnapshot BeforeDuplicate = Fixture.Session->BuildExplorationSnapshot();
	const FRunTreasureSettlementResult Duplicate =
		Fixture.Session->SubmitRunWorldCardInteraction(Request);
	TestFalse(TEXT("Duplicate world interaction is rejected"), Duplicate.bSucceeded);
	TestEqual(TEXT("Rejected duplicate preserves version"),
		Fixture.Session->BuildExplorationSnapshot().StateVersion,
		BeforeDuplicate.StateVersion);
	TestEqual(TEXT("Rejected duplicate preserves reward"), Fixture.Session->GetGold(), 4);
	return true;
}

#endif
