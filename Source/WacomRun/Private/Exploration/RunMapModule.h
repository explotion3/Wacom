// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"

struct FRunState;
class UWacomFloorMapDefinition;

/** Logical Map Graph 的私有深层模块；不依赖场景或 UI。 */
class FRunMapModule
{
public:
	static const UWacomFloorMapDefinition* FindCurrentFloor(const FRunState& State);
	static FRunFloorProgress* FindCurrentFloorProgress(FRunState& State);
	static const FRunFloorProgress* FindCurrentFloorProgress(const FRunState& State);
	static FRunMapNodeProgress* FindNodeProgress(FRunFloorProgress& FloorProgress, FName NodeId);
	static const FRunMapNodeProgress* FindNodeProgress(
		const FRunFloorProgress& FloorProgress,
		FName NodeId);

	static bool IsDirectedReachable(
		const UWacomFloorMapDefinition& Floor,
		FName FromNodeId,
		FName ToNodeId);

	static bool RevealOutgoingTargets(
		FRunState& State,
		TArray<FRunExplorationEvent>& OutEvents);

	static bool CommitArrival(
		FRunState& State,
		FName TargetNodeId,
		TArray<FRunExplorationEvent>& OutEvents);

	static bool CanMapTravel(const FRunState& State, const FWacomMapNodeHandle& Target);
	static bool CommitMapTravel(FRunState& State, const FWacomMapNodeHandle& Target);
	static bool ResolveNode(
		FRunState& State,
		const FWacomMapNodeHandle& Node,
		TArray<FRunExplorationEvent>& OutEvents);
	static FRunExplorationSnapshot BuildSnapshot(const FRunState& State);
};
