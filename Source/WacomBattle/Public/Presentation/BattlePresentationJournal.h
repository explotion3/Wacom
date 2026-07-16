// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"

/**
 * Battle presentation checkpoint type.
 *
 * Checkpoints are rule-resolved presentation boundaries. They do not pause
 * rules, but they give WacomApp exact intermediate snapshots for later
 * presentation planning.
 */
enum class EBattlePresentationCheckpointType : uint8
{
	None,
	TurnEndDiscardResolved,
	TurnEndRetainResolved,
	TurnStartDrawResolved,
	CardGainedResolved
};

enum class EBattlePresentationDeckStepKind : uint8
{
	DrawBatch,
	DiscardPileReshuffledIntoDraw
};

/** Ordered deck-operation fact used to reconstruct intermediate UI presentation. */
struct WACOMBATTLE_API FBattlePresentationDeckStep
{
	EBattlePresentationDeckStepKind Kind = EBattlePresentationDeckStepKind::DrawBatch;
	TArray<FGuid> CardInstanceIds;
	int32 EventSequence = INDEX_NONE;
	int32 DrawPileCountAfter = INDEX_NONE;
	int32 DiscardPileCountAfter = INDEX_NONE;
};

/**
 * One resolved presentation checkpoint.
 *
 * Snapshot is the full battle snapshot after this checkpoint has resolved.
 * CardInstanceIds are the normal hand card ids relevant to this checkpoint,
 * ordered by the rule step that produced them.
 */
struct WACOMBATTLE_API FBattlePresentationCheckpoint
{
	EBattlePresentationCheckpointType Type = EBattlePresentationCheckpointType::None;

	FBattleSnapshot Snapshot;

	TArray<FGuid> CardInstanceIds;

	int32 FirstEventSequence = INDEX_NONE;

	int32 LastEventSequence = INDEX_NONE;
};

/**
 * Per-command presentation journal consumed once by WacomApp.
 */
struct WACOMBATTLE_API FBattlePresentationJournal
{
	TArray<FBattlePresentationCheckpoint> Checkpoints;
	TArray<FBattlePresentationDeckStep> DeckSteps;

	bool IsEmpty() const
	{
		return Checkpoints.IsEmpty() && DeckSteps.IsEmpty();
	}

	void Reset()
	{
		Checkpoints.Reset();
		DeckSteps.Reset();
	}

	void AddCheckpoint(
		EBattlePresentationCheckpointType Type,
		const FBattleSnapshot& Snapshot,
		const TArray<FGuid>& CardInstanceIds,
		int32 FirstEventSequence,
		int32 LastEventSequence)
	{
		FBattlePresentationCheckpoint Checkpoint;
		Checkpoint.Type = Type;
		Checkpoint.Snapshot = Snapshot;
		Checkpoint.CardInstanceIds = CardInstanceIds;
		Checkpoint.FirstEventSequence = FirstEventSequence;
		Checkpoint.LastEventSequence = LastEventSequence;
		Checkpoints.Add(MoveTemp(Checkpoint));
	}

	void AppendDeckStepsFromEvents(const TArray<FBattleEvent>& Events)
	{
		for (const FBattleEvent& Event : Events)
		{
			EBattlePresentationDeckStepKind Kind;
			if (Event.Type == EBattleEventType::CardsDrawn)
			{
				Kind = EBattlePresentationDeckStepKind::DrawBatch;
			}
			else if (Event.Type == EBattleEventType::DiscardPileReshuffledIntoDraw)
			{
				Kind = EBattlePresentationDeckStepKind::DiscardPileReshuffledIntoDraw;
			}
			else
			{
				continue;
			}

			FBattlePresentationDeckStep& Step = DeckSteps.AddDefaulted_GetRef();
			Step.Kind = Kind;
			Step.CardInstanceIds = Event.CardInstanceIds;
			Step.EventSequence = Event.Sequence;
			Step.DrawPileCountAfter = Event.DrawPileCountAfter;
			Step.DiscardPileCountAfter = Event.DiscardPileCountAfter;
		}
	}
};
