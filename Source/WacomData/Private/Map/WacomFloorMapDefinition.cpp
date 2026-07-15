// Copyright Wacom. All Rights Reserved.

#include "Map/WacomFloorMapDefinition.h"

const FWacomMapNodeDefinition* UWacomFloorMapDefinition::FindNode(const FName NodeId) const
{
	return Nodes.FindByPredicate([NodeId](const FWacomMapNodeDefinition& Node)
	{
		return Node.NodeId == NodeId;
	});
}

const FWacomMapEdgeDefinition* UWacomFloorMapDefinition::FindEdge(const FName EdgeId) const
{
	return Edges.FindByPredicate([EdgeId](const FWacomMapEdgeDefinition& Edge)
	{
		return Edge.EdgeId == EdgeId;
	});
}

void UWacomFloorMapDefinition::FindOutgoingEdges(
	const FName FromNodeId,
	TArray<const FWacomMapEdgeDefinition*>& OutEdges) const
{
	OutEdges.Reset();
	for (const FWacomMapEdgeDefinition& Edge : Edges)
	{
		if (Edge.FromNodeId == FromNodeId)
		{
			OutEdges.Add(&Edge);
		}
	}
}
