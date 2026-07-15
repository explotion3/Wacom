// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"

/** 当前 Floor 地图中的一条可呈现节点事实；不持有 DataAsset、Actor 或 Widget。 */
struct WACOMRUN_API FRunFloorMapNodeSnapshot
{
	FWacomMapNodeHandle Handle;
	FText DisplayName;
	FText ShortDescription;
	EWacomMapNodeType NodeType = EWacomMapNodeType::Navigation;
	FVector2D MapPosition = FVector2D::ZeroVector;
	ERunMapNodeLifecycle Lifecycle = ERunMapNodeLifecycle::Hidden;
	bool bLandmarkOnly = false;
	bool bIsCurrentNode = false;
	bool bCanMapTravel = false;
};

/** 当前 Floor 地图中的一条已知有向边事实。 */
struct WACOMRUN_API FRunFloorMapEdgeSnapshot
{
	FWacomMapEdgeHandle Handle;
	FWacomMapNodeHandle SourceNode;
	FWacomMapNodeHandle TargetNode;
};

/**
 * WacomRun 生成的当前 Floor 完整只读地图事实。
 *
 * 与高频 FRunExplorationSnapshot 分离，只在地图 Screen 打开或状态变化时查询。
 */
struct WACOMRUN_API FRunFloorMapSnapshot
{
	int32 StateVersion = 0;
	FName FloorId = NAME_None;
	FText FloorDisplayName;
	FWacomMapNodeHandle CurrentNode;
	TArray<FRunFloorMapNodeSnapshot> Nodes;
	TArray<FRunFloorMapEdgeSnapshot> Edges;
	TOptional<FWacomMapNodeHandle> RecommendedTravelTarget;
	ERunExplorationActivityKind ActiveActivityKind = ERunExplorationActivityKind::None;

	bool IsValid() const
	{
		return StateVersion > 0
			&& !FloorId.IsNone()
			&& CurrentNode.IsValid()
			&& CurrentNode.FloorId == FloorId
			&& Nodes.ContainsByPredicate(
				[this](const FRunFloorMapNodeSnapshot& Node)
				{
					return Node.Handle == CurrentNode;
				});
	}
};
