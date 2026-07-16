// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunSceneBindingRegistry.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "GameFramework/Actor.h"
#include "Map/WacomFloorMapDefinition.h"

namespace
{
	FWacomStatus FailIncompleteSceneBinding(const TCHAR* Detail)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, Detail);
	}
}

void FWacomRunSceneBindingRegistry::Reset(const FName InFloorId)
{
	FloorId = InFloorId;
	PathsByEdgeId.Reset();
	AnchorsByNodeId.Reset();
	BranchTargetsByEdgeId.Reset();
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

bool FWacomRunSceneBindingRegistry::RegisterBranchTarget(
	AWacomRunPathBranchTargetActor& Target)
{
	if (Target.EdgeId.IsNone() || BranchTargetsByEdgeId.Contains(Target.EdgeId))
	{
		return false;
	}
	BranchTargetsByEdgeId.Add(Target.EdgeId, &Target);
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

FWacomStatus FWacomRunSceneBindingRegistry::ValidateComplete(
	const UWacomFloorMapDefinition& FloorDefinition) const
{
	if (FloorId.IsNone() || FloorDefinition.FloorId != FloorId)
	{
		return FailIncompleteSceneBinding(TEXT("SceneFloorDefinitionMismatch"));
	}

	TSet<FName> ExpectedNodeIds;
	TMap<FName, EWacomMapNodeType> ExpectedNodeTypes;
	for (const FWacomMapNodeDefinition& Node : FloorDefinition.Nodes)
	{
		if (Node.NodeId.IsNone() || ExpectedNodeIds.Contains(Node.NodeId))
		{
			return FailIncompleteSceneBinding(TEXT("SceneFloorNodeIdentityInvalid"));
		}
		ExpectedNodeIds.Add(Node.NodeId);
		ExpectedNodeTypes.Add(Node.NodeId, Node.NodeType);
		const TWeakObjectPtr<AWacomRunMapNodeAnchorActor>* Anchor =
			AnchorsByNodeId.Find(Node.NodeId);
		if (!Anchor || !Anchor->IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("SceneNodeAnchorMissing"));
		}

		const FContentHostBinding* Host = ContentHostsByNodeId.Find(Node.NodeId);
		if (RequiresContentHost(Node.NodeType))
		{
			if (!Host || Host->NodeType != Node.NodeType || !Host->Host.IsValid())
			{
				return FailIncompleteSceneBinding(TEXT("SceneContentHostMissingOrMismatched"));
			}
		}
		else if (Host)
		{
			return FailIncompleteSceneBinding(TEXT("SceneContentHostUnexpected"));
		}
	}

	for (const TPair<FName, TWeakObjectPtr<AWacomRunMapNodeAnchorActor>>& Pair :
		AnchorsByNodeId)
	{
		if (!ExpectedNodeIds.Contains(Pair.Key) || !Pair.Value.IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("SceneNodeAnchorUnexpected"));
		}
	}
	for (const TPair<FName, FContentHostBinding>& Pair : ContentHostsByNodeId)
	{
		const EWacomMapNodeType* ExpectedType = ExpectedNodeTypes.Find(Pair.Key);
		if (!ExpectedType || *ExpectedType != Pair.Value.NodeType
			|| !Pair.Value.Host.IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("SceneContentHostUnexpected"));
		}
	}

	TSet<FName> ExpectedEdgeIds;
	TMap<FName, int32> OutgoingCountsByNodeId;
	for (const FWacomMapEdgeDefinition& Edge : FloorDefinition.Edges)
	{
		if (Edge.EdgeId.IsNone() || ExpectedEdgeIds.Contains(Edge.EdgeId)
			|| !ExpectedNodeIds.Contains(Edge.FromNodeId)
			|| !ExpectedNodeIds.Contains(Edge.ToNodeId))
		{
			return FailIncompleteSceneBinding(TEXT("SceneFloorEdgeIdentityInvalid"));
		}
		ExpectedEdgeIds.Add(Edge.EdgeId);
		OutgoingCountsByNodeId.FindOrAdd(Edge.FromNodeId) += 1;
		const TWeakObjectPtr<AWacomRunPathSegmentActor>* Path =
			PathsByEdgeId.Find(Edge.EdgeId);
		if (!Path || !Path->IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("ScenePathMissing"));
		}
	}

	for (const TPair<FName, TWeakObjectPtr<AWacomRunPathSegmentActor>>& Pair :
		PathsByEdgeId)
	{
		if (!ExpectedEdgeIds.Contains(Pair.Key) || !Pair.Value.IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("ScenePathUnexpected"));
		}
	}

	TSet<FName> RequiredBranchEdgeIds;
	for (const FWacomMapEdgeDefinition& Edge : FloorDefinition.Edges)
	{
		if (OutgoingCountsByNodeId.FindRef(Edge.FromNodeId) > 1)
		{
			RequiredBranchEdgeIds.Add(Edge.EdgeId);
		}
	}
	for (const FName RequiredEdgeId : RequiredBranchEdgeIds)
	{
		const TWeakObjectPtr<AWacomRunPathBranchTargetActor>* Branch =
			BranchTargetsByEdgeId.Find(RequiredEdgeId);
		if (!Branch || !Branch->IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("SceneBranchTargetMissing"));
		}
	}
	for (const TPair<FName, TWeakObjectPtr<AWacomRunPathBranchTargetActor>>& Pair :
		BranchTargetsByEdgeId)
	{
		if (!RequiredBranchEdgeIds.Contains(Pair.Key) || !Pair.Value.IsValid())
		{
			return FailIncompleteSceneBinding(TEXT("SceneBranchTargetUnexpected"));
		}
	}

	return FWacomStatus::Ok();
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

TArray<AWacomRunPathBranchTargetActor*>
FWacomRunSceneBindingRegistry::GetBranchTargets() const
{
	TArray<FName> EdgeIds;
	BranchTargetsByEdgeId.GenerateKeyArray(EdgeIds);
	EdgeIds.Sort(
		[](const FName Left, const FName Right)
		{
			return Left.LexicalLess(Right);
		});

	TArray<AWacomRunPathBranchTargetActor*> Targets;
	Targets.Reserve(EdgeIds.Num());
	for (const FName EdgeId : EdgeIds)
	{
		if (const TWeakObjectPtr<AWacomRunPathBranchTargetActor>* Target =
			BranchTargetsByEdgeId.Find(EdgeId);
			Target && Target->IsValid())
		{
			Targets.Add(Target->Get());
		}
	}
	return Targets;
}

bool FWacomRunSceneBindingRegistry::RequiresContentHost(const EWacomMapNodeType NodeType)
{
	return NodeType != EWacomMapNodeType::Navigation;
}
