// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "RunSession.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapTraversalTwoPhaseTest,
	"Wacom.Run.Map.Traversal.TwoPhaseCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapTraversalTwoPhaseTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	if (!TestTrue(TEXT("Fixture initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	const FRunExplorationSnapshot Initial = Initialized.Initialization.PostSnapshot;
	int32 NotificationCount = 0;
	Initialized.Session->OnRunStateChangedNative.AddLambda([&NotificationCount]()
	{
		++NotificationCount;
	});
	const FWacomMapEdgeHandle FirstEdge{ Initial.CurrentNode.FloorId, TEXT("Edge.01") };
	const FRunExplorationResolution Begin = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(FirstEdge, Initial.StateVersion));
	TestTrue(TEXT("Current outgoing edge can begin traversal"), Begin.IsOk());
	if (!Begin.IsOk() || !TestTrue(TEXT("Begin returns a traversal ticket"), Begin.TraversalTicket.IsSet()))
	{
		return false;
	}
	TestEqual(TEXT("Begin increments version once"), Begin.VersionAfter, Initial.StateVersion + 1);
	TestEqual(TEXT("Begin broadcasts exactly once"), NotificationCount, 1);
	TestEqual(TEXT("Begin keeps logical position at source"),
		Begin.PostSnapshot.CurrentNode.NodeId,
		Initial.CurrentNode.NodeId);
	TestEqual(TEXT("Begin owns the active transaction"),
		Begin.PostSnapshot.ActiveActivityKind,
		ERunExplorationActivityKind::Traversal);

	const FRunExplorationResolution Complete = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(Begin.TraversalTicket.GetValue()));
	TestTrue(TEXT("Matching ticket completes traversal"), Complete.IsOk());
	TestEqual(TEXT("Complete increments version once"), Complete.VersionAfter, Begin.VersionAfter + 1);
	TestEqual(TEXT("Complete broadcasts exactly once"), NotificationCount, 2);
	TestEqual(TEXT("Complete commits target node"),
		Complete.PostSnapshot.CurrentNode.NodeId,
		FName(TEXT("Node.02")));
	TestEqual(TEXT("Complete releases active transaction"),
		Complete.PostSnapshot.ActiveActivityKind,
		ERunExplorationActivityKind::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapTraversalRejectionAndTravelTest,
	"Wacom.Run.Map.Traversal.RejectionAndMapTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapTraversalRejectionAndTravelTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession();
	if (!TestTrue(TEXT("Fixture initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	const FName FloorId = Initialized.Initialization.PostSnapshot.CurrentNode.FloorId;
	const FRunExplorationResolution StaleBegin = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ FloorId, TEXT("Edge.01") },
			Initialized.Initialization.PostSnapshot.StateVersion - 1));
	TestFalse(TEXT("Stale version is rejected"), StaleBegin.IsOk());
	TestTrue(TEXT("Stale version emits no events"), StaleBegin.Events.IsEmpty());

	const FRunExplorationResolution Begin = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ FloorId, TEXT("Edge.01") },
			Initialized.Initialization.PostSnapshot.StateVersion));
	if (!TestTrue(TEXT("Traversal begins"), Begin.IsOk()))
	{
		return false;
	}
	const FRunExplorationResolution TravelWhileActive = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::MapTravel(
			{ FloorId, TEXT("Node.01") },
			Begin.PostSnapshot.StateVersion));
	TestFalse(TEXT("MapTravel is rejected while traversal is active"), TravelWhileActive.IsOk());
	TestEqual(TEXT("Active rejection preserves source"),
		TravelWhileActive.PostSnapshot.CurrentNode.NodeId,
		FName(TEXT("Node.01")));

	const FRunExplorationResolution Cancel = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CancelTraversal(Begin.TraversalTicket.GetValue()));
	TestTrue(TEXT("Matching traversal can cancel"), Cancel.IsOk());
	TestEqual(TEXT("Cancel keeps source"),
		Cancel.PostSnapshot.CurrentNode.NodeId,
		FName(TEXT("Node.01")));
	TestEqual(TEXT("Cancel releases active state"),
		Cancel.PostSnapshot.ActiveActivityKind,
		ERunExplorationActivityKind::None);

	const FRunExplorationResolution DuplicateCancel = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CancelTraversal(Begin.TraversalTicket.GetValue()));
	TestFalse(TEXT("Duplicate cancelled ticket is rejected"), DuplicateCancel.IsOk());
	TestTrue(TEXT("Duplicate cancel emits no events"), DuplicateCancel.Events.IsEmpty());

	const FRunExplorationResolution BeginAgain = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ FloorId, TEXT("Edge.01") },
			Cancel.PostSnapshot.StateVersion));
	const FRunExplorationResolution Complete = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(BeginAgain.TraversalTicket.GetValue()));
	TestTrue(TEXT("Second traversal completes"), Complete.IsOk());

	const FRunExplorationResolution Travel = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::MapTravel(
			{ FloorId, TEXT("Node.01") },
			Complete.PostSnapshot.StateVersion));
	TestTrue(TEXT("Resolved node on current Floor is a free travel target"), Travel.IsOk());
	TestEqual(TEXT("MapTravel changes only logical position"),
		Travel.PostSnapshot.CurrentNode.NodeId,
		FName(TEXT("Node.01")));
	TestEqual(TEXT("MapTravel does not spend Action Points"),
		Travel.PostSnapshot.Time.RemainingActionPoints,
		Complete.PostSnapshot.Time.RemainingActionPoints);
	TestEqual(TEXT("MapTravel does not change pressure"),
		Travel.PostSnapshot.TotalPressure,
		Complete.PostSnapshot.TotalPressure);

	const FRunExplorationResolution HiddenTravel = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::MapTravel(
			{ FloorId, TEXT("Node.03") },
			Travel.PostSnapshot.StateVersion));
	TestFalse(TEXT("Unresolved node cannot be a travel target"), HiddenTravel.IsOk());
	TestTrue(TEXT("Rejected travel emits no events"), HiddenTravel.Events.IsEmpty());
	return true;
}

#endif
