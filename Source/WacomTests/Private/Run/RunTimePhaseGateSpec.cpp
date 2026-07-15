// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "Testing/WacomRunTimeAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunTimePhaseGateTest,
	"Wacom.Run.Time.PhaseGate.NightChoiceAndSunrise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunTimePhaseGateTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	TArray<FRunExplorationEvent> Events;
	FWacomRunTimeAutomationTestView::TrySpendActionPoints(State, 1, Events);
	Events.Reset();
	FWacomRunTimeAutomationTestView::TrySpendActionPoints(
		State, State.TimeState.RemainingActionPoints, Events);
	TestEqual(TEXT("Day exhaustion enters Dusk"),
		State.TimeState.CurrentTimePhase, ETimePhase::Dusk);
	Events.Reset();
	TestTrue(TEXT("Dusk remains a normal optional-action phase"),
		FWacomRunTimeAutomationTestView::TrySpendActionPoints(
			State, State.TimeState.RemainingActionPoints, Events).IsOk());
	TestEqual(TEXT("Dusk exhaustion enters Night"),
		State.TimeState.CurrentTimePhase, ETimePhase::Night);
	TestEqual(TEXT("Night starts behind a choice gate"),
		State.TimeState.NightGate, ERunNightGate::AwaitingChoice);

	Events.Reset();
	const FWacomStatus Blocked = FWacomRunTimeAutomationTestView::TrySpendActionPoints(
		State, 1, Events);
	TestFalse(TEXT("Ordinary Night action is blocked before choice"), Blocked.IsOk());
	TestEqual(TEXT("Blocked Night spend preserves budget"),
		State.TimeState.RemainingActionPoints, State.TimeState.PhaseBudgets.Night);
	TestTrue(TEXT("Blocked Night spend emits no events"), Events.IsEmpty());

	const FRunExplorationSnapshot NightSnapshot = Initialized.Session->BuildExplorationSnapshot();
	const FRunExplorationResolution Choose = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::ChooseNightExploration(NightSnapshot.StateVersion));
	TestTrue(TEXT("Formal command opens Night Exploration"), Choose.IsOk());
	TestEqual(TEXT("Night gate is open after choice"),
		Choose.PostSnapshot.Time.NightGate, ERunNightGate::ExplorationOpen);

	Events.Reset();
	TestTrue(TEXT("Open Night budget can be spent"),
		FWacomRunTimeAutomationTestView::TrySpendActionPoints(
			State, State.TimeState.RemainingActionPoints, Events).IsOk());
	TestEqual(TEXT("Night exhaustion enters Sunrise"),
		State.TimeState.CurrentTimePhase, ETimePhase::Sunrise);
	TestEqual(TEXT("Sunrise entry adds Fatigue"),
		State.Pressure.Get(EWacomPressureType::Fatigue), 10);
	Events.Reset();
	TestTrue(TEXT("Sunrise budget completes"),
		FWacomRunTimeAutomationTestView::TrySpendActionPoints(State, 1, Events).IsOk());
	TestEqual(TEXT("Sunrise enters next Morning"),
		State.TimeState.CurrentTimePhase, ETimePhase::Morning);
	TestEqual(TEXT("New Morning increments the day"), State.TimeState.CurrentDayNumber, 2);
	TestEqual(TEXT("Morning Planning is applied again"),
		State.TimeState.RemainingActionPoints, State.TimeState.PhaseBudgets.Morning - 1);
	TestEqual(TEXT("Night gate closes outside Night"),
		State.TimeState.NightGate, ERunNightGate::Closed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCampSpecialAdvanceTest,
	"Wacom.Run.Time.PhaseGate.CampSkipsSunrise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCampSpecialAdvanceTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	TArray<FRunExplorationEvent> Events;
	FWacomRunTimeAutomationTestView::AdvanceToNextPhase(State, Events);
	FWacomRunTimeAutomationTestView::AdvanceToNextPhase(State, Events);
	FWacomRunTimeAutomationTestView::AdvanceToNextPhase(State, Events);
	TestEqual(TEXT("Fixture reaches Night"), State.TimeState.CurrentTimePhase, ETimePhase::Night);
	const int32 FatigueBeforeCamp = State.Pressure.Get(EWacomPressureType::Fatigue);
	Events.Reset();
	TestTrue(TEXT("Camp special advance succeeds with Night AP"),
		FWacomRunTimeAutomationTestView::CompleteCampAndAdvanceToMorning(State, Events).IsOk());
	TestEqual(TEXT("Camp skips directly to next Morning"),
		State.TimeState.CurrentTimePhase, ETimePhase::Morning);
	TestEqual(TEXT("Camp advances the day"), State.TimeState.CurrentDayNumber, 2);
	TestEqual(TEXT("Skipped Sunrise adds no Fatigue"),
		State.Pressure.Get(EWacomPressureType::Fatigue), FatigueBeforeCamp);
	TestEqual(TEXT("Camp discards Night remainder then loads planned Morning"),
		State.TimeState.RemainingActionPoints, State.TimeState.PhaseBudgets.Morning - 1);
	return true;
}

#endif
