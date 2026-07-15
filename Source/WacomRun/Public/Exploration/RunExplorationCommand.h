// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"

/** 唯一探索命令入口的意图类型。 */
enum class ERunExplorationCommandType : uint8
{
	BeginTraversal,
	CompleteTraversal,
	CancelTraversal,
	MapTravel,
	ChooseNightExploration,
	BeginCamp,
	CancelCamp,
	RequestFloorTransition,
	ConfirmFloorTransition,
	CancelFloorTransition,
};

/**
 * C++-only command。调用方只提供稳定身份、expected version 和规则签发的 token，
 * 不允许反向提供 source/target、成本或 lifecycle 目标值。
 */
struct WACOMRUN_API FRunExplorationCommand
{
	ERunExplorationCommandType Type = ERunExplorationCommandType::BeginTraversal;
	int32 ExpectedVersion = 0;
	FWacomMapEdgeHandle Edge;
	FWacomMapNodeHandle Node;
	FRunExplorationToken Token;

	static FRunExplorationCommand BeginTraversal(const FWacomMapEdgeHandle& InEdge, int32 InExpectedVersion);
	static FRunExplorationCommand CompleteTraversal(const FRunTraversalTicket& Ticket);
	static FRunExplorationCommand CancelTraversal(const FRunTraversalTicket& Ticket);
	static FRunExplorationCommand MapTravel(const FWacomMapNodeHandle& InNode, int32 InExpectedVersion);
	static FRunExplorationCommand ChooseNightExploration(int32 InExpectedVersion);
	static FRunExplorationCommand BeginCamp(int32 InExpectedVersion);
	static FRunExplorationCommand CancelCamp(const FRunCampTicket& Ticket);
	static FRunExplorationCommand RequestFloorTransition(int32 InExpectedVersion);
	static FRunExplorationCommand ConfirmFloorTransition(
		const FRunFloorTransitionConfirmation& Confirmation);
	static FRunExplorationCommand CancelFloorTransition(
		const FRunFloorTransitionConfirmation& Confirmation);
};
