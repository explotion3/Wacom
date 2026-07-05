// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHandPresentationController.h"

#include "UI/Battle/WacomBattleCardPresentationHelper.h"

namespace
{
	bool ContainsHandCardIdForBattleHandPresentation(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		if (!CardInstanceId.IsValid())
		{
			return false;
		}

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId)
			{
				return true;
			}
		}
		return false;
	}

	bool ContainsNormalHandCardIdForBattleHandPresentation(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		if (!CardInstanceId.IsValid())
		{
			return false;
		}

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId
				&& !CardSnapshot.bIsHandAnchor)
			{
				return true;
			}
		}
		return false;
	}
}

void FWacomBattleHandPresentationController::Reset()
{
	LastPresentedSnapshot = FBattleSnapshot();
	LastTransitionSnapshot = FBattleSnapshot();
	PendingTransitionEvents.Reset();
	PendingPlayCommitHints.Reset();
	SubmittedTransitionEvents.Reset();
	SubmittedPlayCommitHints.Reset();
	bHasLastPresentedSnapshot = false;
	bHasTransitionSnapshot = false;
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

void FWacomBattleHandPresentationController::ClearPendingTransitionEvents()
{
	PendingTransitionEvents.Reset();
	PendingPlayCommitHints.Reset();
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

void FWacomBattleHandPresentationController::StoreTransitionEvents(const TArray<FBattleEvent>& Events)
{
	for (const FBattleEvent& Event : Events)
	{
		switch (Event.Type)
		{
		case EBattleEventType::CardsDrawn:
		case EBattleEventType::CardGained:
		case EBattleEventType::CardPlayed:
		case EBattleEventType::HandLimitDiscarded:
		case EBattleEventType::CardDiscarded:
		case EBattleEventType::CardExhausted:
		case EBattleEventType::CardsRetained:
			PendingTransitionEvents.Add(Event);
			break;
		default:
			break;
		}
	}
}

void FWacomBattleHandPresentationController::RecordPlayCommit(const FGuid& CardInstanceId)
{
	if (!CardInstanceId.IsValid())
	{
		return;
	}

	FPlayCommitHint CommitHint;
	CommitHint.CardInstanceId = CardInstanceId;
	PendingPlayCommitHints.Add(CommitHint);
}

bool FWacomBattleHandPresentationController::HasPendingTransitionPresentation() const
{
	return !PendingTransitionEvents.IsEmpty()
		|| !PendingPlayCommitHints.IsEmpty();
}

void FWacomBattleHandPresentationController::PreservePendingEntryRevealForNextRefresh()
{
	RestoreSubmittedEntryRevealEventsIfNeeded();
	bUseEmptyHandSnapshotForNextTransitionRefresh = true;
}

void FWacomBattleHandPresentationController::DiscardSubmittedTransitionFrame()
{
	SubmittedTransitionEvents.Reset();
	SubmittedPlayCommitHints.Reset();
}

FWacomBattleHandPresentationFrame FWacomBattleHandPresentationController::BuildFrame(
	const FBattleSnapshot& Snapshot,
	bool bSuppressed)
{
	FWacomBattleHandPresentationFrame Frame;
	if (bSuppressed)
	{
		PreservePendingEntryRevealForNextRefresh();
		return Frame;
	}

	if (HasPendingTransitionPresentation())
	{
		const FBattleSnapshot Baseline =
			(bUseEmptyHandSnapshotForNextTransitionRefresh || !bHasLastPresentedSnapshot)
			? BuildEmptyHandBaseline(Snapshot)
			: LastPresentedSnapshot;
		Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(Snapshot);
		Frame.TransitionHints = BuildTransitionHints(Baseline, Snapshot);
		Frame.FeedbackHints = BuildFeedbackHints(Snapshot);
		Frame.bHasTransitionFrame =
			Frame.TransitionHints.Num() > 0
			|| Frame.FeedbackHints.Num() > 0;
		if (Frame.bHasTransitionFrame)
		{
			RecordSubmittedTransitionFrame();
		}
		MarkSnapshotPresented(Snapshot);
		ClearPendingTransitionEvents();
		return Frame;
	}

	Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(Snapshot);
	MarkSnapshotPresented(Snapshot);
	return Frame;
}

FWacomBattleHandPresentationFrame FWacomBattleHandPresentationController::BuildExplicitFrame(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
{
	FWacomBattleHandPresentationFrame Frame;
	Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(Snapshot);
	Frame.TransitionHints = TransitionHints;
	Frame.FeedbackHints = FeedbackHints;
	Frame.bHasTransitionFrame =
		Frame.TransitionHints.Num() > 0
		|| Frame.FeedbackHints.Num() > 0;
	if (Frame.bHasTransitionFrame)
	{
		RecordSubmittedTransitionFrame();
	}
	MarkSnapshotPresented(Snapshot);
	if (Frame.bHasTransitionFrame)
	{
		ClearPendingTransitionEvents();
	}
	return Frame;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomBattleHandPresentationController::BuildTransitionHints(
	const FBattleSnapshot& PreviousSnapshot,
	const FBattleSnapshot& NextSnapshot) const
{
	TArray<FWacomFirstPersonCardLayerTransitionHint> Hints;
	if (PendingTransitionEvents.IsEmpty()
		&& PendingPlayCommitHints.IsEmpty())
	{
		return Hints;
	}

	TSet<FGuid> NewCardIds;
	for (const FHandCardSnapshot& CardSnapshot : NextSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId.IsValid()
			&& !ContainsHandCardIdForBattleHandPresentation(PreviousSnapshot, CardSnapshot.InstanceId))
		{
			NewCardIds.Add(CardSnapshot.InstanceId);
		}
	}

	TSet<FGuid> RemovedCardIds;
	for (const FHandCardSnapshot& CardSnapshot : PreviousSnapshot.Hand.Cards)
	{
		if (CardSnapshot.InstanceId.IsValid()
			&& !ContainsHandCardIdForBattleHandPresentation(NextSnapshot, CardSnapshot.InstanceId))
		{
			RemovedCardIds.Add(CardSnapshot.InstanceId);
		}
	}

	auto FindCommitHint = [this](const FGuid& CardInstanceId) -> const FPlayCommitHint*
	{
		return PendingPlayCommitHints.FindByPredicate(
			[&CardInstanceId](const FPlayCommitHint& CommitHint)
			{
				return CommitHint.CardInstanceId == CardInstanceId;
			});
	};

	TSet<FGuid> HintedCardIds;
	auto AddHint = [&Hints, &FindCommitHint, &HintedCardIds](
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind,
		int32 SequenceIndex = 0,
		int32 SequenceCount = 1)
	{
		if (!CardInstanceId.IsValid()
			|| HintedCardIds.Contains(CardInstanceId)
			|| TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Default)
		{
			return;
		}

		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = TransitionKind;
		Hint.SequenceIndex = FMath::Max(0, SequenceIndex);
		Hint.SequenceCount = FMath::Max(1, SequenceCount);
		if (TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Played)
		{
			if (FindCommitHint(CardInstanceId))
			{
				Hint.bPlayCommitFeedback = true;
			}
		}
		Hints.Add(Hint);
		HintedCardIds.Add(CardInstanceId);
	};

	TArray<FGuid> ExactDrawnCardIds;
	int32 FallbackDrawnCardHintBudget = 0;
	for (const FBattleEvent& Event : PendingTransitionEvents)
	{
		switch (Event.Type)
		{
		case EBattleEventType::CardGained:
			if (NewCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Gained);
				NewCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::CardPlayed:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Played);
				RemovedCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::HandLimitDiscarded:
		case EBattleEventType::CardDiscarded:
		case EBattleEventType::CardExhausted:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Discarded);
				RemovedCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::CardsDrawn:
			if (!Event.CardInstanceIds.IsEmpty())
			{
				ExactDrawnCardIds.Append(Event.CardInstanceIds);
			}
			else
			{
				FallbackDrawnCardHintBudget += FMath::Max(0, Event.Count);
			}
			break;
		default:
			break;
		}
	}

	TArray<FGuid> VisibleExactDrawnCardIds;
	TSet<FGuid> SeenExactDrawnCardIds;
	for (const FGuid& CardInstanceId : ExactDrawnCardIds)
	{
		if (!CardInstanceId.IsValid()
			|| SeenExactDrawnCardIds.Contains(CardInstanceId)
			|| HintedCardIds.Contains(CardInstanceId)
			|| !ContainsHandCardIdForBattleHandPresentation(NextSnapshot, CardInstanceId))
		{
			continue;
		}

		SeenExactDrawnCardIds.Add(CardInstanceId);
		VisibleExactDrawnCardIds.Add(CardInstanceId);
		NewCardIds.Remove(CardInstanceId);
	}

	const int32 ExactDrawnCardHintCount = VisibleExactDrawnCardIds.Num();
	for (int32 Index = 0; Index < VisibleExactDrawnCardIds.Num(); ++Index)
	{
		AddHint(
			VisibleExactDrawnCardIds[Index],
			EWacomFirstPersonCardSlotTransitionKind::Drawn,
			Index,
			ExactDrawnCardHintCount);
	}

	TArray<FGuid> FallbackDrawnCardIds;
	for (const FHandCardSnapshot& CardSnapshot : NextSnapshot.Hand.Cards)
	{
		if (FallbackDrawnCardHintBudget <= 0)
		{
			break;
		}
		if (!NewCardIds.Contains(CardSnapshot.InstanceId)
			|| HintedCardIds.Contains(CardSnapshot.InstanceId))
		{
			continue;
		}

		FallbackDrawnCardIds.Add(CardSnapshot.InstanceId);
		NewCardIds.Remove(CardSnapshot.InstanceId);
		--FallbackDrawnCardHintBudget;
	}

	const int32 FallbackDrawnCardHintCount = FallbackDrawnCardIds.Num();
	for (int32 Index = 0; Index < FallbackDrawnCardIds.Num(); ++Index)
	{
		AddHint(
			FallbackDrawnCardIds[Index],
			EWacomFirstPersonCardSlotTransitionKind::Drawn,
			Index,
			FallbackDrawnCardHintCount);
	}

	return Hints;
}

TArray<FWacomFirstPersonCardLayerFeedbackHint>
FWacomBattleHandPresentationController::BuildFeedbackHints(
	const FBattleSnapshot& NextSnapshot) const
{
	TArray<FGuid> RetainedCardIds;
	TSet<FGuid> SeenRetainedCardIds;
	for (const FBattleEvent& Event : PendingTransitionEvents)
	{
		if (Event.Type != EBattleEventType::CardsRetained)
		{
			continue;
		}

		for (const FGuid& CardInstanceId : Event.CardInstanceIds)
		{
			if (!CardInstanceId.IsValid()
				|| SeenRetainedCardIds.Contains(CardInstanceId)
				|| !ContainsNormalHandCardIdForBattleHandPresentation(NextSnapshot, CardInstanceId))
			{
				continue;
			}

			SeenRetainedCardIds.Add(CardInstanceId);
			RetainedCardIds.Add(CardInstanceId);
		}
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints;
	Hints.Reserve(RetainedCardIds.Num());
	const int32 SequenceCount = RetainedCardIds.Num();
	for (int32 Index = 0; Index < RetainedCardIds.Num(); ++Index)
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = RetainedCardIds[Index];
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
		Hint.SequenceIndex = Index;
		Hint.SequenceCount = FMath::Max(1, SequenceCount);
		Hints.Add(Hint);
	}
	return Hints;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomBattleHandPresentationController::BuildTransitionHintsForRefresh(
	const FBattleSnapshot& NextSnapshot) const
{
	if (bUseEmptyHandSnapshotForNextTransitionRefresh
		&& NextSnapshot.Phase != EBattlePhase::BattleEnd)
	{
		return BuildTransitionHints(BuildEmptyHandBaseline(NextSnapshot), NextSnapshot);
	}

	if (!bHasTransitionSnapshot
		|| LastTransitionSnapshot.Phase == EBattlePhase::BattleEnd
		|| NextSnapshot.Phase == EBattlePhase::BattleEnd)
	{
		return TArray<FWacomFirstPersonCardLayerTransitionHint>();
	}

	return BuildTransitionHints(LastTransitionSnapshot, NextSnapshot);
}

void FWacomBattleHandPresentationController::SetTransitionSnapshot(const FBattleSnapshot& Snapshot)
{
	LastTransitionSnapshot = Snapshot;
	bHasTransitionSnapshot = true;
}

void FWacomBattleHandPresentationController::ClearTransitionSnapshot()
{
	LastTransitionSnapshot = FBattleSnapshot();
	bHasTransitionSnapshot = false;
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

FBattleSnapshot FWacomBattleHandPresentationController::BuildEmptyHandBaseline(
	const FBattleSnapshot& Snapshot) const
{
	FBattleSnapshot EmptyHandBaseline = Snapshot;
	EmptyHandBaseline.Hand.Cards.Reset();
	EmptyHandBaseline.Hand.NormalCardCount = 0;
	return EmptyHandBaseline;
}

void FWacomBattleHandPresentationController::MarkSnapshotPresented(const FBattleSnapshot& Snapshot)
{
	LastPresentedSnapshot = Snapshot;
	bHasLastPresentedSnapshot = true;
	SetTransitionSnapshot(Snapshot);
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

void FWacomBattleHandPresentationController::RecordSubmittedTransitionFrame()
{
	SubmittedTransitionEvents = PendingTransitionEvents;
	SubmittedPlayCommitHints = PendingPlayCommitHints;
}

void FWacomBattleHandPresentationController::RestoreSubmittedEntryRevealEventsIfNeeded()
{
	if (!PendingTransitionEvents.IsEmpty() || SubmittedTransitionEvents.IsEmpty())
	{
		return;
	}

	for (const FBattleEvent& Event : SubmittedTransitionEvents)
	{
		if (Event.Type == EBattleEventType::CardsDrawn
			|| Event.Type == EBattleEventType::CardGained)
		{
			PendingTransitionEvents.Add(Event);
		}
	}

	if (!PendingTransitionEvents.IsEmpty())
	{
		PendingPlayCommitHints = SubmittedPlayCommitHints;
	}
}
