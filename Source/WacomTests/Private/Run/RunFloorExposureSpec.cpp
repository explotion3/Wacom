// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"
#include "Testing/WacomRunTimeAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorExposureDailyDecayTest,
	"Wacom.Run.Time.FloorExposure.DailyDecayCurveAndIdempotency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorExposureDailyDecayTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	TestEqual(TEXT("Initial Day 1 has no Decay"),
		State.Pressure.Get(EWacomPressureType::Decay), 0);
	TestEqual(TEXT("Initial day is marked settled without applying Decay"),
		State.ExplorationState.LastDailyDecayAppliedDayNumber, 1);

	const TArray<int32> ExpectedDailyDecay = { 5, 5, 7, 10, 14, 17, 17 };
	int32 PreviousDecay = 0;
	for (int32 Day = 2; Day <= 8; ++Day)
	{
		State.TimeState.CurrentTimePhase = ETimePhase::Sunrise;
		State.TimeState.RemainingActionPoints = 1;
		TArray<FRunExplorationEvent> Events;
		TestTrue(*FString::Printf(TEXT("Day %d Morning advances"), Day),
			FWacomRunTimeAutomationTestView::AdvanceToNextPhase(State, Events).IsOk());
		const int32 CurrentDecay = State.Pressure.Get(EWacomPressureType::Decay);
		TestEqual(*FString::Printf(TEXT("Day %d daily Decay matches Base + Overstay"), Day),
			CurrentDecay - PreviousDecay, ExpectedDailyDecay[Day - 2]);
		PreviousDecay = CurrentDecay;

		Events.Reset();
		TestTrue(TEXT("Repeated daily settlement is a successful no-op"),
			FWacomRunTimeAutomationTestView::ApplyDailyDecay(State, Events).IsOk());
		TestEqual(TEXT("Repeated settlement does not add Decay"),
			State.Pressure.Get(EWacomPressureType::Decay), PreviousDecay);
		TestTrue(TEXT("Repeated settlement emits no event"), Events.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorExposureMidDayEntryTest,
	"Wacom.Run.Time.FloorExposure.MidJourneyFloorEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorExposureMidDayEntryTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	State.TimeState.CurrentDayNumber = 4;
	State.TimeState.CurrentTimePhase = ETimePhase::Sunrise;
	State.ExplorationState.FloorEnteredDayNumber = 4;
	State.ExplorationState.FloorProgress[0].EnteredDayNumber = 4;
	State.ExplorationState.LastDailyDecayAppliedDayNumber = 4;
	TArray<FRunExplorationEvent> Events;
	TestTrue(TEXT("Next Morning after a mid-journey Floor entry advances"),
		FWacomRunTimeAutomationTestView::AdvanceToNextPhase(State, Events).IsOk());
	TestEqual(TEXT("Journey reaches Day 5"), State.TimeState.CurrentDayNumber, 5);
	TestEqual(TEXT("New Floor is only on FloorDay 2"),
		Initialized.Session->BuildExplorationSnapshot().FloorDay, 2);
	TestEqual(TEXT("FloorDay 2 has Base Decay only"),
		State.Pressure.Get(EWacomPressureType::Decay), 5);
	return true;
}

#endif
