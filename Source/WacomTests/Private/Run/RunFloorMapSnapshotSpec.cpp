// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Exploration/RunFloorMapSnapshot.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"

namespace
{
	FWacomMapNodeDefinition MakeNode(
		const TCHAR* NodeId,
		const TCHAR* Title,
		const FVector2D Position,
		const EWacomMapLandmarkVisibility Landmark = EWacomMapLandmarkVisibility::None)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.DisplayName = FText::FromString(Title);
		Node.ShortDescription = FText::Format(
			NSLOCTEXT("WacomTests", "MapNodeDescription", "{0} 的说明"),
			Node.DisplayName);
		Node.NodeType = EWacomMapNodeType::Navigation;
		Node.MapPosition = Position;
		Node.LandmarkVisibility = Landmark;
		return Node;
	}

	FWacomMapEdgeDefinition MakeEdge(
		const TCHAR* EdgeId,
		const TCHAR* From,
		const TCHAR* To)
	{
		FWacomMapEdgeDefinition Edge;
		Edge.EdgeId = EdgeId;
		Edge.FromNodeId = From;
		Edge.ToNodeId = To;
		return Edge;
	}

	FRunMapNodeProgress* FindMutableProgress(FRunState& State, const FName NodeId)
	{
		for (FRunFloorProgress& Floor : State.ExplorationState.FloorProgress)
		{
			if (Floor.FloorId != State.ExplorationState.CurrentFloorId)
			{
				continue;
			}
			return Floor.Nodes.FindByPredicate(
				[NodeId](const FRunMapNodeProgress& Node)
				{
					return Node.NodeId == NodeId;
				});
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorMapSnapshotVisibilityTest,
	"Wacom.Run.Map.PresentationSnapshot.VisibilityAndRecommendation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorMapSnapshotVisibilityTest::RunTest(const FString& Parameters)
{
	FWacomRunExplorationFixture Fixture;
	TArray<FWacomMapNodeDefinition> Nodes{
		MakeNode(TEXT("Node.Current"), TEXT("当前位置"), FVector2D(960.0, 900.0)),
		MakeNode(TEXT("Node.A"), TEXT("已完成 A"), FVector2D(680.0, 680.0)),
		MakeNode(TEXT("Node.B"), TEXT("已完成 B"), FVector2D(1240.0, 680.0)),
		MakeNode(TEXT("Node.Revealed"), TEXT("已揭示"), FVector2D(560.0, 420.0)),
		MakeNode(TEXT("Node.Visited"), TEXT("已访问"), FVector2D(860.0, 420.0)),
		MakeNode(
			TEXT("Node.Landmark"),
			TEXT("隐藏地标真名"),
			FVector2D(1360.0, 360.0),
			EWacomMapLandmarkVisibility::BossOutline),
		MakeNode(TEXT("Node.Hidden"), TEXT("隐藏节点"), FVector2D(1600.0, 220.0)),
	};
	TArray<FWacomMapEdgeDefinition> Edges{
		MakeEdge(TEXT("Edge.CurrentA"), TEXT("Node.Current"), TEXT("Node.A")),
		MakeEdge(TEXT("Edge.CurrentB"), TEXT("Node.Current"), TEXT("Node.B")),
		MakeEdge(TEXT("Edge.Known"), TEXT("Node.Revealed"), TEXT("Node.Visited")),
		MakeEdge(TEXT("Edge.Landmark"), TEXT("Node.B"), TEXT("Node.Landmark")),
		MakeEdge(TEXT("Edge.Hidden"), TEXT("Node.Landmark"), TEXT("Node.Hidden")),
	};
	UWacomFloorMapDefinition* Floor = Fixture.MakeFloor(
		TEXT("Test.Floor.MapSnapshot"),
		NSLOCTEXT("WacomTests", "MapSnapshotFloor", "测试地图"),
		Nodes,
		Edges,
		TEXT("Node.Current"));
	const FWacomInitializedRunExplorationSession Initialized = Fixture.CreateInitializedSession(
		Fixture.MakeCharacter(),
		Fixture.MakeJourney({ Floor }));
	if (!TestTrue(TEXT("Fixture initializes"), Initialized.Initialization.IsOk()))
	{
		return false;
	}

	FRunState& MutableState = FWacomRunSessionTestAccess::GetMutableRunState(*Initialized.Session);
	const TMap<FName, ERunMapNodeLifecycle> Lifecycles{
		{ TEXT("Node.Current"), ERunMapNodeLifecycle::Resolved },
		{ TEXT("Node.A"), ERunMapNodeLifecycle::Resolved },
		{ TEXT("Node.B"), ERunMapNodeLifecycle::Resolved },
		{ TEXT("Node.Revealed"), ERunMapNodeLifecycle::Revealed },
		{ TEXT("Node.Visited"), ERunMapNodeLifecycle::Visited },
		{ TEXT("Node.Landmark"), ERunMapNodeLifecycle::Hidden },
		{ TEXT("Node.Hidden"), ERunMapNodeLifecycle::Hidden },
	};
	for (const TPair<FName, ERunMapNodeLifecycle>& Pair : Lifecycles)
	{
		if (FRunMapNodeProgress* Progress = FindMutableProgress(MutableState, Pair.Key))
		{
			Progress->Lifecycle = Pair.Value;
			Progress->bLandmarkVisible = Pair.Key == TEXT("Node.Landmark");
		}
	}

	const int32 VersionBefore = MutableState.ExplorationState.ExplorationStateVersion;
	const int32 ActionPointsBefore = MutableState.TimeState.RemainingActionPoints;
	const int32 PressureBefore = MutableState.Pressure.GetTotal();
	int32 NotificationCount = 0;
	Initialized.Session->OnRunStateChangedNative.AddLambda([&NotificationCount]()
	{
		++NotificationCount;
	});

	const FRunFloorMapSnapshot Snapshot = Initialized.Session->BuildCurrentFloorMapSnapshot();
	TestTrue(TEXT("Snapshot is valid"), Snapshot.IsValid());
	TestEqual(TEXT("Floor title is authored text"), Snapshot.FloorDisplayName.ToString(), FString(TEXT("测试地图")));
	TestEqual(TEXT("Ordinary hidden node is omitted"),
		Snapshot.Nodes.ContainsByPredicate([](const FRunFloorMapNodeSnapshot& Node)
		{
			return Node.Handle.NodeId == TEXT("Node.Hidden");
		}), false);
	const FRunFloorMapNodeSnapshot* Landmark = Snapshot.Nodes.FindByPredicate(
		[](const FRunFloorMapNodeSnapshot& Node)
		{
			return Node.Handle.NodeId == TEXT("Node.Landmark");
		});
	if (TestNotNull(TEXT("Landmark-only node is present"), Landmark))
	{
		TestTrue(TEXT("Landmark-only flag is explicit"), Landmark->bLandmarkOnly);
		TestTrue(TEXT("Landmark title is concealed"), Landmark->DisplayName.IsEmpty());
		TestTrue(TEXT("Landmark description is concealed"), Landmark->ShortDescription.IsEmpty());
	}
	TestEqual(TEXT("Edges with hidden endpoints are omitted"), Snapshot.Edges.Num(), 3);
	TestTrue(TEXT("Nearest tie produces a recommendation"), Snapshot.RecommendedTravelTarget.IsSet());
	if (Snapshot.RecommendedTravelTarget.IsSet())
	{
		TestEqual(TEXT("BFS tie is stable by NodeId"),
			Snapshot.RecommendedTravelTarget->NodeId,
			FName(TEXT("Node.A")));
	}

	TestEqual(TEXT("Query emits no notification"), NotificationCount, 0);
	TestEqual(TEXT("Query preserves version"),
		MutableState.ExplorationState.ExplorationStateVersion,
		VersionBefore);
	TestEqual(TEXT("Query preserves action points"),
		MutableState.TimeState.RemainingActionPoints,
		ActionPointsBefore);
	TestEqual(TEXT("Query preserves pressure"), MutableState.Pressure.GetTotal(), PressureBefore);
	return true;
}

#endif
