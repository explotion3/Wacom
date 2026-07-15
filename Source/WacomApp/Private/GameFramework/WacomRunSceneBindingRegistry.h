// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "Types/WacomResult.h"

class AActor;
class AWacomRunMapNodeAnchorActor;
class AWacomRunPathSegmentActor;

struct FWacomRunTraversalSceneBinding
{
	FWacomStatus Status;
	FWacomMapEdgeHandle Edge;
	FWacomMapNodeHandle SourceNode;
	FWacomMapNodeHandle TargetNode;
	EWacomMapNodeType TargetNodeType = EWacomMapNodeType::Navigation;
	TWeakObjectPtr<AWacomRunPathSegmentActor> Path;
	TWeakObjectPtr<AWacomRunMapNodeAnchorActor> SourceAnchor;
	TWeakObjectPtr<AWacomRunMapNodeAnchorActor> TargetAnchor;
	TWeakObjectPtr<AActor> ContentHost;
	FTransform CachedSourceTransform = FTransform::Identity;
	FTransform CachedTargetTransform = FTransform::Identity;

	bool IsOk() const { return Status.IsOk(); }
};

/** 当前 Floor scoped 的 App runtime cache；不生成或修改 Floor DataAsset。 */
class FWacomRunSceneBindingRegistry
{
public:
	void Reset(FName InFloorId = NAME_None);
	FName GetFloorId() const { return FloorId; }

	bool RegisterPath(AWacomRunPathSegmentActor& Path);
	bool RegisterNodeAnchor(AWacomRunMapNodeAnchorActor& Anchor);
	bool RegisterContentHost(FName NodeId, EWacomMapNodeType NodeType, AActor& Host);

	void UnregisterPath(const AWacomRunPathSegmentActor& Path);
	void UnregisterNodeAnchor(const AWacomRunMapNodeAnchorActor& Anchor);
	void UnregisterContentHost(const AActor& Host);

	FWacomRunTraversalSceneBinding PreflightTraversal(
		const FRunExplorationSnapshot& Snapshot,
		FName EdgeId) const;

	FWacomStatus RevalidateTarget(
		const FWacomMapNodeHandle& TargetNode,
		EWacomMapNodeType TargetNodeType,
		AWacomRunMapNodeAnchorActor*& OutAnchor,
		AActor*& OutContentHost) const;

	AWacomRunMapNodeAnchorActor* FindNodeAnchor(FName NodeId) const;

private:
	struct FContentHostBinding
	{
		EWacomMapNodeType NodeType = EWacomMapNodeType::Navigation;
		TWeakObjectPtr<AActor> Host;
	};

	static bool RequiresContentHost(EWacomMapNodeType NodeType);

	FName FloorId = NAME_None;
	TMap<FName, TWeakObjectPtr<AWacomRunPathSegmentActor>> PathsByEdgeId;
	TMap<FName, TWeakObjectPtr<AWacomRunMapNodeAnchorActor>> AnchorsByNodeId;
	TMap<FName, FContentHostBinding> ContentHostsByNodeId;
};
