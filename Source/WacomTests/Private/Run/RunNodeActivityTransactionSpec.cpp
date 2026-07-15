// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"
#include "Testing/WacomRunNodeActivityAutomationTestView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNodeActivityLifecycleTest,
	"Wacom.Run.NodeActivity.Transaction.ReservationCancelAndAtomicComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNodeActivityLifecycleTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Activity.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Encounter;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Fixture.MakeJourney({ Floor }));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	TOptional<FRunNodeActivityTicket> Pending;
	TArray<FRunExplorationEvent> Events;
	FRunNodeActivityTicket FirstTicket;
	const int32 InitialActionPoints = State.TimeState.RemainingActionPoints;
	const int32 InitialVersion = State.ExplorationState.ExplorationStateVersion;

	TestTrue(TEXT("Encounter begin reserves one point"),
		FWacomRunNodeActivityAutomationTestView::Begin(
			State, Pending, ERunNodeActivityKind::Encounter, 1, FirstTicket, Events).IsOk());
	TestEqual(TEXT("Reservation does not spend before settlement"),
		State.TimeState.RemainingActionPoints, InitialActionPoints);
	TestEqual(TEXT("Begin advances exploration version"),
		State.ExplorationState.ExplorationStateVersion, InitialVersion + 1);
	TestEqual(TEXT("Begin owns the activity gate"),
		State.ExplorationState.ActiveActivityKind, ERunExplorationActivityKind::NodeActivity);

	FRunNodeActivityTicket RejectedTicket;
	Events.Reset();
	TestFalse(TEXT("Second activity is rejected while active"),
		FWacomRunNodeActivityAutomationTestView::Begin(
			State, Pending, ERunNodeActivityKind::Encounter, 1, RejectedTicket, Events).IsOk());
	TestTrue(TEXT("Rejected overlap emits no events"), Events.IsEmpty());

	TestTrue(TEXT("Cancel releases reservation"),
		FWacomRunNodeActivityAutomationTestView::Cancel(
			State, Pending, FirstTicket, Events).IsOk());
	TestEqual(TEXT("Cancel preserves AP"), State.TimeState.RemainingActionPoints, InitialActionPoints);
	TestFalse(TEXT("Duplicate cancel is rejected"),
		FWacomRunNodeActivityAutomationTestView::Cancel(
			State, Pending, FirstTicket, Events).IsOk());

	Events.Reset();
	FRunNodeActivityTicket CompletionTicket;
	TestTrue(TEXT("A new activity can begin after cancel"),
		FWacomRunNodeActivityAutomationTestView::Begin(
			State, Pending, ERunNodeActivityKind::Encounter, 1, CompletionTicket, Events).IsOk());
	Events.Reset();
	TestTrue(TEXT("Completion commits reward boundary, AP, and lifecycle together"),
		FWacomRunNodeActivityAutomationTestView::Complete(
			State, Pending, CompletionTicket, true, true, Events).IsOk());
	TestEqual(TEXT("Spending last Morning AP advances to Day"),
		State.TimeState.CurrentTimePhase, ETimePhase::Day);
	TestEqual(TEXT("Day budget is loaded"),
		State.TimeState.RemainingActionPoints, State.TimeState.PhaseBudgets.Day);
	TestEqual(TEXT("Node becomes resolved"),
		State.ExplorationState.FloorProgress[0].Nodes[0].Lifecycle,
		ERunMapNodeLifecycle::Resolved);
	TestFalse(TEXT("Duplicate completion is rejected"),
		FWacomRunNodeActivityAutomationTestView::Complete(
			State, Pending, CompletionTicket, true, true, Events).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunNodeActivityInsufficientActionPointsTest,
	"Wacom.Run.NodeActivity.Transaction.InsufficientReservationHasNoSideEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunNodeActivityInsufficientActionPointsTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Activity.Floor"), 1);
	Floor->Nodes[0].NodeType = EWacomMapNodeType::Treasure;
	const FWacomInitializedRunExplorationSession Initialized =
		Fixture.CreateInitializedSession(nullptr, Fixture.MakeJourney({ Floor }));
	FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	const FRunState Before = State;
	TOptional<FRunNodeActivityTicket> Pending;
	TArray<FRunExplorationEvent> Events;
	FRunNodeActivityTicket Ticket;
	TestFalse(TEXT("Reservation above available AP is rejected"),
		FWacomRunNodeActivityAutomationTestView::Begin(
			State, Pending, ERunNodeActivityKind::Treasure, 2, Ticket, Events).IsOk());
	TestEqual(TEXT("Version is unchanged"),
		State.ExplorationState.ExplorationStateVersion,
		Before.ExplorationState.ExplorationStateVersion);
	TestEqual(TEXT("AP is unchanged"),
		State.TimeState.RemainingActionPoints, Before.TimeState.RemainingActionPoints);
	TestEqual(TEXT("Lifecycle is unchanged"),
		State.ExplorationState.FloorProgress[0].Nodes[0].Lifecycle,
		Before.ExplorationState.FloorProgress[0].Nodes[0].Lifecycle);
	TestFalse(TEXT("No pending ticket is retained"), Pending.IsSet());
	TestTrue(TEXT("No events are emitted"), Events.IsEmpty());
	return true;
}

#endif
