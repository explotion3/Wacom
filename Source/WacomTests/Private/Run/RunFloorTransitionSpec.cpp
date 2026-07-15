// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"

namespace
{
	enum class ETestOwnedZone : uint8
	{
		Backpack,
		BattleDeck,
		Burden,
		Special,
	};

	struct FFloorTransitionFixture
	{
		FWacomRunExplorationFixture Fixture;
		UWacomFloorMapDefinition* FirstFloor = nullptr;
		UWacomFloorMapDefinition* SecondFloor = nullptr;
		UCardDefinition* KeyCard = nullptr;
		URunSession* Session = nullptr;

		explicit FFloorTransitionFixture(const bool bGiveKey, const ETestOwnedZone Zone = ETestOwnedZone::BattleDeck)
		{
			FirstFloor = Fixture.MakeLinearFloor(TEXT("Floor.Policy.01"), 2);
			SecondFloor = Fixture.MakeLinearFloor(TEXT("Floor.Policy.02"), 2);
			FWacomMapNodeDefinition& Entrance = FirstFloor->Nodes[1];
			Entrance.NodeType = EWacomMapNodeType::FloorEntrance;
			Entrance.Content.FloorEntrance.TargetFloorId = SecondFloor->FloorId;

			KeyCard = NewObject<UCardDefinition>(GetTransientPackage());
			KeyCard->CardId = TEXT("Floor.Policy.Key");
			FWacomOwnedCardRequirement& Requirement =
				Entrance.Content.FloorEntrance.OwnedCardRequirements.AddDefaulted_GetRef();
			Requirement.AllowedCardIds.Add(KeyCard->CardId);

			Session = Fixture.CreateInitializedSession(
				nullptr,
				Fixture.MakeJourney(
					{ FirstFloor, SecondFloor },
					TEXT("Floor.Policy.Journey"))).Session;
			FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Session);
			State.ExplorationState.CurrentNodeId = Entrance.NodeId;
			State.ExplorationState.FloorProgress[0].Nodes[1].Lifecycle = ERunMapNodeLifecycle::Visited;
			State.TimeState.CurrentTimePhase = ETimePhase::Day;
			State.TimeState.RemainingActionPoints = 4;
			State.Pressure.Add(EWacomPressureType::Hunger, 7);

			if (bGiveKey)
			{
				FCardInstance Card;
				Card.InstanceId = FGuid::NewGuid();
				Card.Definition = KeyCard;
				switch (Zone)
				{
				case ETestOwnedZone::Backpack:
					State.Backpack.Add(Card);
					break;
				case ETestOwnedZone::BattleDeck:
					State.BattleDeck.Add(Card);
					break;
				case ETestOwnedZone::Burden:
					State.BurdenZone.Add(Card);
					break;
				case ETestOwnedZone::Special:
					State.SpecialZones.AddDefaulted_GetRef().Cards.Add(Card);
					break;
				}
			}
		}

		int32 CountKeyCards() const
		{
			const FRunState& State = Session->GetRunState();
			auto CountPile = [this](const TArray<FCardInstance>& Pile)
			{
				int32 Count = 0;
				for (const FCardInstance& Card : Pile)
				{
					Count += Card.Definition == KeyCard ? 1 : 0;
				}
				return Count;
			};
			int32 Count = CountPile(State.Backpack) + CountPile(State.BattleDeck)
				+ CountPile(State.BurdenZone);
			for (const FSpecialZone& Special : State.SpecialZones)
			{
				Count += CountPile(Special.Cards);
			}
			return Count;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorTransitionOwnedZonesTest,
	"Wacom.Run.FloorTransition.AllOwnedZonesSatisfyWithoutConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorTransitionOwnedZonesTest::RunTest(const FString& /*Parameters*/)
{
	const TArray<ETestOwnedZone> Zones{
		ETestOwnedZone::Backpack,
		ETestOwnedZone::BattleDeck,
		ETestOwnedZone::Burden,
		ETestOwnedZone::Special,
	};
	for (const ETestOwnedZone Zone : Zones)
	{
		FFloorTransitionFixture Floor(true, Zone);
		const FRunExplorationSnapshot Before = Floor.Session->BuildExplorationSnapshot();
		TestTrue(TEXT("Entrance exposes preview"), Before.bHasFloorTransitionPreview);
		TestTrue(TEXT("Owned card satisfies preview"), Before.FloorTransitionPreview.bRequirementsMet);
		const int32 CardCountBefore = Floor.CountKeyCards();
		const FRunExplorationResolution Request = Floor.Session->ResolveExplorationCommand(
			FRunExplorationCommand::RequestFloorTransition(Before.StateVersion));
		TestTrue(TEXT("Transition request succeeds in each physical zone"), Request.IsOk());
		TestTrue(TEXT("Request returns confirmation"), Request.FloorTransitionConfirmation.IsSet());
		const FRunExplorationResolution Confirm = Floor.Session->ResolveExplorationCommand(
			FRunExplorationCommand::ConfirmFloorTransition(
				Request.FloorTransitionConfirmation.GetValue()));
		TestTrue(TEXT("Transition confirmation succeeds"), Confirm.IsOk());
		TestEqual(TEXT("Card is never consumed"), Floor.CountKeyCards(), CardCountBefore);
		TestEqual(TEXT("Target Floor becomes current"), Confirm.PostSnapshot.CurrentNode.FloorId, Floor.SecondFloor->FloorId);
		TestEqual(TEXT("Journey time is preserved"), Confirm.PostSnapshot.Time.CurrentTimePhase, Before.Time.CurrentTimePhase);
		TestEqual(TEXT("Action points are preserved"), Confirm.PostSnapshot.Time.RemainingActionPoints, Before.Time.RemainingActionPoints);
		TestEqual(TEXT("Pressure is preserved"), Confirm.PostSnapshot.TotalPressure, Before.TotalPressure);
		TestEqual(TEXT("New Floor exposure starts today"), Confirm.PostSnapshot.FloorDay, 1);
		TestEqual(TEXT("Old and current Floor summaries remain readable"), Confirm.PostSnapshot.FloorHistory.Num(), 2);
		TestTrue(TEXT("Entrance is permanently unlocked"),
			Floor.Session->GetRunState().ExplorationState.UnlockedEntranceIds.Contains(
				{ Floor.FirstFloor->FloorId, Floor.FirstFloor->Nodes[1].NodeId }));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorTransitionRequirementAndTokenTest,
	"Wacom.Run.FloorTransition.RequirementCancelAndStaleTokenAreAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorTransitionRequirementAndTokenTest::RunTest(const FString& /*Parameters*/)
{
	FFloorTransitionFixture Missing(false);
	const FRunExplorationSnapshot MissingBefore = Missing.Session->BuildExplorationSnapshot();
	const FRunExplorationResolution Rejected = Missing.Session->ResolveExplorationCommand(
		FRunExplorationCommand::RequestFloorTransition(MissingBefore.StateVersion));
	TestFalse(TEXT("Missing requirement rejects request"), Rejected.IsOk());
	TestTrue(TEXT("Rejected request emits no events"), Rejected.Events.IsEmpty());
	TestEqual(TEXT("Rejected request preserves version"), Rejected.PostSnapshot.StateVersion, MissingBefore.StateVersion);

	FFloorTransitionFixture Floor(true);
	const FRunExplorationResolution FirstRequest = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::RequestFloorTransition(
			Floor.Session->BuildExplorationSnapshot().StateVersion));
	TestTrue(TEXT("First request succeeds"), FirstRequest.IsOk());
	const FRunExplorationResolution Cancel = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CancelFloorTransition(
			FirstRequest.FloorTransitionConfirmation.GetValue()));
	TestTrue(TEXT("Confirmation can be cancelled"), Cancel.IsOk());
	TestEqual(TEXT("Cancellation releases exclusive activity"), Cancel.PostSnapshot.ActiveActivityKind, ERunExplorationActivityKind::None);
	TestEqual(TEXT("Cancellation keeps current Floor"), Cancel.PostSnapshot.CurrentNode.FloorId, Floor.FirstFloor->FloorId);

	const FRunExplorationResolution SecondRequest = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::RequestFloorTransition(Cancel.PostSnapshot.StateVersion));
	const FRunExplorationSnapshot BeforeStale = Floor.Session->BuildExplorationSnapshot();
	const FRunExplorationResolution Stale = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::ConfirmFloorTransition(
			FirstRequest.FloorTransitionConfirmation.GetValue()));
	TestFalse(TEXT("Old confirmation is stale"), Stale.IsOk());
	TestEqual(TEXT("Stale confirmation has no side effect"), Stale.PostSnapshot.StateVersion, BeforeStale.StateVersion);
	TestEqual(TEXT("New confirmation remains active"), Stale.PostSnapshot.ActiveActivityKind, ERunExplorationActivityKind::FloorTransitionConfirmation);
	TestTrue(TEXT("Second request remains valid"), SecondRequest.FloorTransitionConfirmation.IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorTransitionNoReturnTest,
	"Wacom.Run.FloorTransition.OldFloorCannotBecomeTravelTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorTransitionNoReturnTest::RunTest(const FString& /*Parameters*/)
{
	FFloorTransitionFixture Floor(true);
	const FRunExplorationResolution Request = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::RequestFloorTransition(
			Floor.Session->BuildExplorationSnapshot().StateVersion));
	const FRunExplorationResolution Confirm = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::ConfirmFloorTransition(
			Request.FloorTransitionConfirmation.GetValue()));
	TestTrue(TEXT("Fixture reaches second Floor"), Confirm.IsOk());

	const FRunExplorationResolution ReturnAttempt = Floor.Session->ResolveExplorationCommand(
		FRunExplorationCommand::MapTravel(
			{ Floor.FirstFloor->FloorId, Floor.FirstFloor->EntryNodeId },
			Confirm.PostSnapshot.StateVersion));
	TestFalse(TEXT("MapTravel cannot return to old Floor"), ReturnAttempt.IsOk());
	TestEqual(TEXT("Failed return preserves current Floor"), ReturnAttempt.PostSnapshot.CurrentNode.FloorId, Floor.SecondFloor->FloorId);
	TestEqual(TEXT("Old Floor remains history only"), ReturnAttempt.PostSnapshot.FloorHistory.Num(), 2);
	return true;
}

#endif
