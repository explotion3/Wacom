// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "Types/WacomResult.h"

struct FRunState;

/** Session-private owner of one mutually exclusive node activity and its AP reservation. */
class FRunNodeActivityModule
{
public:
	static FWacomStatus Begin(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& PendingActivity,
		ERunNodeActivityKind Kind,
		int32 ReservedActionPoints,
		FRunNodeActivityTicket& OutTicket,
		TArray<FRunExplorationEvent>& OutEvents);

	static FWacomStatus Complete(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& PendingActivity,
		const FRunNodeActivityTicket& Ticket,
		int32 ActionPointCost,
		bool bResolveNode,
		TArray<FRunExplorationEvent>& OutEvents);

	static FWacomStatus Cancel(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& PendingActivity,
		const FRunNodeActivityTicket& Ticket,
		TArray<FRunExplorationEvent>& OutEvents);

	/**
	 * 在 activity 内提交行动点成本；未跨 phase 时重签同 token 的 ticket 并继续占有，
	 * 跨 phase 时结束 activity。bDeferPhaseAdvance 为 true 时，归零也继续占有，
	 * 由 activity 的关闭路径负责推进阶段。
	 */
	static FWacomStatus SpendAndContinue(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& PendingActivity,
		const FRunNodeActivityTicket& Ticket,
		int32 ActionPointCost,
		bool bResolveNode,
		bool bDeferPhaseAdvance,
		bool& bOutActivityContinues,
		FRunNodeActivityTicket& OutUpdatedTicket,
		TArray<FRunExplorationEvent>& OutEvents);

	/** 单次世界交互内完成 Begin + 结算，只增加一次 exploration version。 */
	static FWacomStatus ResolveImmediate(
		FRunState& State,
		TOptional<FRunNodeActivityTicket>& PendingActivity,
		ERunNodeActivityKind Kind,
		int32 ActionPointCost,
		bool bResolveNode,
		TArray<FRunExplorationEvent>& OutEvents);

	static bool Matches(
		const FRunState& State,
		const TOptional<FRunNodeActivityTicket>& PendingActivity,
		const FRunNodeActivityTicket& Ticket);

private:
	static EWacomMapNodeType ExpectedNodeType(ERunNodeActivityKind Kind);
};
