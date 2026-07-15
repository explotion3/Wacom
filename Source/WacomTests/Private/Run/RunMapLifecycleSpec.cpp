// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"

namespace
{
	const FRunMapNodeSnapshot* FindNodeSnapshot(
		const FRunExplorationSnapshot& Snapshot,
		const FName NodeId)
	{
		return Snapshot.Nodes.FindByPredicate([NodeId](const FRunMapNodeSnapshot& Node)
		{
			return Node.Handle.NodeId == NodeId;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapLifecycleProgressionTest,
	"Wacom.Run.Map.Lifecycle.ProgressionAndLandmarks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapLifecycleProgressionTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	UWacomFloorMapDefinition* Floor = Fixture.MakeLinearFloor(TEXT("Test.Floor.Lifecycle"), 3);
	Floor->Nodes[2].NodeType = EWacomMapNodeType::FloorEntrance;
	Floor->Nodes[2].LandmarkVisibility = EWacomMapLandmarkVisibility::FloorEntranceOutline;
	Floor->Nodes[2].Content.FloorEntrance.TargetFloorId = TEXT("Test.Floor.Future");
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession(
		Fixture.MakeCharacter(),
		Fixture.MakeJourney({ Floor }));
	if (!TestTrue(TEXT("Fixture initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	const FRunMapNodeSnapshot* Entry = FindNodeSnapshot(
		Initialized.Initialization.PostSnapshot,
		TEXT("Node.01"));
	const FRunMapNodeSnapshot* Next = FindNodeSnapshot(
		Initialized.Initialization.PostSnapshot,
		TEXT("Node.02"));
	const FRunMapNodeSnapshot* Landmark = FindNodeSnapshot(
		Initialized.Initialization.PostSnapshot,
		TEXT("Node.03"));
	if (!TestNotNull(TEXT("Entry snapshot exists"), Entry)
		|| !TestNotNull(TEXT("Next snapshot exists"), Next)
		|| !TestNotNull(TEXT("Landmark snapshot exists"), Landmark))
	{
		return false;
	}
	TestEqual(TEXT("Safe entry resolves during initialization"),
		Entry->Lifecycle,
		ERunMapNodeLifecycle::Resolved);
	TestEqual(TEXT("Only outgoing target is revealed"),
		Next->Lifecycle,
		ERunMapNodeLifecycle::Revealed);
	TestEqual(TEXT("Distant landmark remains hidden"),
		Landmark->Lifecycle,
		ERunMapNodeLifecycle::Hidden);
	TestTrue(TEXT("Distant FloorEntrance exposes outline fact"), Landmark->bLandmarkVisible);

	const FRunExplorationResolution BeginFirst = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ Floor->FloorId, TEXT("Edge.01") },
			Initialized.Initialization.PostSnapshot.StateVersion));
	const FRunExplorationResolution CompleteFirst = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(BeginFirst.TraversalTicket.GetValue()));
	TestTrue(TEXT("First traversal completes"), CompleteFirst.IsOk());
	Next = FindNodeSnapshot(CompleteFirst.PostSnapshot, TEXT("Node.02"));
	Landmark = FindNodeSnapshot(CompleteFirst.PostSnapshot, TEXT("Node.03"));
	TestEqual(TEXT("Visited Navigation auto-resolves"),
		Next->Lifecycle,
		ERunMapNodeLifecycle::Resolved);
	TestEqual(TEXT("Completing Node.02 reveals its outgoing target"),
		Landmark->Lifecycle,
		ERunMapNodeLifecycle::Revealed);

	const FRunExplorationResolution ReverseAttempt = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ Floor->FloorId, TEXT("Edge.01") },
			CompleteFirst.PostSnapshot.StateVersion));
	TestFalse(TEXT("Directed graph does not infer a reverse edge"), ReverseAttempt.IsOk());
	TestTrue(TEXT("Rejected reverse attempt emits no events"), ReverseAttempt.Events.IsEmpty());
	TestEqual(TEXT("Rejected reverse attempt preserves current node"),
		ReverseAttempt.PostSnapshot.CurrentNode.NodeId,
		FName(TEXT("Node.02")));

	const FRunExplorationResolution BeginEntrance = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginTraversal(
			{ Floor->FloorId, TEXT("Edge.02") },
			CompleteFirst.PostSnapshot.StateVersion));
	const FRunExplorationResolution CompleteEntrance = Initialized.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CompleteTraversal(BeginEntrance.TraversalTicket.GetValue()));
	TestTrue(TEXT("Entrance traversal completes"), CompleteEntrance.IsOk());
	Landmark = FindNodeSnapshot(CompleteEntrance.PostSnapshot, TEXT("Node.03"));
	TestEqual(TEXT("Content node stops at Visited until content settlement"),
		Landmark->Lifecycle,
		ERunMapNodeLifecycle::Visited);
	TestTrue(TEXT("Content arrival emits explicit request"),
		CompleteEntrance.Events.ContainsByPredicate([](const FRunExplorationEvent& Event)
		{
			return Event.Type == ERunExplorationEventType::NodeContentRequested;
		}));
	return true;
}

#endif
