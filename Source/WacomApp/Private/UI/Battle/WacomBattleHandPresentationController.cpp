// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHandPresentationController.h"

#include "UI/Battle/WacomBattleCardChangeFeedbackPolicy.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleEffectBadgeFeedbackBuilder.h"

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

	const FHandCardSnapshot* FindNormalHandCardSnapshotForPresentation(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId && !CardSnapshot.bIsHandAnchor)
			{
				return &CardSnapshot;
			}
		}
		return nullptr;
	}

	const FHandCardSnapshot* FindVisibleHandCardSnapshotForDataRewritePresentation(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId == CardInstanceId)
			{
				return &CardSnapshot;
			}
		}
		return nullptr;
	}
}

void FWacomBattleHandPresentationController::Reset()
{
	LastPresentedSnapshot = FBattleSnapshot();
	LastTransitionSnapshot = FBattleSnapshot();
	PendingTransitionEvents.Reset();
	PendingCardDataChangeEvents.Reset();
	PendingPlayCommitHints.Reset();
	PendingHandTargetImpactIds.Reset();
	SubmittedTransitionEvents.Reset();
	SubmittedPlayCommitHints.Reset();
	SubmittedHandTargetImpactIds.Reset();
	ClearPendingHandAnchorEnterFrame();
	bHasLastPresentedSnapshot = false;
	bHasTransitionSnapshot = false;
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

void FWacomBattleHandPresentationController::ClearPendingTransitionEvents()
{
	PendingTransitionEvents.Reset();
	PendingCardDataChangeEvents.Reset();
	PendingPlayCommitHints.Reset();
	PendingHandTargetImpactIds.Reset();
	ClearPendingHandAnchorEnterFrame();
	bUseEmptyHandSnapshotForNextTransitionRefresh = false;
}

void FWacomBattleHandPresentationController::StoreTransitionEvents(const TArray<FBattleEvent>& Events)
{
	for (const FBattleEvent& Event : Events)
	{
		if (WacomBattleCardChangeFeedbackPolicy::LicensesEffectBadgeRewrite(
			Event.Type))
		{
			PendingCardDataChangeEvents.Add(Event);
		}
		switch (Event.Type)
		{
		case EBattleEventType::CardsDrawn:
		case EBattleEventType::CardGained:
		case EBattleEventType::CardPlayed:
		case EBattleEventType::CardPlayDestinationResolved:
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

void FWacomBattleHandPresentationController::RecordHandTargetImpact(
	const FGuid& TargetCardInstanceId)
{
	if (TargetCardInstanceId.IsValid())
	{
		PendingHandTargetImpactIds.AddUnique(TargetCardInstanceId);
	}
}

bool FWacomBattleHandPresentationController::HasPendingTransitionPresentation() const
{
	return !PendingTransitionEvents.IsEmpty()
		|| !PendingCardDataChangeEvents.IsEmpty()
		|| !PendingPlayCommitHints.IsEmpty()
		|| !PendingHandTargetImpactIds.IsEmpty();
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
	SubmittedHandTargetImpactIds.Reset();
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
		Frame.FeedbackHints = BuildFeedbackHints(FrameSnapshot);
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
		PendingCardDataChangeEvents.Reset();
		PendingPlayCommitHints.Reset();
		PendingHandTargetImpactIds.Reset();
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
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
{
	FWacomFirstPersonCardLayerPresentationFrame Frame;
	Frame.Entries = WacomBattleCardPresentation::BuildCardLayerEntries(Snapshot);
	Frame.TransitionHints = TransitionHints;
	Frame.FeedbackHints = FeedbackHints;
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

	TMap<FGuid, ECardLocation> ResolvedPlayedDestinations;
	for (const FBattleEvent& Event : PendingTransitionEvents)
	{
		if (Event.Type == EBattleEventType::CardPlayDestinationResolved
			&& Event.CardInstanceId.IsValid())
		{
			ResolvedPlayedDestinations.Add(Event.CardInstanceId, Event.CardDestination);
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
			if (NewCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Gained);
				NewCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::CardPlayed:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				const ECardLocation* Destination =
					ResolvedPlayedDestinations.Find(Event.CardInstanceId);
				AddHint(
					Event.CardInstanceId,
					Destination && *Destination == ECardLocation::Exhaust
						? EWacomFirstPersonCardSlotTransitionKind::Exhausted
						: EWacomFirstPersonCardSlotTransitionKind::Played);
				RemovedCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::HandLimitDiscarded:
		case EBattleEventType::CardDiscarded:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Discarded);
				RemovedCardIds.Remove(Event.CardInstanceId);
			}
			break;
		case EBattleEventType::CardExhausted:
			if (RemovedCardIds.Contains(Event.CardInstanceId))
			{
				AddHint(Event.CardInstanceId, EWacomFirstPersonCardSlotTransitionKind::Exhausted);
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

TArray<FWacomFirstPersonCardLayerFeedbackHint>
FWacomBattleHandPresentationController::BuildFeedbackHints(
	const FBattleSnapshot& NextSnapshot) const
{
	TArray<FGuid> CardUseReformIds;
	TSet<FGuid> SeenCardUseReformIds;
	for (const FBattleEvent& Event : PendingTransitionEvents)
	{
		if (Event.Type != EBattleEventType::CardPlayed
			|| !Event.CardInstanceId.IsValid()
			|| SeenCardUseReformIds.Contains(Event.CardInstanceId)
			|| !ContainsNormalHandCardIdForBattleHandPresentation(
				NextSnapshot,
				Event.CardInstanceId))
		{
			continue;
		}

		const bool bHasAcceptedPlayCommit = PendingPlayCommitHints.ContainsByPredicate(
			[&Event](const FPlayCommitHint& CommitHint)
			{
				return CommitHint.CardInstanceId == Event.CardInstanceId;
			});
		if (!bHasAcceptedPlayCommit)
		{
			continue;
		}

		SeenCardUseReformIds.Add(Event.CardInstanceId);
		CardUseReformIds.Add(Event.CardInstanceId);
	}

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
				|| SeenCardUseReformIds.Contains(CardInstanceId)
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
	for (const FGuid& CardInstanceId : PendingHandTargetImpactIds)
	{
		if (!CardInstanceId.IsValid())
		{
			continue;
		}
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
		Hints.Add(Hint);
	}
	for (const FGuid& CardInstanceId : CardUseReformIds)
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
		Hints.Add(Hint);
	}
	for (int32 Index = 0; Index < RetainedCardIds.Num(); ++Index)
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = RetainedCardIds[Index];
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
		Hint.SequenceIndex = Index;
		Hint.SequenceCount = FMath::Max(1, RetainedCardIds.Num());
		Hints.Add(Hint);
	}

	if (bHasLastPresentedSnapshot && !PendingCardDataChangeEvents.IsEmpty())
	{
		TMap<FGuid, int32> LastRelevantSequenceByCard;
		for (const FBattleEvent& Event : PendingCardDataChangeEvents)
		{
			if (Event.CardInstanceId.IsValid())
			{
				int32& LastSequence = LastRelevantSequenceByCard.FindOrAdd(Event.CardInstanceId);
				LastSequence = FMath::Max(LastSequence, Event.Sequence);
			}
		}

		TArray<const FHandCardSnapshot*> ChangedCards;
		for (const FHandCardSnapshot& NextCard : NextSnapshot.Hand.Cards)
		{
			if (!LastRelevantSequenceByCard.Contains(NextCard.InstanceId)
				|| SeenCardUseReformIds.Contains(NextCard.InstanceId))
			{
				continue;
			}
			const FHandCardSnapshot* PreviousCard =
				FindVisibleHandCardSnapshotForDataRewritePresentation(
					LastPresentedSnapshot,
					NextCard.InstanceId);
			if (PreviousCard && PreviousCard->RuntimeCost != NextCard.RuntimeCost)
			{
				ChangedCards.Add(&NextCard);
			}
		}

		for (int32 Index = 0; Index < ChangedCards.Num(); ++Index)
		{
			const FHandCardSnapshot& NextCard = *ChangedCards[Index];
			const FHandCardSnapshot* PreviousCard =
				FindVisibleHandCardSnapshotForDataRewritePresentation(
					LastPresentedSnapshot,
					NextCard.InstanceId);
			if (!PreviousCard)
			{
				continue;
			}

			FWacomFirstPersonCardLayerFeedbackHint Hint;
			Hint.CardInstanceId = NextCard.InstanceId;
			Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
			Hint.SequenceIndex = Index;
			Hint.SequenceCount = FMath::Max(1, ChangedCards.Num());
			Hint.DataRewriteFieldMask =
				static_cast<int32>(EWacomFirstPersonCardDataRewriteField::Cost);
			Hint.DataRewriteTone = NextCard.RuntimeCost < PreviousCard->RuntimeCost
				? EWacomFirstPersonCardDataRewriteTone::Beneficial
				: EWacomFirstPersonCardDataRewriteTone::Detrimental;
			Hint.bHasDataRewriteCostValues = true;
			Hint.DataRewriteCostBefore = PreviousCard->RuntimeCost;
			Hint.DataRewriteCostAfter = NextCard.RuntimeCost;
			const int32 LastSequence = LastRelevantSequenceByCard.FindRef(NextCard.InstanceId);
			Hint.DataRewriteSeed = static_cast<int32>(HashCombineFast(
				GetTypeHash(NextCard.InstanceId),
				HashCombineFast(
					GetTypeHash(LastSequence),
					GetTypeHash(Hint.DataRewriteFieldMask))));
			Hints.Add(Hint);
		}

		TArray<FWacomFirstPersonCardLayerFeedbackHint> BadgeHints;
		for (const FHandCardSnapshot& NextCard : NextSnapshot.Hand.Cards)
		{
			if (NextCard.bIsHandAnchor
				|| !LastRelevantSequenceByCard.Contains(NextCard.InstanceId)
				|| SeenCardUseReformIds.Contains(NextCard.InstanceId))
			{
				continue;
			}
			const FHandCardSnapshot* PreviousCard =
				FindNormalHandCardSnapshotForPresentation(LastPresentedSnapshot, NextCard.InstanceId);
			if (!PreviousCard)
			{
				continue;
			}
			TArray<FWacomFirstPersonCardEffectBadgeChange> Changes =
				WacomBattleEffectBadgeFeedback::BuildVisibleValueChanges(
					*PreviousCard,
					NextCard,
					LastRelevantSequenceByCard.FindRef(NextCard.InstanceId));
			if (Changes.IsEmpty())
			{
				continue;
			}
			FWacomFirstPersonCardLayerFeedbackHint& Hint = BadgeHints.AddDefaulted_GetRef();
			Hint.CardInstanceId = NextCard.InstanceId;
			Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::EffectBadgeChange;
			Hint.EffectBadgeChanges = MoveTemp(Changes);
		}
		for (int32 Index = 0; Index < BadgeHints.Num(); ++Index)
		{
			BadgeHints[Index].SequenceIndex = Index;
			BadgeHints[Index].SequenceCount = FMath::Max(1, BadgeHints.Num());
			Hints.Add(MoveTemp(BadgeHints[Index]));
		}
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
	SubmittedHandTargetImpactIds = PendingHandTargetImpactIds;
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
		PendingHandTargetImpactIds = SubmittedHandTargetImpactIds;
	}
}
