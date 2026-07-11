// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHandPresentationController.h"

#include "UI/Battle/WacomBattleCardPresentationHelper.h"

namespace
{
	EWacomFirstPersonCardLayerFrameCommitMode ResolveBattleHandPresentationFrameCommitMode(
		const FWacomFirstPersonCardLayerPresentationFrame& Frame)
	{
		return Frame.HasPresentationHints()
			? EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame
			: EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	}

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

	TArray<FGuid> CollectNewHandAnchorCardIdsForBattleHandPresentation(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot)
	{
		TArray<FGuid> Result;
		TSet<FGuid> SeenIds;
		for (const FHandCardSnapshot& CardSnapshot : NextSnapshot.Hand.Cards)
		{
			if (!CardSnapshot.InstanceId.IsValid()
				|| !CardSnapshot.bIsHandAnchor
				|| SeenIds.Contains(CardSnapshot.InstanceId)
				|| ContainsHandCardIdForBattleHandPresentation(PreviousSnapshot, CardSnapshot.InstanceId))
			{
				continue;
			}

			SeenIds.Add(CardSnapshot.InstanceId);
			Result.Add(CardSnapshot.InstanceId);
		}
		return Result;
	}

	FBattleSnapshot BuildSnapshotWithoutHandCardIdsForBattleHandPresentation(
		const FBattleSnapshot& Snapshot,
		const TArray<FGuid>& CardInstanceIds)
	{
		if (CardInstanceIds.IsEmpty())
		{
			return Snapshot;
		}

		TSet<FGuid> HiddenIds;
		HiddenIds.Reserve(CardInstanceIds.Num());
		for (const FGuid& CardInstanceId : CardInstanceIds)
		{
			if (CardInstanceId.IsValid())
			{
				HiddenIds.Add(CardInstanceId);
			}
		}
		if (HiddenIds.IsEmpty())
		{
			return Snapshot;
		}

		FBattleSnapshot Result = Snapshot;
		Result.Hand.Cards.RemoveAll(
			[&HiddenIds](const FHandCardSnapshot& CardSnapshot)
			{
				return HiddenIds.Contains(CardSnapshot.InstanceId);
			});
		Result.Hand.NormalCardCount = 0;
		for (const FHandCardSnapshot& CardSnapshot : Result.Hand.Cards)
		{
			if (!CardSnapshot.bIsHandAnchor)
			{
				++Result.Hand.NormalCardCount;
			}
		}
		return Result;
	}

	TArray<FGuid> SortCardIdsByHandSnapshotOrder(
		const FBattleSnapshot& Snapshot,
		const TArray<FGuid>& CardInstanceIds)
	{
		TArray<FGuid> Result;
		Result.Reserve(CardInstanceIds.Num());

		TSet<FGuid> PendingCardIds;
		PendingCardIds.Reserve(CardInstanceIds.Num());
		for (const FGuid& CardInstanceId : CardInstanceIds)
		{
			if (CardInstanceId.IsValid())
			{
				PendingCardIds.Add(CardInstanceId);
			}
		}

		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (PendingCardIds.Remove(CardSnapshot.InstanceId) > 0)
			{
				Result.Add(CardSnapshot.InstanceId);
			}
		}

		return Result;
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
	ClearPendingHandAnchorEnterFrame();
	bHasLastPresentedSnapshot = false;
	bHasTransitionSnapshot = false;
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

void FWacomBattleHandPresentationController::ClearPendingTransitionEvents()
{
	PendingTransitionEvents.Reset();
	PendingPlayCommitHints.Reset();
	ClearPendingHandAnchorEnterFrame();
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

void FWacomBattleHandPresentationController::RecordPlayCommit(
	const FGuid& CardInstanceId,
	const TOptional<FVector2D>& TargetWidgetPosition)
{
	if (!CardInstanceId.IsValid())
	{
		return;
	}

	FPlayCommitHint CommitHint;
	CommitHint.CardInstanceId = CardInstanceId;
	CommitHint.TargetWidgetPosition = TargetWidgetPosition;
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

bool FWacomBattleHandPresentationController::HasPendingHandAnchorEnterFrame() const
{
	return bHasPendingHandAnchorEnterFrame && !PendingHandAnchorEnterCardIds.IsEmpty();
}

FWacomFirstPersonCardLayerPresentationFrame
FWacomBattleHandPresentationController::ConsumePendingHandAnchorEnterFrame()
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	if (!HasPendingHandAnchorEnterFrame())
	{
		ClearPendingHandAnchorEnterFrame();
		return Frame;
	}

	const TArray<FGuid> EnterCardIds = SortCardIdsByHandSnapshotOrder(
		PendingHandAnchorEnterSnapshot,
		PendingHandAnchorEnterCardIds);
	Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(PendingHandAnchorEnterSnapshot);
	Frame.TransitionHints.Reserve(EnterCardIds.Num());
	const int32 SequenceCount = EnterCardIds.Num();
	for (int32 Index = 0; Index < EnterCardIds.Num(); ++Index)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = EnterCardIds[Index];
		Hint.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered;
		Hint.SequenceIndex = Index;
		Hint.SequenceCount = FMath::Max(1, SequenceCount);
		Frame.TransitionHints.Add(Hint);
	}
	Frame.CommitMode = ResolveBattleHandPresentationFrameCommitMode(Frame);
	MarkSnapshotPresented(PendingHandAnchorEnterSnapshot);
	ClearPendingHandAnchorEnterFrame();
	return Frame;
}

FWacomFirstPersonCardLayerPresentationFrame FWacomBattleHandPresentationController::BuildFrame(
	const FBattleSnapshot& Snapshot,
	bool bSuppressed)
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
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
		const TArray<FGuid> NewHandAnchorCardIds =
			CollectNewHandAnchorCardIdsForBattleHandPresentation(Baseline, Snapshot);
		const FBattleSnapshot FrameSnapshot =
			BuildSnapshotWithoutHandCardIdsForBattleHandPresentation(Snapshot, NewHandAnchorCardIds);
		Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(FrameSnapshot);
		Frame.TransitionHints = BuildTransitionHints(Baseline, FrameSnapshot);
		Frame.CommitMode = ResolveBattleHandPresentationFrameCommitMode(Frame);
		if (Frame.CommitMode == EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame)
		{
			RecordSubmittedTransitionFrame();
		}
		if (!NewHandAnchorCardIds.IsEmpty())
		{
			StorePendingHandAnchorEnterFrame(Snapshot, NewHandAnchorCardIds);
		}
		MarkSnapshotPresented(FrameSnapshot);
		PendingTransitionEvents.Reset();
		PendingPlayCommitHints.Reset();
		bUseEmptyHandSnapshotForNextTransitionRefresh = false;
		return Frame;
	}

	Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(Snapshot);
	Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	MarkSnapshotPresented(Snapshot);
	return Frame;
}

FWacomFirstPersonCardLayerPresentationFrame FWacomBattleHandPresentationController::BuildExplicitFrame(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(Snapshot);
	Frame.TransitionHints = TransitionHints;
	// An explicit frame is a replacement contract even when the hint channel is empty.
	// Treating it as a state refresh would leave an older deferred hint alive in the layer.
	Frame.CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame;
	RecordSubmittedTransitionFrame();
	MarkSnapshotPresented(Snapshot);
	ClearPendingTransitionEvents();
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
			if (const FPlayCommitHint* CommitHint = FindCommitHint(CardInstanceId))
			{
				Hint.bPlayCommitFeedback = true;
				if (CommitHint->TargetWidgetPosition.IsSet())
				{
					Hint.bHasPlayedExitTargetWidgetPosition = true;
					Hint.PlayedExitTargetWidgetPosition = CommitHint->TargetWidgetPosition.GetValue();
				}
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
			// Card acquisition remains a rule/log event. It has no dedicated hand-layer visual.
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
			|| !ContainsNormalHandCardIdForBattleHandPresentation(NextSnapshot, CardInstanceId))
		{
			continue;
		}

		SeenExactDrawnCardIds.Add(CardInstanceId);
		VisibleExactDrawnCardIds.Add(CardInstanceId);
		NewCardIds.Remove(CardInstanceId);
	}

	VisibleExactDrawnCardIds = SortCardIdsByHandSnapshotOrder(NextSnapshot, VisibleExactDrawnCardIds);
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
			|| CardSnapshot.bIsHandAnchor
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

	TArray<FWacomFirstPersonCardLayerTransitionHint*> DiscardHints;
	for (FWacomFirstPersonCardLayerTransitionHint& Hint : Hints)
	{
		if (Hint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Discarded)
		{
			DiscardHints.Add(&Hint);
		}
	}
	const int32 DiscardSequenceCount = DiscardHints.Num();
	for (int32 Index = 0; Index < DiscardHints.Num(); ++Index)
	{
		DiscardHints[Index]->SequenceIndex = Index;
		DiscardHints[Index]->SequenceCount = FMath::Max(1, DiscardSequenceCount);
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

void FWacomBattleHandPresentationController::StorePendingHandAnchorEnterFrame(
	const FBattleSnapshot& Snapshot,
	const TArray<FGuid>& CardInstanceIds)
{
	PendingHandAnchorEnterSnapshot = Snapshot;
	PendingHandAnchorEnterCardIds = CardInstanceIds;
	bHasPendingHandAnchorEnterFrame = !PendingHandAnchorEnterCardIds.IsEmpty();
}

void FWacomBattleHandPresentationController::ClearPendingHandAnchorEnterFrame()
{
	PendingHandAnchorEnterSnapshot = FBattleSnapshot();
	PendingHandAnchorEnterCardIds.Reset();
	bHasPendingHandAnchorEnterFrame = false;
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
