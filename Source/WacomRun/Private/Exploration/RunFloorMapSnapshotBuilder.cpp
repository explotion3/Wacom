// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunFloorMapSnapshotBuilder.h"

#include "Exploration/RunMapModule.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunState.h"

namespace
{
	const FRunMapNodeProgress* FindProgress(
		const FRunFloorProgress& FloorProgress,
		const FName NodeId)
	{
		return FRunMapModule::FindNodeProgress(FloorProgress, NodeId);
	}

	void BuildRecommendedTravelTarget(
		const UWacomFloorMapDefinition& Floor,
		const FRunFloorProgress& FloorProgress,
		const FRunExplorationState& Exploration,
		FRunFloorMapSnapshot& OutSnapshot)
	{
		if (Exploration.ActiveActivityKind != ERunExplorationActivityKind::None
			|| Exploration.CurrentNodeId.IsNone())
		{
			return;
		}

		TMap<FName, TArray<FName>> Adjacency;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			Adjacency.FindOrAdd(Node.NodeId);
		}
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			if (!Floor.FindNode(Edge.FromNodeId) || !Floor.FindNode(Edge.ToNodeId))
			{
				continue;
			}
			Adjacency.FindOrAdd(Edge.FromNodeId).AddUnique(Edge.ToNodeId);
			Adjacency.FindOrAdd(Edge.ToNodeId).AddUnique(Edge.FromNodeId);
		}

		TArray<FName> Frontier{ Exploration.CurrentNodeId };
		TSet<FName> Visited{ Exploration.CurrentNodeId };
		while (!Frontier.IsEmpty())
		{
			TArray<FName> Candidates;
			TArray<FName> NextFrontier;
			for (const FName NodeId : Frontier)
			{
				const FRunMapNodeProgress* Progress = FindProgress(FloorProgress, NodeId);
				if (NodeId != Exploration.CurrentNodeId
					&& Progress
					&& Progress->Lifecycle == ERunMapNodeLifecycle::Resolved)
				{
					Candidates.Add(NodeId);
				}

				const TArray<FName>* Neighbours = Adjacency.Find(NodeId);
				if (!Neighbours)
				{
					continue;
				}
				for (const FName Neighbour : *Neighbours)
				{
					if (!Visited.Contains(Neighbour))
					{
						Visited.Add(Neighbour);
						NextFrontier.Add(Neighbour);
					}
				}
			}

			if (!Candidates.IsEmpty())
			{
				Candidates.Sort([](const FName A, const FName B)
				{
					return A.LexicalLess(B);
				});
				OutSnapshot.RecommendedTravelTarget =
					FWacomMapNodeHandle{ Floor.FloorId, Candidates[0] };
				return;
			}

			Frontier = MoveTemp(NextFrontier);
		}
	}
}

FRunFloorMapSnapshot FRunFloorMapSnapshotBuilder::Build(const FRunState& State)
{
	FRunFloorMapSnapshot Snapshot;
	const FRunExplorationState& Exploration = State.ExplorationState;
	Snapshot.StateVersion = Exploration.ExplorationStateVersion;
	Snapshot.FloorId = Exploration.CurrentFloorId;
	Snapshot.CurrentNode = { Exploration.CurrentFloorId, Exploration.CurrentNodeId };
	Snapshot.ActiveActivityKind = Exploration.ActiveActivityKind;

	const UWacomFloorMapDefinition* Floor = FRunMapModule::FindCurrentFloor(State);
	const FRunFloorProgress* FloorProgress = FRunMapModule::FindCurrentFloorProgress(State);
	if (!Floor || !FloorProgress || Floor->FloorId != Exploration.CurrentFloorId)
	{
		return Snapshot;
	}

	Snapshot.FloorDisplayName = Floor->DisplayName;
	Snapshot.Nodes.Reserve(Floor->Nodes.Num());
	for (const FWacomMapNodeDefinition& Node : Floor->Nodes)
	{
		const FRunMapNodeProgress* Progress = FindProgress(*FloorProgress, Node.NodeId);
		if (!Progress
			|| (Progress->Lifecycle == ERunMapNodeLifecycle::Hidden
				&& !Progress->bLandmarkVisible))
		{
			continue;
		}

		FRunFloorMapNodeSnapshot& NodeSnapshot = Snapshot.Nodes.AddDefaulted_GetRef();
		NodeSnapshot.Handle = { Floor->FloorId, Node.NodeId };
		NodeSnapshot.NodeType = Node.NodeType;
		NodeSnapshot.MapPosition = Node.MapPosition;
		NodeSnapshot.Lifecycle = Progress->Lifecycle;
		NodeSnapshot.bLandmarkOnly =
			Progress->Lifecycle == ERunMapNodeLifecycle::Hidden
			&& Progress->bLandmarkVisible;
		NodeSnapshot.bIsCurrentNode = Node.NodeId == Exploration.CurrentNodeId;
		NodeSnapshot.bCanMapTravel =
			Progress->Lifecycle == ERunMapNodeLifecycle::Resolved
			&& !NodeSnapshot.bIsCurrentNode
			&& Exploration.ActiveActivityKind == ERunExplorationActivityKind::None;
		if (!NodeSnapshot.bLandmarkOnly)
		{
			NodeSnapshot.DisplayName = Node.DisplayName;
			NodeSnapshot.ShortDescription = Node.ShortDescription;
		}
	}

	Snapshot.Edges.Reserve(Floor->Edges.Num());
	for (const FWacomMapEdgeDefinition& Edge : Floor->Edges)
	{
		const FRunMapNodeProgress* SourceProgress = FindProgress(*FloorProgress, Edge.FromNodeId);
		const FRunMapNodeProgress* TargetProgress = FindProgress(*FloorProgress, Edge.ToNodeId);
		if (!SourceProgress || !TargetProgress
			|| SourceProgress->Lifecycle == ERunMapNodeLifecycle::Hidden
			|| TargetProgress->Lifecycle == ERunMapNodeLifecycle::Hidden)
		{
			continue;
		}

		FRunFloorMapEdgeSnapshot& EdgeSnapshot = Snapshot.Edges.AddDefaulted_GetRef();
		EdgeSnapshot.Handle = { Floor->FloorId, Edge.EdgeId };
		EdgeSnapshot.SourceNode = { Floor->FloorId, Edge.FromNodeId };
		EdgeSnapshot.TargetNode = { Floor->FloorId, Edge.ToNodeId };
	}

	BuildRecommendedTravelTarget(*Floor, *FloorProgress, Exploration, Snapshot);
	return Snapshot;
}
