// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomBattleHUDRuntime;
struct FBattleEvent;

struct FWacomBattleDrawPileFeedbackBatch
{
	int32 EventSequence = INDEX_NONE;
	TArray<FGuid> CardInstanceIds;
	int32 DrawPileCountBefore = INDEX_NONE;
	int32 DrawPileCountAfter = INDEX_NONE;

	bool IsValid() const
	{
		return !CardInstanceIds.IsEmpty()
			&& DrawPileCountBefore >= 0
			&& DrawPileCountAfter >= 0
			&& DrawPileCountBefore >= DrawPileCountAfter;
	}
};

/**
 * App-private presentation owner for DrawPileView's temporary per-card count and send feedback.
 * It consumes Battle draw facts but never mutates Battle state.
 */
class FWacomBattleDrawPileFeedbackController
{
public:
	explicit FWacomBattleDrawPileFeedbackController(FWacomBattleHUDRuntime& InRuntime);

	void QueueBatchesFromEvents(const TArray<FBattleEvent>& Events);
	void QueueBatch(const FWacomBattleDrawPileFeedbackBatch& Batch);
	void PrepareForPresentationFrame(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	void HandleEnterTransitionStarted(
		const FWacomFirstPersonCardEnterTransitionStartedView& View);
	void CompleteActiveBatch();
	void Reset(int32 AuthoritativeDrawPileCount = INDEX_NONE);

#if WITH_AUTOMATION_TESTS
	int32 GetPendingBatchCountForTest() const { return PendingBatches.Num(); }
	int32 GetActiveVisibleCardCountForTest() const;
	int32 GetActiveStartedCardCountForTest() const;
#endif

private:
	struct FActiveBatch
	{
		FWacomBattleDrawPileFeedbackBatch Batch;
		TSet<FGuid> VisibleCardIds;
		TSet<FGuid> StartedCardIds;
		TSet<int32> EventSequences;
	};

	FWacomBattleHUDRuntime& Runtime;
	TArray<FWacomBattleDrawPileFeedbackBatch> PendingBatches;
	TOptional<FActiveBatch> ActiveBatch;
	TSet<int32> CompletedEventSequences;

	void RestoreActiveBatchFinalCount();
	bool IsDuplicateBatch(const FWacomBattleDrawPileFeedbackBatch& Batch) const;
};
