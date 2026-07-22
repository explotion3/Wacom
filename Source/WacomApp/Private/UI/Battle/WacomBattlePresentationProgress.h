// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EWacomBattlePresentationProgressKind : uint8
{
	PlanStarted,
	PhaseEventsReached,
	EnemyActionStarted,
	EnemyActionImpact,
	TurnAdvanced,
	PlanCompleted,
	PlanCancelled,
};

enum class EWacomBattlePresentationCancelPolicy : uint8
{
	FlushPending,
	DiscardPending,
};

/** App-private semantic progress emitted by the single Battle presentation clock. */
struct FWacomBattlePresentationProgress
{
	uint64 ActivityTransactionId = 0;
	EWacomBattlePresentationProgressKind Kind =
		EWacomBattlePresentationProgressKind::PlanStarted;
	TArray<int32> EventSequences;
	int32 FirstEventSequence = INDEX_NONE;
	int32 LastEventSequence = INDEX_NONE;
	int32 PresentedTurnNumber = INDEX_NONE;
	EWacomBattlePresentationCancelPolicy CancelPolicy =
		EWacomBattlePresentationCancelPolicy::FlushPending;
};
