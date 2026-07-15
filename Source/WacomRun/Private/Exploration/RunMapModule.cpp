// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunMapModule.h"

#include "Exploration/RunExplorationSnapshotBuilder.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunState.h"

namespace
{
	void AddNodeEvent(
		TArray<FRunExplorationEvent>& Events,
		const ERunExplorationEventType Type,
		const FName FloorId,
		const FName NodeId)
	{
		FRunExplorationEvent& Event = Events.AddDefaulted_GetRef();
		Event.Type = Type;
		Event.Node = { FloorId, NodeId };
	}

	bool IsSafeArrivalNode(const EWacomMapNodeType Type)
	{
		return Type == EWacomMapNodeType::Navigation || Type == EWacomMapNodeType::Shop;
	}
}

const UWacomFloorMapDefinition* FRunMapModule::FindCurrentFloor(const FRunState& State)
{
	return State.ExplorationState.JourneyDefinition
		? State.ExplorationState.JourneyDefinition->FindFloor(State.ExplorationState.CurrentFloorId)
		: nullptr;
}

FRunFloorProgress* FRunMapModule::FindCurrentFloorProgress(FRunState& State)
{
	return State.ExplorationState.FloorProgress.FindByPredicate(
		[&State](const FRunFloorProgress& Progress)
		{
			return Progress.FloorId == State.ExplorationState.CurrentFloorId;
		});
}

const FRunFloorProgress* FRunMapModule::FindCurrentFloorProgress(const FRunState& State)
{
	return State.ExplorationState.FloorProgress.FindByPredicate(
		[&State](const FRunFloorProgress& Progress)
		{
			return Progress.FloorId == State.ExplorationState.CurrentFloorId;
		});
}

FRunMapNodeProgress* FRunMapModule::FindNodeProgress(
	FRunFloorProgress& FloorProgress,
	const FName NodeId)
{
	return FloorProgress.Nodes.FindByPredicate([NodeId](const FRunMapNodeProgress& Progress)
	{
		return Progress.NodeId == NodeId;
	});
}

const FRunMapNodeProgress* FRunMapModule::FindNodeProgress(
	const FRunFloorProgress& FloorProgress,
	const FName NodeId)
{
	return FloorProgress.Nodes.FindByPredicate([NodeId](const FRunMapNodeProgress& Progress)
	{
		return Progress.NodeId == NodeId;
	});
}

bool FRunMapModule::IsDirectedReachable(
	const UWacomFloorMapDefinition& Floor,
	const FName FromNodeId,
	const FName ToNodeId)
{
	if (!Floor.FindNode(FromNodeId) || !Floor.FindNode(ToNodeId))
	{
		return false;
	}
	if (FromNodeId == ToNodeId)
	{
		return true;
	}

	TSet<FName> Visited;
	TArray<FName> Queue{ FromNodeId };
	for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
	{
		const FName Current = Queue[QueueIndex];
		if (Visited.Contains(Current))
		{
			continue;
		}
		Visited.Add(Current);

		TArray<const FWacomMapEdgeDefinition*> Outgoing;
		Floor.FindOutgoingEdges(Current, Outgoing);
		for (const FWacomMapEdgeDefinition* Edge : Outgoing)
		{
			if (!Edge)
			{
				continue;
			}
			if (Edge->ToNodeId == ToNodeId)
			{
				return true;
			}
			if (!Visited.Contains(Edge->ToNodeId))
			{
				Queue.Add(Edge->ToNodeId);
			}
		}
	}
	return false;
}

bool FRunMapModule::RevealOutgoingTargets(
	FRunState& State,
	TArray<FRunExplorationEvent>& OutEvents)
{
	const UWacomFloorMapDefinition* Floor = FindCurrentFloor(State);
	FRunFloorProgress* FloorProgress = FindCurrentFloorProgress(State);
	if (!Floor || !FloorProgress)
	{
		return false;
	}

	TArray<const FWacomMapEdgeDefinition*> Outgoing;
	Floor->FindOutgoingEdges(State.ExplorationState.CurrentNodeId, Outgoing);
	for (const FWacomMapEdgeDefinition* Edge : Outgoing)
	{
		FRunMapNodeProgress* TargetProgress =
			Edge ? FindNodeProgress(*FloorProgress, Edge->ToNodeId) : nullptr;
		if (TargetProgress && TargetProgress->Lifecycle == ERunMapNodeLifecycle::Hidden)
		{
			TargetProgress->Lifecycle = ERunMapNodeLifecycle::Revealed;
			AddNodeEvent(
				OutEvents,
				ERunExplorationEventType::NodeRevealed,
				Floor->FloorId,
				Edge->ToNodeId);
		}
	}
	return true;
}

bool FRunMapModule::CommitArrival(
	FRunState& State,
	const FName TargetNodeId,
	TArray<FRunExplorationEvent>& OutEvents)
{
	const UWacomFloorMapDefinition* Floor = FindCurrentFloor(State);
	FRunFloorProgress* FloorProgress = FindCurrentFloorProgress(State);
	const FWacomMapNodeDefinition* TargetNode = Floor ? Floor->FindNode(TargetNodeId) : nullptr;
	FRunMapNodeProgress* TargetProgress =
		FloorProgress ? FindNodeProgress(*FloorProgress, TargetNodeId) : nullptr;
	if (!Floor || !FloorProgress || !TargetNode || !TargetProgress
		|| TargetProgress->Lifecycle == ERunMapNodeLifecycle::Hidden)
	{
		return false;
	}

	State.ExplorationState.CurrentNodeId = TargetNodeId;
	if (TargetProgress->Lifecycle == ERunMapNodeLifecycle::Revealed)
	{
		TargetProgress->Lifecycle = ERunMapNodeLifecycle::Visited;
		AddNodeEvent(OutEvents, ERunExplorationEventType::NodeVisited, Floor->FloorId, TargetNodeId);
	}
	if (TargetProgress->Lifecycle == ERunMapNodeLifecycle::Visited
		&& IsSafeArrivalNode(TargetNode->NodeType))
	{
		TargetProgress->Lifecycle = ERunMapNodeLifecycle::Resolved;
		AddNodeEvent(OutEvents, ERunExplorationEventType::NodeResolved, Floor->FloorId, TargetNodeId);
	}
	if (!RevealOutgoingTargets(State, OutEvents))
	{
		return false;
	}

	if (!IsSafeArrivalNode(TargetNode->NodeType))
	{
		AddNodeEvent(
			OutEvents,
			ERunExplorationEventType::NodeContentRequested,
			Floor->FloorId,
			TargetNodeId);
	}
	return true;
}

bool FRunMapModule::CanMapTravel(const FRunState& State, const FWacomMapNodeHandle& Target)
{
	if (State.ExplorationState.ActiveActivityKind != ERunExplorationActivityKind::None
		|| !Target.IsValid()
		|| Target.FloorId != State.ExplorationState.CurrentFloorId
		|| Target.NodeId == State.ExplorationState.CurrentNodeId)
	{
		return false;
	}
	const FRunFloorProgress* Progress = FindCurrentFloorProgress(State);
	const FRunMapNodeProgress* NodeProgress = Progress ? FindNodeProgress(*Progress, Target.NodeId) : nullptr;
	return NodeProgress && NodeProgress->Lifecycle == ERunMapNodeLifecycle::Resolved;
}

bool FRunMapModule::CommitMapTravel(FRunState& State, const FWacomMapNodeHandle& Target)
{
	if (!CanMapTravel(State, Target))
	{
		return false;
	}
	State.ExplorationState.CurrentNodeId = Target.NodeId;
	return true;
}

bool FRunMapModule::ResolveNode(
	FRunState& State,
	const FWacomMapNodeHandle& Node,
	TArray<FRunExplorationEvent>& OutEvents)
{
	if (!Node.IsValid() || Node.FloorId != State.ExplorationState.CurrentFloorId)
	{
		return false;
	}
	FRunFloorProgress* FloorProgress = FindCurrentFloorProgress(State);
	FRunMapNodeProgress* Progress = FloorProgress
		? FindNodeProgress(*FloorProgress, Node.NodeId)
		: nullptr;
	if (!Progress || Progress->Lifecycle == ERunMapNodeLifecycle::Hidden)
	{
		return false;
	}
	if (Progress->Lifecycle != ERunMapNodeLifecycle::Resolved)
	{
		Progress->Lifecycle = ERunMapNodeLifecycle::Resolved;
		FRunExplorationEvent& Event = OutEvents.AddDefaulted_GetRef();
		Event.Type = ERunExplorationEventType::NodeResolved;
		Event.Node = Node;
	}
	return true;
}

FRunExplorationSnapshot FRunMapModule::BuildSnapshot(const FRunState& State)
{
	return FRunExplorationSnapshotBuilder::Build(State);
}
