// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "Testing/WacomRunTimeAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunActionPointAtomicSpendTest,
	"Wacom.Run.Time.ActionPoints.AtomicSpendAndPhaseReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunActionPointAtomicSpendTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	TestEqual(TEXT("Morning Planning occupies one point"), State.TimeState.RemainingActionPoints, 1);
	const FRunTimeState BeforeRejected = State.TimeState;
	const int32 PressureBeforeRejected = State.Pressure.GetTotal();
	TArray<FRunExplorationEvent> Events;
	const FWacomStatus Rejected = FWacomRunTimeAutomationTestView::TrySpendActionPoints(
		State, 2, Events);
	TestFalse(TEXT("Cost above remaining AP is rejected"), Rejected.IsOk());
	TestEqual(TEXT("Rejected spend preserves AP"),
		State.TimeState.RemainingActionPoints, BeforeRejected.RemainingActionPoints);
	TestEqual(TEXT("Rejected spend preserves phase"),
		State.TimeState.CurrentTimePhase, BeforeRejected.CurrentTimePhase);
	TestEqual(TEXT("Rejected spend preserves pressure"),
		State.Pressure.GetTotal(), PressureBeforeRejected);
	TestTrue(TEXT("Rejected spend produces no events"), Events.IsEmpty());

	const FWacomStatus ExactSpend = FWacomRunTimeAutomationTestView::TrySpendActionPoints(
		State, 1, Events);
	TestTrue(TEXT("Exact remaining AP succeeds"), ExactSpend.IsOk());
	TestEqual(TEXT("Exact spend advances once to Day"),
		State.TimeState.CurrentTimePhase, ETimePhase::Day);
	TestEqual(TEXT("Day loads its configured budget"),
		State.TimeState.RemainingActionPoints, State.TimeState.PhaseBudgets.Day);
	TestEqual(TEXT("Exact spend emits one phase event"), Events.Num(), 1);

	Events.Reset();
	const FWacomStatus DaySpend = FWacomRunTimeAutomationTestView::TrySpendActionPoints(
		State, State.TimeState.PhaseBudgets.Day, Events);
	TestTrue(TEXT("Whole Day budget succeeds atomically"), DaySpend.IsOk());
	TestEqual(TEXT("Day exhaustion advances to Dusk"),
		State.TimeState.CurrentTimePhase, ETimePhase::Dusk);
	TestEqual(TEXT("Dusk budget resets"),
		State.TimeState.RemainingActionPoints, State.TimeState.PhaseBudgets.Dusk);
	return true;
}

#endif
