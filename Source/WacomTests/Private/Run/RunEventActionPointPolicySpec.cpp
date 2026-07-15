// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Events/RunEventDefinition.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "RunState.h"

namespace
{
	struct FRunEventActivityFixture
	{
		FWacomRunExplorationFixture Exploration;
		URunSession* Session = nullptr;

		FRunEventActivityFixture()
		{
			UWacomFloorMapDefinition* Floor =
				Exploration.MakeLinearFloor(TEXT("Event.Policy.Floor"), 1);
			Floor->Nodes[0].NodeType = EWacomMapNodeType::RunEvent;
			Session = Exploration.CreateInitializedSession(
				nullptr,
				Exploration.MakeJourney({ Floor }, TEXT("Event.Policy.Journey"))).Session;
		}

		UWacomRunEventDefinition* MakeEvent(
			const FWacomRunEventChoiceDefinition& Choice,
			FName EventId = TEXT("Event.Policy")) const
		{
			UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Session);
			Event->EventId = EventId;
			Event->StartNodeId = TEXT("Start");
			FWacomRunEventNodeDefinition Node;
			Node.NodeId = TEXT("Start");
			Node.Choices.Add(Choice);
			Event->Nodes.Add(Node);
			return Event;
		}
	};

	FWacomRunEventChoiceDefinition MakeTerminalChoice(
		EWacomRunEventActionPointPolicy Policy,
		int32 FixedCost = 1)
	{
		FWacomRunEventChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("Resolve");
		Choice.ActionPointPolicy = Policy;
		Choice.FixedActionPointCost = FixedCost;
		Choice.bCloseEventAfterResolve = true;
		return Choice;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventAutomaticAndFreeActionPointPolicyTest,
	"Wacom.Run.NodeActivity.RunEvent.AutomaticAndFreePolicies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventAutomaticAndFreeActionPointPolicyTest::RunTest(const FString& /*Parameters*/)
{
	{
		FRunEventActivityFixture Fixture;
		UWacomRunEventDefinition* Event = Fixture.MakeEvent(
			MakeTerminalChoice(EWacomRunEventActionPointPolicy::Automatic),
			TEXT("Event.Policy.Automatic"));
		TestTrue(TEXT("Automatic event begins"),
			Fixture.Session->BeginRunEvent(TEXT("Event.Policy.Automatic.Actor"), Event));
		const FRunEventSnapshot EventSnapshot = Fixture.Session->BuildCurrentRunEventSnapshot();
		TestEqual(TEXT("Automatic terminal preview costs one"),
			EventSnapshot.Choices[0].ActionPointCost,
			1);

		const FRunEventChoiceResult Result =
			Fixture.Session->ChooseRunEventOptionWithResult(TEXT("Resolve"));
		TestTrue(TEXT("Automatic terminal succeeds"), Result.bSucceeded);
		TestEqual(TEXT("Automatic terminal spends one"), Result.ActionPointCost, 1);
		TestTrue(TEXT("Automatic settlement returns exploration result"),
			Result.ExplorationResolution.IsOk());
		const FRunExplorationSnapshot After = Fixture.Session->BuildExplorationSnapshot();
		TestEqual(TEXT("Exhausting Morning AP advances to Day"),
			After.Time.CurrentTimePhase,
			ETimePhase::Day);
		TestEqual(TEXT("Day receives its phase budget"),
			After.Time.RemainingActionPoints,
			After.Time.PhaseBudgets.Day);
		TestEqual(TEXT("Automatic terminal resolves map node"),
			Fixture.Session->BuildExplorationSnapshot().Nodes[0].Lifecycle,
			ERunMapNodeLifecycle::Resolved);
	}

	{
		FRunEventActivityFixture Fixture;
		UWacomRunEventDefinition* Event = Fixture.MakeEvent(
			MakeTerminalChoice(EWacomRunEventActionPointPolicy::Free),
			TEXT("Event.Policy.Free"));
		const int32 BeforeActionPoints =
			Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints;
		TestTrue(TEXT("Free event begins"),
			Fixture.Session->BeginRunEvent(TEXT("Event.Policy.Free.Actor"), Event));
		const FRunEventChoiceResult Result =
			Fixture.Session->ChooseRunEventOptionWithResult(TEXT("Resolve"));
		TestTrue(TEXT("Free terminal succeeds"), Result.bSucceeded);
		TestEqual(TEXT("Free terminal reports zero cost"), Result.ActionPointCost, 0);
		TestEqual(TEXT("Free terminal preserves AP"),
			Fixture.Session->BuildExplorationSnapshot().Time.RemainingActionPoints,
			BeforeActionPoints);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventFixedCostRollbackTest,
	"Wacom.Run.NodeActivity.RunEvent.FixedCostFailureRollsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventFixedCostRollbackTest::RunTest(const FString& /*Parameters*/)
{
	FRunEventActivityFixture Fixture;
	FWacomRunEventChoiceDefinition Choice =
		MakeTerminalChoice(EWacomRunEventActionPointPolicy::Fixed, 2);
	FWacomRunEventEffectDefinition Gold;
	Gold.Type = EWacomRunEventEffectType::AddGold;
	Gold.Value = 5;
	Choice.Effects.Add(Gold);
	UWacomRunEventDefinition* Event = Fixture.MakeEvent(Choice, TEXT("Event.Policy.Fixed"));
	TestTrue(TEXT("Fixed event begins"),
		Fixture.Session->BeginRunEvent(TEXT("Event.Policy.Fixed.Actor"), Event));

	const FRunExplorationSnapshot Before = Fixture.Session->BuildExplorationSnapshot();
	const FRunEventChoiceResult Result =
		Fixture.Session->ChooseRunEventOptionWithResult(TEXT("Resolve"));
	TestFalse(TEXT("Insufficient fixed cost is rejected"), Result.bSucceeded);
	TestEqual(TEXT("Fixed cost failure reason"),
		Result.DisabledReason,
		FName(TEXT("InsufficientActionPoints")));
	TestEqual(TEXT("Failed fixed cost reports no committed cost"), Result.ActionPointCost, 0);
	TestEqual(TEXT("Failed fixed cost rolls back gold"), Fixture.Session->GetGold(), 0);
	TestTrue(TEXT("Failed fixed cost keeps event active"), Fixture.Session->IsRunEventActive());
	TestEqual(TEXT("Failed fixed cost preserves exploration version"),
		Fixture.Session->BuildExplorationSnapshot().StateVersion,
		Before.StateVersion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventRequirementAndTerminalContractTest,
	"Wacom.Run.NodeActivity.RunEvent.RequirementAndTerminalContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventRequirementAndTerminalContractTest::RunTest(const FString& /*Parameters*/)
{
	{
		FRunEventActivityFixture Fixture;
		FWacomRunEventChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("Locked");
		FWacomRunEventConditionDefinition Requirement;
		Requirement.Type = EWacomRunEventConditionType::MinActionPoints;
		Requirement.Value = 2;
		Choice.Conditions.Add(Requirement);
		UWacomRunEventDefinition* Event = Fixture.MakeEvent(Choice, TEXT("Event.Policy.Requirement"));
		TestTrue(TEXT("Requirement event begins"),
			Fixture.Session->BeginRunEvent(TEXT("Event.Policy.Requirement.Actor"), Event));
		const FRunEventSnapshot Snapshot = Fixture.Session->BuildCurrentRunEventSnapshot();
		TestFalse(TEXT("MinActionPoints disables choice"), Snapshot.Choices[0].bAvailable);
		TestEqual(TEXT("MinActionPoints reason"),
			Snapshot.Choices[0].DisabledReason,
			FName(TEXT("InsufficientActionPoints")));
	}

	{
		FRunEventActivityFixture Fixture;
		FWacomRunEventChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("InvalidPaidNonTerminal");
		Choice.ActionPointPolicy = EWacomRunEventActionPointPolicy::Fixed;
		Choice.FixedActionPointCost = 1;
		FWacomRunEventEffectDefinition Gold;
		Gold.Type = EWacomRunEventEffectType::AddGold;
		Gold.Value = 5;
		Choice.Effects.Add(Gold);
		UWacomRunEventDefinition* Event = Fixture.MakeEvent(Choice, TEXT("Event.Policy.NonTerminal"));
		TestTrue(TEXT("Non-terminal event begins"),
			Fixture.Session->BeginRunEvent(TEXT("Event.Policy.NonTerminal.Actor"), Event));
		const FRunEventChoiceResult Result =
			Fixture.Session->ChooseRunEventOptionWithResult(TEXT("InvalidPaidNonTerminal"));
		TestFalse(TEXT("Positive non-terminal cost is rejected"), Result.bSucceeded);
		TestEqual(TEXT("Positive non-terminal rejection reason"),
			Result.DisabledReason,
			FName(TEXT("PositiveActionPointCostRequiresTerminalChoice")));
		TestEqual(TEXT("Positive non-terminal rollback preserves gold"), Fixture.Session->GetGold(), 0);
	}

	return true;
}
