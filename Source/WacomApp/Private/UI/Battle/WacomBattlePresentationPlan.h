// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

enum class EWacomBattlePresentationPhaseKind : uint8
{
	None,
	TurnEndDiscard,
	TurnEndRetain,
	EnemyAction,
	TurnStartDraw,
	TurnStartHandAnchorEnter,
	CommandHandResolution,
	DeckReshuffle
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

	bool HasHandFrame() const
	{
		return Kind == EWacomBattlePresentationPhaseKind::TurnEndDiscard
			|| Kind == EWacomBattlePresentationPhaseKind::TurnEndRetain
			|| Kind == EWacomBattlePresentationPhaseKind::TurnStartDraw
			|| Kind == EWacomBattlePresentationPhaseKind::TurnStartHandAnchorEnter
			|| Kind == EWacomBattlePresentationPhaseKind::CommandHandResolution
			|| Kind == EWacomBattlePresentationPhaseKind::DeckReshuffle;
	}

	bool HasEventQueue() const
	{
		return Kind == EWacomBattlePresentationPhaseKind::EnemyAction && Events.Num() > 0;
	}
};

struct FWacomBattlePresentationPlan
{
	TArray<FWacomBattlePresentationPhase> Phases;

	bool IsEmpty() const
	{
		return Phases.IsEmpty();
	}
};
