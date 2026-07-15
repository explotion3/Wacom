// Copyright Wacom. All Rights Reserved.

#include "Exploration/RunExplorationSnapshotBuilder.h"

#include "Exploration/RunCampModule.h"
#include "Exploration/RunFloorTransitionModule.h"
#include "Exploration/RunMapModule.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunState.h"

FRunExplorationSnapshot FRunExplorationSnapshotBuilder::Build(const FRunState& State)
{
	FRunExplorationSnapshot Snapshot;
	const FRunExplorationState& Exploration = State.ExplorationState;
	Snapshot.StateVersion = Exploration.ExplorationStateVersion;
	Snapshot.JourneyId = Exploration.JourneyDefinition
		? Exploration.JourneyDefinition->JourneyId
		: NAME_None;
	Snapshot.CurrentNode = { Exploration.CurrentFloorId, Exploration.CurrentNodeId };
	Snapshot.Time = State.TimeState;
	Snapshot.FloorDay = FMath::Max(
		1,
		State.TimeState.CurrentDayNumber - Exploration.FloorEnteredDayNumber + 1);
	Snapshot.TotalPressure = State.Pressure.GetTotal();
	Snapshot.ActiveActivityKind = Exploration.ActiveActivityKind;

	for (const FRunFloorProgress& Progress : Exploration.FloorProgress)
	{
		if (Progress.EnteredDayNumber <= 0)
		{
			continue;
		}
		FRunFloorHistorySnapshot& History = Snapshot.FloorHistory.AddDefaulted_GetRef();
		History.FloorId = Progress.FloorId;
		History.EnteredDayNumber = Progress.EnteredDayNumber;
		History.TotalNodeCount = Progress.Nodes.Num();
		History.bIsCurrentFloor = Progress.FloorId == Exploration.CurrentFloorId;
		for (const FRunMapNodeProgress& Node : Progress.Nodes)
		{
			if (Node.Lifecycle != ERunMapNodeLifecycle::Hidden)
			{
				++History.RevealedNodeCount;
			}
			if (Node.Lifecycle == ERunMapNodeLifecycle::Resolved)
			{
				++History.ResolvedNodeCount;
			}
		}
	}

	const UWacomFloorMapDefinition* Floor = FRunMapModule::FindCurrentFloor(State);
	const FRunFloorProgress* FloorProgress = FRunMapModule::FindCurrentFloorProgress(State);
	if (!Floor || !FloorProgress)
	{
		return Snapshot;
	}

	Snapshot.Nodes.Reserve(Floor->Nodes.Num());
	for (const FWacomMapNodeDefinition& Node : Floor->Nodes)
	{
		const FRunMapNodeProgress* Progress =
			FRunMapModule::FindNodeProgress(*FloorProgress, Node.NodeId);
		FRunMapNodeSnapshot& NodeSnapshot = Snapshot.Nodes.AddDefaulted_GetRef();
		NodeSnapshot.Handle = { Floor->FloorId, Node.NodeId };
		NodeSnapshot.NodeType = Node.NodeType;
		if (Progress)
		{
			NodeSnapshot.Lifecycle = Progress->Lifecycle;
			NodeSnapshot.bLandmarkVisible = Progress->bLandmarkVisible;
			NodeSnapshot.bCanMapTravel =
				Progress->Lifecycle == ERunMapNodeLifecycle::Resolved
				&& Node.NodeId != Exploration.CurrentNodeId
				&& Exploration.ActiveActivityKind == ERunExplorationActivityKind::None;
		}
	}

	TArray<const FWacomMapEdgeDefinition*> Outgoing;
	Floor->FindOutgoingEdges(Exploration.CurrentNodeId, Outgoing);
	for (const FWacomMapEdgeDefinition* Edge : Outgoing)
	{
		if (!Edge)
		{
			continue;
		}
		const FRunMapNodeProgress* TargetProgress =
			FRunMapModule::FindNodeProgress(*FloorProgress, Edge->ToNodeId);
		FRunMapEdgeSnapshot& EdgeSnapshot = Snapshot.OutgoingEdges.AddDefaulted_GetRef();
		EdgeSnapshot.Handle = { Floor->FloorId, Edge->EdgeId };
		EdgeSnapshot.TargetNode = { Floor->FloorId, Edge->ToNodeId };
		EdgeSnapshot.bCanTraverse = TargetProgress
			&& TargetProgress->Lifecycle != ERunMapNodeLifecycle::Hidden
			&& Exploration.ActiveActivityKind == ERunExplorationActivityKind::None;
	}

	Snapshot.bCanBeginCamp =
		Exploration.ActiveActivityKind == ERunExplorationActivityKind::None
		&& State.TimeState.CurrentTimePhase == ETimePhase::Night
		&& State.TimeState.NightGate == ERunNightGate::AwaitingChoice
		&& State.TimeState.RemainingActionPoints >= 1
		&& FRunCampModule::FindNearestCampNode(State, Snapshot.NearestCampNode);

	Snapshot.bHasFloorTransitionPreview =
		FRunFloorTransitionModule::BuildCurrentPreview(State, Snapshot.FloorTransitionPreview);
	return Snapshot;
}
