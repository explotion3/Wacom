// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleDrawPileFeedbackController.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

enum class EWacomBattlePresentationPhaseKind : uint8
{
	None,
	TurnEndDiscard,
	TurnEndRetain,
	EnemyAction,
	TurnStartDraw,
	TurnStartHandAnchorEnter,
	TurnStartRetainRelease,
	CommandHandResolution,
	HandDiscardGlyphTransfer,
	DeckReshuffle,
	CommandCardGained,
	CommandSourceOut,
	CommandPrimaryTarget,
	CommandOutcome,
	CommandSourceReturn,
	CommandBlockingDialog
};

enum class EWacomBattlePresentationPhaseCompletionPolicy : uint8
{
	PlaybackIdle,
	HandTargetImpactPeak,
	EventQueue
};

struct FWacomBattlePresentationPhase
{
	EWacomBattlePresentationPhaseKind Kind = EWacomBattlePresentationPhaseKind::None;
	FBattleSnapshot Snapshot;
	TArray<FWacomFirstPersonCardLayerTransitionHint> TransitionHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints;
	TArray<FBattleEvent> Events;
	int32 PresentationStackEntryId = INDEX_NONE;
	TArray<FWacomFirstPersonCardPileTransferHint> PileTransferHints;
	int32 PileTransferInitialDrawCount = 0;
	int32 PileTransferInitialDiscardCount = 0;
	int32 PileTransferFinalDrawCount = 0;
	int32 PileTransferFinalDiscardCount = 0;
	int32 PileTransferPlayedCount = 0;
	TOptional<FWacomBattleDrawPileFeedbackBatch> DrawPileFeedbackBatch;
	TOptional<FWacomBattlePresentationTargetCue> TargetCue;
	EWacomBattlePresentationPhaseCompletionPolicy CompletionPolicy =
		EWacomBattlePresentationPhaseCompletionPolicy::PlaybackIdle;
	FGuid CompletionCardInstanceId;
	int32 OrderingSequence = INDEX_NONE;
	bool bTargetAlreadyConfirmed = false;

	bool HasHandFrame() const
	{
		return Kind == EWacomBattlePresentationPhaseKind::TurnEndDiscard
			|| Kind == EWacomBattlePresentationPhaseKind::TurnEndRetain
			|| Kind == EWacomBattlePresentationPhaseKind::TurnStartDraw
			|| Kind == EWacomBattlePresentationPhaseKind::TurnStartHandAnchorEnter
			|| Kind == EWacomBattlePresentationPhaseKind::TurnStartRetainRelease
			|| Kind == EWacomBattlePresentationPhaseKind::CommandHandResolution
			|| Kind == EWacomBattlePresentationPhaseKind::HandDiscardGlyphTransfer
			|| Kind == EWacomBattlePresentationPhaseKind::DeckReshuffle
			|| Kind == EWacomBattlePresentationPhaseKind::CommandCardGained
			|| Kind == EWacomBattlePresentationPhaseKind::CommandSourceOut
			|| Kind == EWacomBattlePresentationPhaseKind::CommandPrimaryTarget
			|| Kind == EWacomBattlePresentationPhaseKind::CommandOutcome
			|| Kind == EWacomBattlePresentationPhaseKind::CommandSourceReturn;
	}

	bool HasEventQueue() const
	{
		return (Kind == EWacomBattlePresentationPhaseKind::EnemyAction
			|| Kind == EWacomBattlePresentationPhaseKind::CommandOutcome
			|| Kind == EWacomBattlePresentationPhaseKind::CommandBlockingDialog)
			&& Events.Num() > 0;
	}

	bool HasTargetCue() const
	{
		return TargetCue.IsSet();
	}
};

struct FWacomBattlePresentationPlan
{
	TArray<FWacomBattlePresentationPhase> Phases;
	int32 CompletionStackEntryId = INDEX_NONE;

	bool IsEmpty() const
	{
		return Phases.IsEmpty();
	}
};
