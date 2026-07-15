// Copyright Wacom. All Rights Reserved.

#include "UI/Map/WacomRunMapViewDataBuilder.h"

#define LOCTEXT_NAMESPACE "WacomRunMapViewData"

namespace
{
	constexpr double MapCanvasWidth = 1920.0;
	constexpr double MapCanvasHeight = 1080.0;

	FText BuildNodeTypeLabel(const EWacomMapNodeType NodeType)
	{
		switch (NodeType)
		{
		case EWacomMapNodeType::Navigation:
			return LOCTEXT("Navigation", "道路");
		case EWacomMapNodeType::Encounter:
			return LOCTEXT("Encounter", "遭遇");
		case EWacomMapNodeType::RunEvent:
			return LOCTEXT("RunEvent", "事件");
		case EWacomMapNodeType::Shop:
			return LOCTEXT("Shop", "商店");
		case EWacomMapNodeType::Treasure:
			return LOCTEXT("Treasure", "宝箱");
		case EWacomMapNodeType::FloorEntrance:
			return LOCTEXT("FloorEntrance", "层级入口");
		default:
			return FText::GetEmpty();
		}
	}

	EWacomRunMapNodeVisualState BuildVisualState(const FRunFloorMapNodeSnapshot& Node)
	{
		if (Node.bIsCurrentNode)
		{
			return EWacomRunMapNodeVisualState::Current;
		}
		if (Node.bLandmarkOnly)
		{
			return EWacomRunMapNodeVisualState::Landmark;
		}
		switch (Node.Lifecycle)
		{
		case ERunMapNodeLifecycle::Resolved:
			return EWacomRunMapNodeVisualState::Resolved;
		case ERunMapNodeLifecycle::Visited:
			return EWacomRunMapNodeVisualState::Visited;
		case ERunMapNodeLifecycle::Revealed:
			return EWacomRunMapNodeVisualState::Revealed;
		case ERunMapNodeLifecycle::Hidden:
		default:
			return EWacomRunMapNodeVisualState::Landmark;
		}
	}

	FText BuildDisabledReason(
		const FRunFloorMapNodeSnapshot& Node,
		const ERunExplorationActivityKind ActiveActivityKind)
	{
		if (Node.bIsCurrentNode)
		{
			return LOCTEXT("CurrentNode", "当前位置");
		}
		if (ActiveActivityKind != ERunExplorationActivityKind::None)
		{
			return LOCTEXT("ActivityActive", "当前探索事务尚未结束");
		}
		if (Node.bLandmarkOnly)
		{
			return LOCTEXT("LandmarkOnly", "尚未探索");
		}
		if (Node.Lifecycle != ERunMapNodeLifecycle::Resolved)
		{
			return LOCTEXT("NodeNotResolved", "完成节点后可传送");
		}
		return FText::GetEmpty();
	}

	FVector2D ClampDesignPosition(const FVector2D Position)
	{
		return FVector2D(
			FMath::Clamp(Position.X, 0.0, MapCanvasWidth),
			FMath::Clamp(Position.Y, 0.0, MapCanvasHeight));
	}
}

FWacomRunMapScreenViewData FWacomRunMapViewDataBuilder::Build(
	const FRunFloorMapSnapshot& Snapshot,
	const TOptional<FWacomMapNodeHandle> RequestedSelection,
	const bool bPreferRecommendedTarget,
	const FText& StatusOverride)
{
	FWacomRunMapScreenViewData ViewData;
	ViewData.StateVersion = Snapshot.StateVersion;
	ViewData.FloorId = Snapshot.FloorId;
	ViewData.FloorTitle = Snapshot.FloorDisplayName;
	ViewData.CurrentNode = Snapshot.CurrentNode;
	ViewData.bIsAvailable = Snapshot.IsValid();

	TMap<FWacomMapNodeHandle, FVector2D> Positions;
	ViewData.Nodes.Reserve(Snapshot.Nodes.Num());
	for (const FRunFloorMapNodeSnapshot& Node : Snapshot.Nodes)
	{
		FWacomRunMapNodeViewData& NodeView = ViewData.Nodes.AddDefaulted_GetRef();
		NodeView.Handle = Node.Handle;
		NodeView.Title = Node.bLandmarkOnly ? LOCTEXT("UnknownLandmark", "未知地标") : Node.DisplayName;
		NodeView.Description = Node.bLandmarkOnly ? FText::GetEmpty() : Node.ShortDescription;
		NodeView.TypeLabel = Node.bLandmarkOnly ? FText::GetEmpty() : BuildNodeTypeLabel(Node.NodeType);
		NodeView.DesignPosition = ClampDesignPosition(Node.MapPosition);
		NodeView.VisualState = BuildVisualState(Node);
		NodeView.bCanSelect = true;
		NodeView.bCanTravel = Node.bCanMapTravel;
		NodeView.DisabledReason = BuildDisabledReason(Node, Snapshot.ActiveActivityKind);
		Positions.Add(Node.Handle, NodeView.DesignPosition);
	}

	ViewData.Edges.Reserve(Snapshot.Edges.Num());
	for (const FRunFloorMapEdgeSnapshot& Edge : Snapshot.Edges)
	{
		const FVector2D* SourcePosition = Positions.Find(Edge.SourceNode);
		const FVector2D* TargetPosition = Positions.Find(Edge.TargetNode);
		if (!SourcePosition || !TargetPosition)
		{
			continue;
		}
		FWacomRunMapEdgeViewData& EdgeView = ViewData.Edges.AddDefaulted_GetRef();
		EdgeView.Handle = Edge.Handle;
		EdgeView.SourceNode = Edge.SourceNode;
		EdgeView.TargetNode = Edge.TargetNode;
		EdgeView.SourceDesignPosition = *SourcePosition;
		EdgeView.TargetDesignPosition = *TargetPosition;
	}

	const auto IsSelectable = [&ViewData](const FWacomMapNodeHandle& Handle)
	{
		return ViewData.Nodes.ContainsByPredicate(
			[&Handle](const FWacomRunMapNodeViewData& Node)
			{
				return Node.Handle == Handle && Node.bCanSelect;
			});
	};

	if (RequestedSelection.IsSet() && IsSelectable(RequestedSelection.GetValue()))
	{
		ViewData.SelectedNode = RequestedSelection.GetValue();
	}
	else if (bPreferRecommendedTarget
		&& Snapshot.RecommendedTravelTarget.IsSet()
		&& IsSelectable(Snapshot.RecommendedTravelTarget.GetValue()))
	{
		ViewData.SelectedNode = Snapshot.RecommendedTravelTarget.GetValue();
	}
	else if (IsSelectable(Snapshot.CurrentNode))
	{
		ViewData.SelectedNode = Snapshot.CurrentNode;
	}
	else if (!ViewData.Nodes.IsEmpty())
	{
		ViewData.SelectedNode = ViewData.Nodes[0].Handle;
	}

	ViewData.DefaultFocusNode = ViewData.SelectedNode;
	for (FWacomRunMapNodeViewData& Node : ViewData.Nodes)
	{
		Node.bIsSelected = Node.Handle == ViewData.SelectedNode;
		if (Node.bIsSelected)
		{
			ViewData.SelectedNodeTitle = Node.Title;
			ViewData.SelectedNodeDescription = Node.Description;
			ViewData.bCanConfirmTravel = ViewData.bIsAvailable && Node.bCanTravel;
			if (StatusOverride.IsEmpty())
			{
				ViewData.StatusText = Node.DisabledReason;
			}
		}
	}

	if (!StatusOverride.IsEmpty())
	{
		ViewData.StatusText = StatusOverride;
	}
	else if (!ViewData.bIsAvailable)
	{
		ViewData.StatusText = LOCTEXT("MapUnavailable", "地图暂不可用");
	}
	return ViewData;
}

#undef LOCTEXT_NAMESPACE
