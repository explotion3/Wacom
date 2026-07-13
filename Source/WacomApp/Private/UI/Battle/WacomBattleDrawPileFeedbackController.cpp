// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleDrawPileFeedbackController.h"

#include "Events/BattleEvent.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Common/PileCountView.h"

namespace
{
	bool ContainsAnyCardId(
		const FWacomBattleDrawPileFeedbackBatch& Batch,
		const TSet<FGuid>& CardIds)
	{
		return Batch.CardInstanceIds.ContainsByPredicate(
			[&CardIds](const FGuid& CardId)
			{
				return CardIds.Contains(CardId);
			});
	}
}

FWacomBattleDrawPileFeedbackController::FWacomBattleDrawPileFeedbackController(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleDrawPileFeedbackController::QueueBatchesFromEvents(
	const TArray<FBattleEvent>& Events)
{
	for (const FBattleEvent& Event : Events)
	{
		if (Event.Type != EBattleEventType::CardsDrawn
			|| Event.CardInstanceIds.IsEmpty()
			|| Event.DrawPileCountAfter < 0)
		{
			continue;
		}

		FWacomBattleDrawPileFeedbackBatch Batch;
		Batch.EventSequence = Event.Sequence;
		Batch.CardInstanceIds = Event.CardInstanceIds;
		Batch.DrawPileCountAfter = Event.DrawPileCountAfter;
		Batch.DrawPileCountBefore = Event.DrawPileCountAfter + Event.CardInstanceIds.Num();
		QueueBatch(Batch);
	}
}

void FWacomBattleDrawPileFeedbackController::QueueBatch(
	const FWacomBattleDrawPileFeedbackBatch& Batch)
{
	if (!Batch.IsValid() || IsDuplicateBatch(Batch))
	{
		return;
	}
	PendingBatches.Add(Batch);
}

void FWacomBattleDrawPileFeedbackController::PrepareForPresentationFrame(
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	TSet<FGuid> DrawnCardIds;
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : TransitionHints)
	{
		if (Hint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Drawn
			&& Hint.CardInstanceId.IsValid())
		{
			DrawnCardIds.Add(Hint.CardInstanceId);
		}
	}
	if (DrawnCardIds.IsEmpty() || PendingBatches.IsEmpty())
	{
		return;
	}

	int32 FirstMatchingIndex = INDEX_NONE;
	int32 LastMatchingIndex = INDEX_NONE;
	for (int32 Index = 0; Index < PendingBatches.Num(); ++Index)
	{
		if (!ContainsAnyCardId(PendingBatches[Index], DrawnCardIds))
		{
			continue;
		}
		if (FirstMatchingIndex == INDEX_NONE)
		{
			FirstMatchingIndex = Index;
		}
		LastMatchingIndex = Index;
	}
	if (FirstMatchingIndex == INDEX_NONE)
	{
		return;
	}

	CompleteActiveBatch();
	FActiveBatch NewActiveBatch;
	NewActiveBatch.Batch = PendingBatches[FirstMatchingIndex];
	for (int32 Index = FirstMatchingIndex; Index <= LastMatchingIndex; ++Index)
	{
		if (PendingBatches[Index].EventSequence != INDEX_NONE)
		{
			NewActiveBatch.EventSequences.Add(PendingBatches[Index].EventSequence);
		}
		if (Index > FirstMatchingIndex)
		{
			NewActiveBatch.Batch.CardInstanceIds.Append(PendingBatches[Index].CardInstanceIds);
			NewActiveBatch.Batch.DrawPileCountAfter = PendingBatches[Index].DrawPileCountAfter;
		}
	}
	for (const FGuid& CardId : NewActiveBatch.Batch.CardInstanceIds)
	{
		if (DrawnCardIds.Contains(CardId))
		{
			NewActiveBatch.VisibleCardIds.Add(CardId);
		}
	}
	PendingBatches.RemoveAt(
		FirstMatchingIndex,
		LastMatchingIndex - FirstMatchingIndex + 1,
		EAllowShrinking::No);
	if (NewActiveBatch.VisibleCardIds.IsEmpty())
	{
		if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
		{
			DrawPileView->SetCount(NewActiveBatch.Batch.DrawPileCountAfter);
		}
		return;
	}

	ActiveBatch = MoveTemp(NewActiveBatch);
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->ResetSendFeedback();
		DrawPileView->SetCount(ActiveBatch->Batch.DrawPileCountBefore);
	}
}

void FWacomBattleDrawPileFeedbackController::HandleEnterTransitionStarted(
	const FWacomFirstPersonCardEnterTransitionStartedView& View)
{
	if (View.TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Drawn
		|| !View.CardInstanceId.IsValid()
		|| !ActiveBatch.IsSet()
		|| !ActiveBatch->VisibleCardIds.Contains(View.CardInstanceId)
		|| ActiveBatch->StartedCardIds.Contains(View.CardInstanceId))
	{
		return;
	}

	ActiveBatch->StartedCardIds.Add(View.CardInstanceId);
	const bool bFinalDeparture =
		ActiveBatch->StartedCardIds.Num() >= ActiveBatch->VisibleCardIds.Num();
	const int32 DisplayCount = bFinalDeparture
		? ActiveBatch->Batch.DrawPileCountAfter
		: FMath::Max(
			ActiveBatch->Batch.DrawPileCountAfter,
			ActiveBatch->Batch.DrawPileCountBefore - ActiveBatch->StartedCardIds.Num());

	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->SetCount(DisplayCount);
		DrawPileView->PlaySendFeedback(
			1,
			bFinalDeparture,
			View.TargetWidgetPosition - View.StartWidgetPosition,
			false);
	}

	if (bFinalDeparture)
	{
		for (const int32 EventSequence : ActiveBatch->EventSequences)
		{
			CompletedEventSequences.Add(EventSequence);
		}
		ActiveBatch.Reset();
	}
}

void FWacomBattleDrawPileFeedbackController::CompleteActiveBatch()
{
	if (!ActiveBatch.IsSet())
	{
		return;
	}
	RestoreActiveBatchFinalCount();
	for (const int32 EventSequence : ActiveBatch->EventSequences)
	{
		CompletedEventSequences.Add(EventSequence);
	}
	ActiveBatch.Reset();
}

void FWacomBattleDrawPileFeedbackController::Reset(int32 AuthoritativeDrawPileCount)
{
	PendingBatches.Reset();
	ActiveBatch.Reset();
	CompletedEventSequences.Reset();
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->ResetSendFeedback();
		if (AuthoritativeDrawPileCount >= 0)
		{
			DrawPileView->SetCount(AuthoritativeDrawPileCount);
		}
	}
}

void FWacomBattleDrawPileFeedbackController::RestoreActiveBatchFinalCount()
{
	if (!ActiveBatch.IsSet())
	{
		return;
	}
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->ResetSendFeedback();
		DrawPileView->SetCount(ActiveBatch->Batch.DrawPileCountAfter);
	}
}

bool FWacomBattleDrawPileFeedbackController::IsDuplicateBatch(
	const FWacomBattleDrawPileFeedbackBatch& Batch) const
{
	if (Batch.EventSequence != INDEX_NONE
		&& CompletedEventSequences.Contains(Batch.EventSequence))
	{
		return true;
	}
	const auto Matches = [&Batch](const FWacomBattleDrawPileFeedbackBatch& Existing)
	{
		return Existing.EventSequence == Batch.EventSequence
			&& Existing.CardInstanceIds == Batch.CardInstanceIds
			&& Existing.DrawPileCountBefore == Batch.DrawPileCountBefore
			&& Existing.DrawPileCountAfter == Batch.DrawPileCountAfter;
	};
	if (PendingBatches.ContainsByPredicate(Matches))
	{
		return true;
	}
	return ActiveBatch.IsSet()
		&& ((Batch.EventSequence != INDEX_NONE
				&& ActiveBatch->EventSequences.Contains(Batch.EventSequence))
			|| Matches(ActiveBatch->Batch));
}

#if WITH_AUTOMATION_TESTS
int32 FWacomBattleDrawPileFeedbackController::GetActiveVisibleCardCountForTest() const
{
	return ActiveBatch.IsSet() ? ActiveBatch->VisibleCardIds.Num() : 0;
}

int32 FWacomBattleDrawPileFeedbackController::GetActiveStartedCardCountForTest() const
{
	return ActiveBatch.IsSet() ? ActiveBatch->StartedCardIds.Num() : 0;
}
#endif
