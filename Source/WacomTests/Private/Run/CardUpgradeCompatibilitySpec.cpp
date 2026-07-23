// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "WacomSaveGame.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomRunCardUpgradeCompatibilitySpec
{
	struct FCardUpgradeCompatibilityFixture
	{
		FWacomBattleFixture Battle;
		TStrongObjectPtr<URunSession> Run;
		UCharacterDefinition* Character = nullptr;

		explicit FCardUpgradeCompatibilityFixture(const EWacomMapNodeType EntryNodeType = EWacomMapNodeType::Navigation)
			: Run(NewObject<URunSession>())
		{
			Character = Battle.MakeCharacter(
				Battle.MakeNoopCard(0),
				Battle.MakeNoopCard(0),
				{});
			InitializeRunSessionForTest(*Run, Character, EntryNodeType).IsOk();
		}

		UCardDefinition* MakeCard(
			const TCHAR* Id,
			const TCHAR* Family,
			const FGameplayTag& Rarity,
			const int32 Cost)
		{
			UCardDefinition* Card = NewObject<UCardDefinition>(Run.Get());
			Card->CardId = Id;
			Card->UpgradeFamilyId = Family;
			Card->Rarity = Rarity;
			Card->BaseCost = Cost;
			return Card;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardUpgradeIdentitySaveAndBattleSpec,
	"Wacom.Run.CardUpgradeCompatibility.IdentitySaveAndBattleProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardUpgradeIdentitySaveAndBattleSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunCardUpgradeCompatibilitySpec;
	FCardUpgradeCompatibilityFixture Fixture;
	UCardDefinition* White = Fixture.MakeCard(
		TEXT("Upgrade.Compatibility.White"),
		TEXT("Upgrade.Compatibility"),
		WacomTags::Card_Rarity_White,
		3);
	UCardDefinition* Blue = Fixture.MakeCard(
		TEXT("Upgrade.Compatibility.Blue"),
		TEXT("Upgrade.Compatibility"),
		WacomTags::Card_Rarity_Blue,
		1);
	White->NextUpgradeDefinition = Blue;

	FCardInstance UpgradedInstance;
	UpgradedInstance.InstanceId = FGuid::NewGuid();
	UpgradedInstance.Definition = Blue;
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Run);
	State.Backpack.Reset();
	State.BattleDeck = { UpgradedInstance };
	State.BurdenZone.Reset();
	State.SpecialZones.Reset();

	TestTrue(TEXT("Definition matches current CardId"), Blue->MatchesCardIdOrUpgradeFamily(Blue->CardId));
	TestTrue(TEXT("Definition matches stable upgrade family"), Blue->MatchesCardIdOrUpgradeFamily(TEXT("Upgrade.Compatibility")));
	TestFalse(TEXT("Definition does not reinterpret a sibling version CardId as family"), Blue->MatchesCardIdOrUpgradeFamily(White->CardId));

	FRunCardWorkspaceRequest FamilyRequest;
	FamilyRequest.WorkspaceId = TEXT("UpgradeFamily");
	FamilyRequest.Kind = ERunCardWorkspaceKind::OwnedCardsFilter;
	FamilyRequest.AllowedCardIds = { TEXT("Upgrade.Compatibility") };
	const FRunCardWorkspaceSnapshot FamilySnapshot =
		Fixture.Run->BuildRunCardWorkspaceSnapshot(FamilyRequest);
	TestTrue(TEXT("Family workspace succeeds"), FamilySnapshot.bSucceeded);
	TestEqual(TEXT("Family workspace contains upgraded card"), FamilySnapshot.Entries.Num(), 1);

	FRunCardWorkspaceRequest ExactDefinitionRequest = FamilyRequest;
	ExactDefinitionRequest.AllowedCardIds.Reset();
	ExactDefinitionRequest.AllowedCardDefinitions = { White };
	const FRunCardWorkspaceSnapshot ExactDefinitionSnapshot =
		Fixture.Run->BuildRunCardWorkspaceSnapshot(ExactDefinitionRequest);
	TestEqual(TEXT("Exact definition whitelist remains strict"), ExactDefinitionSnapshot.Entries.Num(), 0);

	FRunWorldCardInteractionRequest WorldRequest;
	WorldRequest.PersistentId = TEXT("Upgrade.Compatibility.WorldGate");
	WorldRequest.SourceCardInstanceId = UpgradedInstance.InstanceId;
	WorldRequest.AllowedCardIds = { TEXT("Upgrade.Compatibility") };
	WorldRequest.bConsumeCardOnSuccess = false;
	FWacomRunWorldCardInteractionReward WorldReward;
	WorldReward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
	WorldReward.GoldAmount = 1;
	WorldRequest.Rewards = { WorldReward };
	TestTrue(TEXT("World interaction accepts upgrade family"),
		Fixture.Run->ValidateRunWorldCardInteraction(WorldRequest).bCanSubmit);
	WorldRequest.AllowedCardIds.Reset();
	WorldRequest.AllowedCardDefinitions = { White };
	TestFalse(TEXT("World interaction exact definition remains strict"),
		Fixture.Run->ValidateRunWorldCardInteraction(WorldRequest).bCanSubmit);

	FBattleInitParams BattleParams;
	TestTrue(TEXT("Battle params build"), Fixture.Run->BuildInitParamsForBattle(
		Fixture.Run->BuildExplorationSnapshot().CurrentNode,
		TEXT("Upgrade.Compatibility.Encounter"),
		BattleParams));
	TestEqual(TEXT("Battle receives one upgraded deck entry"), BattleParams.BattleDeckEntries.Num(), 1);
	if (BattleParams.BattleDeckEntries.Num() == 1)
	{
		TestTrue(TEXT("Battle reads upgraded immutable definition"), BattleParams.BattleDeckEntries[0].Definition.Get() == Blue);
		TestEqual(TEXT("Battle sees upgraded cost"), BattleParams.BattleDeckEntries[0].Definition->BaseCost, 1);
	}

	UWacomSaveGame* Save = Fixture.Run->BuildSaveGameFromRunState();
	TestNotNull(TEXT("Save is built"), Save);
	if (!Save)
	{
		return false;
	}
	TestEqual(TEXT("Save schema remains v5"), Save->SaveVersion, 5);
	TestEqual(TEXT("Save keeps one battle card"), Save->BattleDeck.Num(), 1);
	if (Save->BattleDeck.Num() == 1)
	{
		TestEqual(TEXT("Save keeps instance id"), Save->BattleDeck[0].InstanceId, UpgradedInstance.InstanceId);
		TestEqual(TEXT("Save stores upgraded definition path"), Save->BattleDeck[0].DefinitionAssetPath, FSoftObjectPath(Blue));
	}

	TStrongObjectPtr<URunSession> Restored(NewObject<URunSession>());
	InitializeRunSessionForTest(*Restored, Fixture.Character).IsOk();
	TestTrue(TEXT("Save v5 applies without migration"), Restored->ApplySaveGameToRunState(Save));
	TestEqual(TEXT("Restored deck contains upgraded card"), Restored->GetRunState().BattleDeck.Num(), 1);
	if (Restored->GetRunState().BattleDeck.Num() == 1)
	{
		TestEqual(TEXT("Restored instance id is stable"), Restored->GetRunState().BattleDeck[0].InstanceId, UpgradedInstance.InstanceId);
		TestEqual(TEXT("Restored definition is upgraded version"), Restored->GetRunState().BattleDeck[0].Definition.Get(), Blue);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardUpgradeDeleteValueParitySpec,
	"Wacom.Run.CardUpgradeCompatibility.DeleteValuePresentationParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardUpgradeDeleteValueParitySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunCardUpgradeCompatibilitySpec;
	FCardUpgradeCompatibilityFixture Fixture;
	struct FCase
	{
		FGameplayTag Rarity;
		int32 ExpectedValue;
	};
	const TArray<FCase> Cases = {
		{ WacomTags::Card_Rarity_White, 1 },
		{ WacomTags::Card_Rarity_Blue, 2 },
		{ WacomTags::Card_Rarity_Yellow, 3 },
		{ WacomTags::Card_Rarity_Purple, 4 },
		{ WacomTags::Card_Rarity_Intrinsic, 0 },
	};

	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		UCardDefinition* Card = Fixture.MakeCard(
			*FString::Printf(TEXT("Upgrade.Value.%d"), Index),
			TEXT("Upgrade.Value"),
			Cases[Index].Rarity,
			0);
		const int32 RunValue = URunSession::GetDeleteGoldRewardForCard(Card);
		const FWacomCardViewData ViewData = UWacomCardPresentationBuilder::BuildCardViewData(Card);
		TestEqual(FString::Printf(TEXT("Run value %d"), Index), RunValue, Cases[Index].ExpectedValue);
		TestEqual(FString::Printf(TEXT("App value %d"), Index), ViewData.Value, RunValue);
		TestEqual(FString::Printf(TEXT("App visibility %d"), Index), ViewData.bShowValue, RunValue > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCardUpgradeFamilyPaymentAndFloorGateSpec,
	"Wacom.Run.CardUpgradeCompatibility.FamilyPaymentAndFloorGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCardUpgradeFamilyPaymentAndFloorGateSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomRunCardUpgradeCompatibilitySpec;

	{
		FCardUpgradeCompatibilityFixture Fixture(EWacomMapNodeType::RunEvent);
		UCardDefinition* Blue = Fixture.MakeCard(
			TEXT("Upgrade.Payment.Blue"),
			TEXT("Upgrade.Payment"),
			WacomTags::Card_Rarity_Blue,
			1);
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = Blue;
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Fixture.Run);
		State.Backpack = { Instance };
		State.BattleDeck.Reset();

		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Fixture.Run.Get());
		Event->EventId = TEXT("Event.Upgrade.Payment");
		Event->StartNodeId = TEXT("Start");
		FWacomRunEventNodeDefinition& Node = Event->Nodes.AddDefaulted_GetRef();
		Node.NodeId = Event->StartNodeId;
		FWacomRunEventChoiceDefinition& Choice = Node.Choices.AddDefaulted_GetRef();
		Choice.ChoiceId = TEXT("Pay");
		Choice.CardPayment.bRequiresOwnedCardPayment = true;
		Choice.CardPayment.AllowedCardIds = { TEXT("Upgrade.Payment") };

		TestTrue(TEXT("Family payment event opens"),
			Fixture.Run->BeginRunEvent(TEXT("Event.Upgrade.Payment.Actor"), Event));
		TestTrue(TEXT("RunEvent payment accepts upgraded family member"),
			Fixture.Run->ValidateRunEventOptionCardPayment(TEXT("Pay"), Instance.InstanceId).bCanExecute);
	}

	{
		FWacomRunExplorationFixture Exploration;
		UWacomFloorMapDefinition* FirstFloor = Exploration.MakeLinearFloor(TEXT("Upgrade.Gate.Floor.01"), 2);
		UWacomFloorMapDefinition* SecondFloor = Exploration.MakeLinearFloor(TEXT("Upgrade.Gate.Floor.02"), 1);
		FWacomMapNodeDefinition& Entrance = FirstFloor->Nodes[1];
		Entrance.NodeType = EWacomMapNodeType::FloorEntrance;
		Entrance.Content.FloorEntrance.TargetFloorId = SecondFloor->FloorId;
		FWacomOwnedCardRequirement& Requirement =
			Entrance.Content.FloorEntrance.OwnedCardRequirements.AddDefaulted_GetRef();
		Requirement.AllowedCardIds = { TEXT("Upgrade.Gate") };

		UCardDefinition* Blue = NewObject<UCardDefinition>(FirstFloor);
		Blue->CardId = TEXT("Upgrade.Gate.Blue");
		Blue->UpgradeFamilyId = TEXT("Upgrade.Gate");
		Blue->Rarity = WacomTags::Card_Rarity_Blue;
		URunSession* Session = Exploration.CreateInitializedSession(
			nullptr,
			Exploration.MakeJourney({ FirstFloor, SecondFloor }, TEXT("Upgrade.Gate.Journey"))).Session;
		FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Session);
		State.ExplorationState.CurrentNodeId = Entrance.NodeId;
		State.ExplorationState.FloorProgress[0].Nodes[1].Lifecycle = ERunMapNodeLifecycle::Visited;
		FCardInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Definition = Blue;
		State.BattleDeck = { Instance };

		const FRunExplorationSnapshot Snapshot = Session->BuildExplorationSnapshot();
		TestTrue(TEXT("Floor entrance exposes transition preview"), Snapshot.bHasFloorTransitionPreview);
		TestTrue(TEXT("Floor entrance accepts upgraded family member"),
			Snapshot.FloorTransitionPreview.bRequirementsMet);
	}

	return true;
}
