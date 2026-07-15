// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunSceneBindingRegistry.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "GameFramework/Actor.h"

void FWacomRunSceneBindingRegistry::Reset(const FName InFloorId)
{
	FloorId = InFloorId;
	PathsByEdgeId.Reset();
	AnchorsByNodeId.Reset();
	ContentHostsByNodeId.Reset();
}

bool FWacomRunSceneBindingRegistry::RegisterPath(AWacomRunPathSegmentActor& Path)
{
	if (Path.EdgeId.IsNone() || PathsByEdgeId.Contains(Path.EdgeId))
	{
		return false;
	}
	PathsByEdgeId.Add(Path.EdgeId, &Path);
	return true;
}

bool FWacomRunSceneBindingRegistry::RegisterNodeAnchor(AWacomRunMapNodeAnchorActor& Anchor)
{
	if (Anchor.NodeId.IsNone() || AnchorsByNodeId.Contains(Anchor.NodeId))
	{
		return false;
	}
	AnchorsByNodeId.Add(Anchor.NodeId, &Anchor);
	return true;
}

bool FWacomRunSceneBindingRegistry::RegisterContentHost(
	const FName NodeId,
	const EWacomMapNodeType NodeType,
	AActor& Host)
{
	if (NodeId.IsNone() || !RequiresContentHost(NodeType) || ContentHostsByNodeId.Contains(NodeId))
	{
		return false;
	}
	FContentHostBinding Binding;
	Binding.NodeType = NodeType;
	Binding.Host = &Host;
	ContentHostsByNodeId.Add(NodeId, Binding);
	return true;
}

void FWacomRunSceneBindingRegistry::UnregisterPath(const AWacomRunPathSegmentActor& Path)
{
	if (const TWeakObjectPtr<AWacomRunPathSegmentActor>* Existing = PathsByEdgeId.Find(Path.EdgeId);
		Existing && Existing->Get() == &Path)
	{
		PathsByEdgeId.Remove(Path.EdgeId);
	}
}

void FWacomRunSceneBindingRegistry::UnregisterNodeAnchor(const AWacomRunMapNodeAnchorActor& Anchor)
{
	if (const TWeakObjectPtr<AWacomRunMapNodeAnchorActor>* Existing = AnchorsByNodeId.Find(Anchor.NodeId);
		Existing && Existing->Get() == &Anchor)
	{
		AnchorsByNodeId.Remove(Anchor.NodeId);
	}
}

void FWacomRunSceneBindingRegistry::UnregisterContentHost(const AActor& Host)
{
	for (auto It = ContentHostsByNodeId.CreateIterator(); It; ++It)
	{
		if (It.Value().Host.Get() == &Host)
		{
			It.RemoveCurrent();
		}
	}
}

FWacomRunTraversalSceneBinding FWacomRunSceneBindingRegistry::PreflightTraversal(
	const FRunExplorationSnapshot& Snapshot,
	const FName EdgeId) const
{
	FWacomRunTraversalSceneBinding Result;
	Result.SourceNode = Snapshot.CurrentNode;
	Result.Edge = { Snapshot.CurrentNode.FloorId, EdgeId };
	if (FloorId.IsNone() || Snapshot.CurrentNode.FloorId != FloorId || EdgeId.IsNone())
	{
		Result.Status = FWacomStatus::Fail(EWacomError::InvalidState, TEXT("SceneFloorMismatch"));
		return Result;
	}

	const FRunMapEdgeSnapshot* Edge = Snapshot.OutgoingEdges.FindByPredicate(
		[EdgeId](const FRunMapEdgeSnapshot& Candidate)
		{
			return Candidate.Handle.EdgeId == EdgeId && Candidate.bCanTraverse;
		});
	const TWeakObjectPtr<AWacomRunPathSegmentActor>* Path = PathsByEdgeId.Find(EdgeId);
	AWacomRunMapNodeAnchorActor* SourceAnchor = FindNodeAnchor(Snapshot.CurrentNode.NodeId);
	if (!Edge || !Path || !Path->IsValid() || !SourceAnchor)
	{
		Result.Status = FWacomStatus::Fail(EWacomError::NotFound, TEXT("TraversalSceneBindingMissing"));
		return Result;
	}

	const FRunMapNodeSnapshot* TargetNode = Snapshot.Nodes.FindByPredicate(
		[&Edge](const FRunMapNodeSnapshot& Candidate)
		{
			return Candidate.Handle == Edge->TargetNode;
		});
	AWacomRunMapNodeAnchorActor* TargetAnchor = nullptr;
	AActor* ContentHost = nullptr;
	const FWacomStatus TargetStatus = TargetNode
		? RevalidateTarget(Edge->TargetNode, TargetNode->NodeType, TargetAnchor, ContentHost)
		: FWacomStatus::Fail(EWacomError::NotFound, TEXT("TargetNodeSnapshotMissing"));
	if (!TargetStatus.IsOk())
	{
		Result.Status = TargetStatus;
		return Result;
	}

	Result.Status = FWacomStatus::Ok();
	Result.TargetNode = Edge->TargetNode;
	Result.TargetNodeType = TargetNode->NodeType;
	Result.Path = Path->Get();
	Result.SourceAnchor = SourceAnchor;
	Result.TargetAnchor = TargetAnchor;
	Result.ContentHost = ContentHost;
	Result.CachedSourceTransform = SourceAnchor->GetViewTransform();
	Result.CachedTargetTransform = TargetAnchor->GetViewTransform();
	return Result;
}

FWacomStatus FWacomRunSceneBindingRegistry::RevalidateTarget(
	const FWacomMapNodeHandle& TargetNode,
	const EWacomMapNodeType TargetNodeType,
	AWacomRunMapNodeAnchorActor*& OutAnchor,
	AActor*& OutContentHost) const
{
	OutAnchor = nullptr;
	OutContentHost = nullptr;
	if (TargetNode.FloorId != FloorId)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("SceneFloorMismatch"));
	}
	OutAnchor = FindNodeAnchor(TargetNode.NodeId);
	if (!OutAnchor)
	{
		return FWacomStatus::Fail(EWacomError::NotFound, TEXT("TargetNodeAnchorMissing"));
	}
	if (!RequiresContentHost(TargetNodeType))
	{
		return FWacomStatus::Ok();
	}
	const FContentHostBinding* Binding = ContentHostsByNodeId.Find(TargetNode.NodeId);
	if (!Binding || Binding->NodeType != TargetNodeType || !Binding->Host.IsValid())
	{
		return FWacomStatus::Fail(EWacomError::NotFound, TEXT("TargetContentHostMissing"));
	}
	OutContentHost = Binding->Host.Get();
	return FWacomStatus::Ok();
}

AWacomRunMapNodeAnchorActor* FWacomRunSceneBindingRegistry::FindNodeAnchor(const FName NodeId) const
{
	const TWeakObjectPtr<AWacomRunMapNodeAnchorActor>* Found = AnchorsByNodeId.Find(NodeId);
	return Found ? Found->Get() : nullptr;
}

bool FWacomRunSceneBindingRegistry::RequiresContentHost(const EWacomMapNodeType NodeType)
{
	return NodeType != EWacomMapNodeType::Navigation;
}
