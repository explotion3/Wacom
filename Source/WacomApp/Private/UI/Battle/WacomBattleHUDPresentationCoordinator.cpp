// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"

#include "UI/Battle/WacomBattleEnemyActionPlaybackTypes.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/Widget.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleEventPresentationQueue.h"
#include "UI/Battle/WacomBattleEffectBadgeFeedbackBuilder.h"
#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDResultApplicator.h"
#include "UI/Battle/WacomBattlePileCountPresentation.h"
#include "UI/Battle/WacomBattlePresentationTimerOwner.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Common/PileCountView.h"

namespace
{
	constexpr float BattlePresentationStackExitSeconds = 0.16f;
	constexpr float BattlePresentationPlanPollSeconds = 0.03f;
	constexpr float BattlePresentationPlanHandPhaseTimeoutSeconds = 4.0f;

	const TCHAR* BattlePresentationPhaseKindToString(EWacomBattlePresentationPhaseKind Kind)
	{
		switch (Kind)
		{
		case EWacomBattlePresentationPhaseKind::TurnEndDiscard:
			return TEXT("TurnEndDiscard");
		case EWacomBattlePresentationPhaseKind::TurnEndRetain:
			return TEXT("TurnEndRetain");
		case EWacomBattlePresentationPhaseKind::EnemyAction:
			return TEXT("EnemyAction");
		case EWacomBattlePresentationPhaseKind::TurnStartDraw:
			return TEXT("TurnStartDraw");
		case EWacomBattlePresentationPhaseKind::TurnStartHandAnchorEnter:
			return TEXT("TurnStartHandAnchorEnter");
		case EWacomBattlePresentationPhaseKind::TurnStartRetainRelease:
			return TEXT("TurnStartRetainRelease");
		case EWacomBattlePresentationPhaseKind::CommandHandResolution:
			return TEXT("CommandHandResolution");
		case EWacomBattlePresentationPhaseKind::HandDiscardGlyphTransfer:
			return TEXT("HandDiscardGlyphTransfer");
		case EWacomBattlePresentationPhaseKind::DeckReshuffle:
			return TEXT("DeckReshuffle");
		case EWacomBattlePresentationPhaseKind::CommandCardGained:
			return TEXT("CommandCardGained");
		case EWacomBattlePresentationPhaseKind::CommandSourceOut:
			return TEXT("CommandSourceOut");
		case EWacomBattlePresentationPhaseKind::CommandPrimaryTarget:
			return TEXT("CommandPrimaryTarget");
		case EWacomBattlePresentationPhaseKind::CommandOutcome:
			return TEXT("CommandOutcome");
		case EWacomBattlePresentationPhaseKind::CommandSourceReturn:
			return TEXT("CommandSourceReturn");
		case EWacomBattlePresentationPhaseKind::CommandBlockingDialog:
			return TEXT("CommandBlockingDialog");
		case EWacomBattlePresentationPhaseKind::None:
		default:
			return TEXT("None");
		}
	}

	const FBattlePresentationCheckpoint* FindCheckpoint(
		const FBattlePresentationJournal& Journal,
		EBattlePresentationCheckpointType Type)
	{
		return Journal.Checkpoints.FindByPredicate(
			[Type](const FBattlePresentationCheckpoint& Checkpoint)
			{
				return Checkpoint.Type == Type;
			});
	}

	bool ContainsNormalHandCardId(
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

	bool ContainsHandCardId(
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

	TArray<FGuid> CollectHandAnchorCardIds(const FBattleSnapshot& Snapshot)
	{
		TArray<FGuid> Result;
		for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
		{
			if (CardSnapshot.InstanceId.IsValid()
				&& CardSnapshot.bIsHandAnchor)
			{
				Result.Add(CardSnapshot.InstanceId);
			}
		}
		return Result;
	}

	TArray<FGuid> CollectNewHandAnchorCardIds(
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
				|| ContainsHandCardId(PreviousSnapshot, CardSnapshot.InstanceId))
			{
				continue;
			}

			SeenIds.Add(CardSnapshot.InstanceId);
			Result.Add(CardSnapshot.InstanceId);
		}
		return Result;
	}

	FBattleSnapshot BuildSnapshotWithoutHandCardIds(
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

	TArray<FGuid> BuildUniqueValidCardIds(
		const TArray<FGuid>& CardInstanceIds,
		TFunctionRef<bool(const FGuid&)> Predicate)
	{
		TArray<FGuid> Result;
		TSet<FGuid> SeenIds;
		for (const FGuid& CardInstanceId : CardInstanceIds)
		{
			if (!CardInstanceId.IsValid()
				|| SeenIds.Contains(CardInstanceId)
				|| !Predicate(CardInstanceId))
			{
				continue;
			}

			SeenIds.Add(CardInstanceId);
			Result.Add(CardInstanceId);
		}
		return Result;
	}

	TArray<FGuid> SortCardIdsByPhaseSnapshotOrder(
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

	TArray<FGuid> BuildRetainedPhaseFeedbackCardIds(
		const FBattleSnapshot& Snapshot,
		const TArray<FGuid>& RetainedNormalCardIds)
	{
		TArray<FGuid> FeedbackCardIds = BuildUniqueValidCardIds(
			RetainedNormalCardIds,
			[&Snapshot](const FGuid& CardInstanceId)
			{
				return ContainsNormalHandCardId(Snapshot, CardInstanceId);
			});
		return SortCardIdsByPhaseSnapshotOrder(Snapshot, FeedbackCardIds);
	}

	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHintsForCardIds(
		const TArray<FGuid>& CardInstanceIds,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind)
	{
		TArray<FWacomFirstPersonCardLayerTransitionHint> Hints;
		Hints.Reserve(CardInstanceIds.Num());
		const int32 SequenceCount = CardInstanceIds.Num();
		for (int32 Index = 0; Index < CardInstanceIds.Num(); ++Index)
		{
			FWacomFirstPersonCardLayerTransitionHint Hint;
			Hint.CardInstanceId = CardInstanceIds[Index];
			Hint.TransitionKind = TransitionKind;
			Hint.SequenceIndex = Index;
			Hint.SequenceCount = FMath::Max(1, SequenceCount);
			Hints.Add(Hint);
		}
		return Hints;
	}

	bool IsGlyphTransferPhase(EWacomBattlePresentationPhaseKind Kind)
	{
		return Kind == EWacomBattlePresentationPhaseKind::DeckReshuffle
			|| Kind == EWacomBattlePresentationPhaseKind::HandDiscardGlyphTransfer
			|| Kind == EWacomBattlePresentationPhaseKind::TurnEndDiscard;
	}

	struct FHandDiscardPresentationBatch
	{
		int32 Sequence = INDEX_NONE;
		TArray<FGuid> CardInstanceIds;
		int32 DiscardPileCountAfter = 0;
	};

	TArray<FHandDiscardPresentationBatch> BuildHandDiscardPresentationBatches(
		const TArray<FBattleEvent>& Events)
	{
		TArray<FHandDiscardPresentationBatch> Result;
		TSet<int32> SeenSequences;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type != EBattleEventType::CardDiscarded || !Event.CardInstanceId.IsValid())
			{
				continue;
			}
			const int32 BatchSequence = Event.HandCardZoneMoveBatchSequence != INDEX_NONE
				? Event.HandCardZoneMoveBatchSequence
				: Event.Sequence;
			if (BatchSequence == INDEX_NONE || SeenSequences.Contains(BatchSequence))
			{
				continue;
			}
			SeenSequences.Add(BatchSequence);
			FHandDiscardPresentationBatch Batch;
			Batch.Sequence = BatchSequence;
			Batch.CardInstanceIds = Event.CardInstanceIds.IsEmpty()
				? TArray<FGuid>({ Event.CardInstanceId })
				: Event.CardInstanceIds;
			Batch.CardInstanceIds = BuildUniqueValidCardIds(
				Batch.CardInstanceIds,
				[](const FGuid&) { return true; });
			Batch.DiscardPileCountAfter = FMath::Max(0, Event.DiscardPileCountAfter);
			if (!Batch.CardInstanceIds.IsEmpty())
			{
				Result.Add(MoveTemp(Batch));
			}
		}
		Result.Sort([](const FHandDiscardPresentationBatch& A, const FHandDiscardPresentationBatch& B)
		{
			return A.Sequence < B.Sequence;
		});
		return Result;
	}

	FBattleSnapshot BuildSnapshotRestoringHandCardIds(
		const FBattleSnapshot& BaseSnapshot,
		const FBattleSnapshot& SourceSnapshot,
		const TArray<FGuid>& CardInstanceIds)
	{
		if (CardInstanceIds.IsEmpty())
		{
			return BaseSnapshot;
		}

		TSet<FGuid> RestoredIds;
		for (const FGuid& CardInstanceId : CardInstanceIds)
		{
			if (CardInstanceId.IsValid())
			{
				RestoredIds.Add(CardInstanceId);
			}
		}
		TMap<FGuid, const FHandCardSnapshot*> BaseCardsById;
		for (const FHandCardSnapshot& Card : BaseSnapshot.Hand.Cards)
		{
			BaseCardsById.Add(Card.InstanceId, &Card);
		}

		FBattleSnapshot Result = BaseSnapshot;
		Result.Hand.Cards.Reset();
		TSet<FGuid> AddedIds;
		for (const FHandCardSnapshot& SourceCard : SourceSnapshot.Hand.Cards)
		{
			if (RestoredIds.Contains(SourceCard.InstanceId))
			{
				Result.Hand.Cards.Add(SourceCard);
				AddedIds.Add(SourceCard.InstanceId);
				continue;
			}
			if (const FHandCardSnapshot* const* BaseCard = BaseCardsById.Find(SourceCard.InstanceId))
			{
				Result.Hand.Cards.Add(**BaseCard);
				AddedIds.Add(SourceCard.InstanceId);
			}
		}
		for (const FHandCardSnapshot& BaseCard : BaseSnapshot.Hand.Cards)
		{
			if (!AddedIds.Contains(BaseCard.InstanceId))
			{
				Result.Hand.Cards.Add(BaseCard);
			}
		}
		Result.Hand.NormalCardCount = 0;
		for (const FHandCardSnapshot& Card : Result.Hand.Cards)
		{
			if (!Card.bIsHandAnchor)
			{
				++Result.Hand.NormalCardCount;
			}
		}
		return Result;
	}

	bool IsImmediateEventQueuePresentationEvent(const FBattleEvent& Event)
	{
		return Event.Type == EBattleEventType::EnemyPartActed
			|| Event.Type == EBattleEventType::DamageDealt
			|| Event.Type == EBattleEventType::EnemyPartHpEmptied
			|| Event.Type == EBattleEventType::BattleEnded;
	}

	bool IsBlockingDialogPresentationEvent(const FBattleEvent& Event)
	{
		return Event.Type == EBattleEventType::KnockdownChoiceRequested;
	}

	int32 FindFirstPositiveEventSequence(const TArray<FBattleEvent>& Events)
	{
		int32 Result = INDEX_NONE;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Sequence > 0 && (Result == INDEX_NONE || Event.Sequence < Result))
			{
				Result = Event.Sequence;
			}
		}
		return Result;
	}

	const FHandCardSnapshot* FindNormalHandCardSnapshot(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		return Snapshot.Hand.Cards.FindByPredicate(
			[&CardInstanceId](const FHandCardSnapshot& Card)
			{
				return Card.InstanceId == CardInstanceId && !Card.bIsHandAnchor;
			});
	}

	const FHandCardSnapshot* FindVisibleHandCardSnapshotForDataRewrite(
		const FBattleSnapshot& Snapshot,
		const FGuid& CardInstanceId)
	{
		return Snapshot.Hand.Cards.FindByPredicate(
			[&CardInstanceId](const FHandCardSnapshot& Card)
			{
				return Card.InstanceId == CardInstanceId;
			});
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildCommandDataRewriteHints(
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		const TArray<FBattleEvent>& Events,
		const FGuid& SuppressedSourceCardId)
	{
		TMap<FGuid, int32> LastRelevantSequenceByCard;
		for (const FBattleEvent& Event : Events)
		{
			if ((Event.Type == EBattleEventType::CardRuntimeCostChanged
					|| Event.Type == EBattleEventType::CardStatusChanged)
				&& Event.CardInstanceId.IsValid())
			{
				int32& LastSequence = LastRelevantSequenceByCard.FindOrAdd(Event.CardInstanceId);
				LastSequence = FMath::Max(LastSequence, Event.Sequence);
			}
		}

		TArray<const FHandCardSnapshot*> ChangedCards;
		for (const FHandCardSnapshot& NextCard : PostCommandSnapshot.Hand.Cards)
		{
			if (NextCard.InstanceId == SuppressedSourceCardId
				|| !LastRelevantSequenceByCard.Contains(NextCard.InstanceId))
			{
				continue;
			}
			const FHandCardSnapshot* PreviousCard = FindVisibleHandCardSnapshotForDataRewrite(
				PreCommandSnapshot,
				NextCard.InstanceId);
			if (PreviousCard && PreviousCard->RuntimeCost != NextCard.RuntimeCost)
			{
				ChangedCards.Add(&NextCard);
			}
		}

		TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints;
		Hints.Reserve(ChangedCards.Num());
		for (int32 Index = 0; Index < ChangedCards.Num(); ++Index)
		{
			const FHandCardSnapshot& NextCard = *ChangedCards[Index];
			const FHandCardSnapshot* PreviousCard = FindVisibleHandCardSnapshotForDataRewrite(
				PreCommandSnapshot,
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
			Hints.Add(MoveTemp(Hint));
		}
		return Hints;
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildCommandEffectBadgeChangeHints(
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		const TArray<FBattleEvent>& Events,
		const FGuid& SuppressedSourceCardId)
	{
		TMap<FGuid, int32> LastRelevantSequenceByCard;
		for (const FBattleEvent& Event : Events)
		{
			if ((Event.Type == EBattleEventType::CardRuntimeCostChanged
					|| Event.Type == EBattleEventType::CardStatusChanged)
				&& Event.CardInstanceId.IsValid())
			{
				int32& Sequence = LastRelevantSequenceByCard.FindOrAdd(Event.CardInstanceId);
				Sequence = FMath::Max(Sequence, Event.Sequence);
			}
		}

		TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints;
		for (const FHandCardSnapshot& NextCard : PostCommandSnapshot.Hand.Cards)
		{
			if (NextCard.bIsHandAnchor
				|| NextCard.InstanceId == SuppressedSourceCardId
				|| !LastRelevantSequenceByCard.Contains(NextCard.InstanceId))
			{
				continue;
			}
			const FHandCardSnapshot* PreviousCard = FindNormalHandCardSnapshot(
				PreCommandSnapshot,
				NextCard.InstanceId);
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
			FWacomFirstPersonCardLayerFeedbackHint& Hint = Hints.AddDefaulted_GetRef();
			Hint.CardInstanceId = NextCard.InstanceId;
			Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::EffectBadgeChange;
			Hint.EffectBadgeChanges = MoveTemp(Changes);
		}
		for (int32 Index = 0; Index < Hints.Num(); ++Index)
		{
			Hints[Index].SequenceIndex = Index;
			Hints[Index].SequenceCount = FMath::Max(1, Hints.Num());
		}
		return Hints;
	}

	int32 FindFirstHandOutcomeSequence(
		const TArray<FBattleEvent>& Events,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
	{
		TSet<FGuid> OutcomeCardIds;
		for (const FWacomFirstPersonCardLayerTransitionHint& Hint : TransitionHints)
		{
			if (Hint.CardInstanceId.IsValid())
			{
				OutcomeCardIds.Add(Hint.CardInstanceId);
			}
		}
		for (const FWacomFirstPersonCardLayerFeedbackHint& Hint : FeedbackHints)
		{
			if (Hint.CardInstanceId.IsValid())
			{
				OutcomeCardIds.Add(Hint.CardInstanceId);
			}
		}

		int32 Result = INDEX_NONE;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Sequence <= 0
				|| Event.Type == EBattleEventType::CardPlayed
				|| Event.Type == EBattleEventType::CardPlayDestinationResolved
				|| Event.Type == EBattleEventType::CardDiscarded
				|| Event.Type == EBattleEventType::CardsDrawn
				|| Event.Type == EBattleEventType::DiscardPileReshuffledIntoDraw)
			{
				continue;
			}
			bool bMatchesOutcomeCard = Event.CardInstanceId.IsValid()
				&& OutcomeCardIds.Contains(Event.CardInstanceId);
			if (!bMatchesOutcomeCard)
			{
				bMatchesOutcomeCard = Event.CardInstanceIds.ContainsByPredicate(
					[&OutcomeCardIds](const FGuid& CardInstanceId)
					{
						return OutcomeCardIds.Contains(CardInstanceId);
					});
			}
			if (bMatchesOutcomeCard
				&& (Result == INDEX_NONE || Event.Sequence < Result))
			{
				Result = Event.Sequence;
			}
		}
		return Result;
	}

	void RemoveHandCards(FBattleSnapshot& Snapshot, const TArray<FGuid>& CardInstanceIds)
	{
		TSet<FGuid> RemovedIds;
		for (const FGuid& CardInstanceId : CardInstanceIds)
		{
			RemovedIds.Add(CardInstanceId);
		}
		Snapshot.Hand.Cards.RemoveAll([&RemovedIds](const FHandCardSnapshot& Card)
		{
			return RemovedIds.Contains(Card.InstanceId);
		});
		Snapshot.Hand.NormalCardCount = 0;
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (!Card.bIsHandAnchor)
			{
				++Snapshot.Hand.NormalCardCount;
			}
		}
	}

	FWacomBattlePresentationPhase MakeHandDiscardGlyphPhase(
		const FHandDiscardPresentationBatch& Batch,
		const FBattleSnapshot& Snapshot)
	{
		FWacomBattlePresentationPhase Phase;
		Phase.Kind = EWacomBattlePresentationPhaseKind::HandDiscardGlyphTransfer;
		Phase.OrderingSequence = Batch.Sequence;
		Phase.Snapshot = Snapshot;
		Phase.TransitionHints = BuildTransitionHintsForCardIds(
			Batch.CardInstanceIds,
			EWacomFirstPersonCardSlotTransitionKind::Discarded);
		Phase.PileTransferInitialDiscardCount = FMath::Max(
			0,
			Batch.DiscardPileCountAfter - Batch.CardInstanceIds.Num());
		FWacomFirstPersonCardPileTransferHint Hint;
		Hint.EventSequence = Batch.Sequence;
		Hint.CardInstanceIds = Batch.CardInstanceIds;
		Hint.TransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
		Hint.TargetAnchorKind = EWacomFirstPersonCardPresentationAnchorKind::DiscardPile;
		Hint.Seed = HashCombineFast(GetTypeHash(Batch.Sequence), GetTypeHash(Batch.CardInstanceIds.Num()));
		Phase.PileTransferHints.Add(MoveTemp(Hint));
		return Phase;
	}

	TArray<FBattleEvent> FilterEventsBySequenceRange(
		const TArray<FBattleEvent>& Events,
		int32 FirstSequence,
		int32 LastSequence)
	{
		TArray<FBattleEvent> Result;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Sequence <= 0)
			{
				continue;
			}
			if (FirstSequence != INDEX_NONE && Event.Sequence < FirstSequence)
			{
				continue;
			}
			if (LastSequence != INDEX_NONE && Event.Sequence > LastSequence)
			{
				continue;
			}

			Result.Add(Event);
		}
		return Result;
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildRetainedFeedbackHintsForCardIds(
		const TArray<FGuid>& CardInstanceIds,
		bool bRetainUntilExplicitRelease)
	{
		TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints;
		for (int32 Index = 0; Index < CardInstanceIds.Num(); ++Index)
		{
			FWacomFirstPersonCardLayerFeedbackHint Hint;
			Hint.CardInstanceId = CardInstanceIds[Index];
			Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
			Hint.SequenceIndex = Index;
			Hint.SequenceCount = FMath::Max(1, CardInstanceIds.Num());
			Hint.bRetainUntilExplicitRelease = bRetainUntilExplicitRelease;
			Hints.Add(Hint);
		}
		return Hints;
	}

	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildRetainedReleaseHintsForCardIds(
		const TArray<FGuid>& CardInstanceIds)
	{
		TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints;
		for (int32 Index = 0; Index < CardInstanceIds.Num(); ++Index)
		{
			FWacomFirstPersonCardLayerFeedbackHint Hint;
			Hint.CardInstanceId = CardInstanceIds[Index];
			Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::RetainedRelease;
			Hint.SequenceIndex = Index;
			Hint.SequenceCount = FMath::Max(1, CardInstanceIds.Num());
			Hints.Add(Hint);
		}
		return Hints;
	}

	void AppendDeckStepPhases(
		FWacomBattlePresentationPlan& Plan,
		const TArray<FBattlePresentationDeckStep>& DeckSteps,
		const FBattleSnapshot& InitialSnapshot,
		const FBattleSnapshot& FinalSnapshot,
		const TArray<FGuid>& DeferredHandAnchorIds)
	{
		if (DeckSteps.IsEmpty())
		{
			return;
		}

		TArray<FGuid> FutureDrawnIds;
		for (const FBattlePresentationDeckStep& Step : DeckSteps)
		{
			if (Step.Kind == EBattlePresentationDeckStepKind::DrawBatch)
			{
				FutureDrawnIds.Append(Step.CardInstanceIds);
			}
		}
		int32 CurrentDrawCount = InitialSnapshot.PileCounts.DrawCount;
		int32 CurrentDiscardCount = InitialSnapshot.PileCounts.DiscardCount;

		for (const FBattlePresentationDeckStep& Step : DeckSteps)
		{
			if (Step.Kind == EBattlePresentationDeckStepKind::DrawBatch)
			{
				for (const FGuid& CardId : Step.CardInstanceIds)
				{
					FutureDrawnIds.RemoveSingle(CardId);
				}
				TArray<FGuid> HiddenIds = FutureDrawnIds;
				HiddenIds.Append(DeferredHandAnchorIds);
				FWacomBattlePresentationPhase Phase;
				Phase.Kind = EWacomBattlePresentationPhaseKind::TurnStartDraw;
				Phase.OrderingSequence = Step.EventSequence;
				Phase.Snapshot = BuildSnapshotWithoutHandCardIds(FinalSnapshot, HiddenIds);
				Phase.Snapshot.PileCounts.DrawCount = Step.DrawPileCountAfter;
				Phase.Snapshot.PileCounts.DiscardCount = Step.DiscardPileCountAfter;
				const TArray<FGuid> VisibleDrawnIds = BuildUniqueValidCardIds(
					Step.CardInstanceIds,
					[&Phase](const FGuid& CardId)
					{
						return ContainsNormalHandCardId(Phase.Snapshot, CardId);
					});
				Phase.TransitionHints = BuildTransitionHintsForCardIds(
					SortCardIdsByPhaseSnapshotOrder(Phase.Snapshot, VisibleDrawnIds),
					EWacomFirstPersonCardSlotTransitionKind::Drawn);
				if (!Phase.TransitionHints.IsEmpty())
				{
					FWacomBattleDrawPileFeedbackBatch DrawPileFeedbackBatch;
					DrawPileFeedbackBatch.EventSequence = Step.EventSequence;
					DrawPileFeedbackBatch.CardInstanceIds = Step.CardInstanceIds;
					DrawPileFeedbackBatch.DrawPileCountBefore = CurrentDrawCount;
					DrawPileFeedbackBatch.DrawPileCountAfter = Step.DrawPileCountAfter;
					Phase.DrawPileFeedbackBatch = MoveTemp(DrawPileFeedbackBatch);
					Plan.Phases.Add(MoveTemp(Phase));
				}
				CurrentDrawCount = Step.DrawPileCountAfter;
				CurrentDiscardCount = Step.DiscardPileCountAfter;
				continue;
			}

			FWacomBattlePresentationPhase Phase;
			Phase.Kind = EWacomBattlePresentationPhaseKind::DeckReshuffle;
			Phase.OrderingSequence = Step.EventSequence;
			TArray<FGuid> HiddenIds = FutureDrawnIds;
			HiddenIds.Append(DeferredHandAnchorIds);
			Phase.Snapshot = BuildSnapshotWithoutHandCardIds(FinalSnapshot, HiddenIds);
			Phase.Snapshot.PileCounts.DrawCount = CurrentDrawCount;
			Phase.Snapshot.PileCounts.DiscardCount = CurrentDiscardCount;
			Phase.PileTransferInitialDrawCount = CurrentDrawCount;
			Phase.PileTransferInitialDiscardCount = CurrentDiscardCount;
			Phase.PileTransferFinalDrawCount = Step.DrawPileCountAfter;
			Phase.PileTransferFinalDiscardCount = Step.DiscardPileCountAfter;
			Phase.PileTransferPlayedCount = Phase.Snapshot.PileCounts.PlayedCount;
			FWacomFirstPersonCardPileTransferHint Hint;
			Hint.EventSequence = Step.EventSequence;
			Hint.CardInstanceIds = Step.CardInstanceIds;
			Hint.TransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
			Hint.Seed = HashCombineFast(
				GetTypeHash(Step.EventSequence),
				GetTypeHash(Step.CardInstanceIds.Num()));
			Phase.PileTransferHints.Add(MoveTemp(Hint));
			Plan.Phases.Add(MoveTemp(Phase));
			CurrentDrawCount = Step.DrawPileCountAfter;
			CurrentDiscardCount = Step.DiscardPileCountAfter;
		}
	}

	TArray<FBattleEvent> FilterNonDeckPresentationEvents(const TArray<FBattleEvent>& Events)
	{
		return Events.FilterByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type != EBattleEventType::CardsDrawn
				&& Event.Type != EBattleEventType::DiscardPileReshuffledIntoDraw;
		});
	}

	FBattleSnapshot BuildSnapshotWithCombatFacts(
		const FBattleSnapshot& LayoutSnapshot,
		const FBattleSnapshot& CombatSnapshot)
	{
		FBattleSnapshot Result = LayoutSnapshot;
		Result.Player = CombatSnapshot.Player;
		Result.Enemies = CombatSnapshot.Enemies;
		return Result;
	}

	TArray<FBattlePresentationEnemyActionStep> FilterEnemyActionStepsForEvents(
		const TArray<FBattlePresentationEnemyActionStep>& Steps,
		const TArray<FBattleEvent>& Events)
	{
		TSet<int32> EnemyPartActedSequences;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::EnemyPartActed && Event.Sequence > 0)
			{
				EnemyPartActedSequences.Add(Event.Sequence);
			}
		}

		return Steps.FilterByPredicate(
			[&EnemyPartActedSequences](const FBattlePresentationEnemyActionStep& Step)
			{
				return Step.IsValid()
					&& EnemyPartActedSequences.Contains(Step.FirstEventSequence);
			});
	}

	const FBattlePresentationEnemyActionStep* FindFirstEnemyActionStep(
		const TArray<FBattlePresentationEnemyActionStep>& Steps)
	{
		return Steps.IsEmpty() ? nullptr : &Steps[0];
	}

	FWacomBattlePresentationPlan BuildEndTurnPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		FWacomBattlePresentationPlan Plan;
		if (Journal.IsEmpty())
		{
			return Plan;
		}

		const FBattlePresentationCheckpoint* DiscardCheckpoint = FindCheckpoint(
			Journal,
			EBattlePresentationCheckpointType::TurnEndDiscardResolved);
		const FBattlePresentationCheckpoint* RetainCheckpoint = FindCheckpoint(
			Journal,
			EBattlePresentationCheckpointType::TurnEndRetainResolved);
		const FBattlePresentationCheckpoint* DrawCheckpoint = FindCheckpoint(
			Journal,
			EBattlePresentationCheckpointType::TurnStartDrawResolved);

		if (DiscardCheckpoint)
		{
			const TArray<FGuid> DiscardedCardIds = BuildUniqueValidCardIds(
				DiscardCheckpoint->CardInstanceIds,
				[](const FGuid&) { return true; });
			if (!DiscardedCardIds.IsEmpty())
			{
				FWacomBattlePresentationPhase Phase;
				Phase.Kind = EWacomBattlePresentationPhaseKind::TurnEndDiscard;
				Phase.Snapshot = DiscardCheckpoint->Snapshot;
				Phase.TransitionHints = BuildTransitionHintsForCardIds(
					DiscardedCardIds,
					EWacomFirstPersonCardSlotTransitionKind::Discarded);
				const TArray<FHandDiscardPresentationBatch> DiscardBatches =
					BuildHandDiscardPresentationBatches(Events);
				FHandDiscardPresentationBatch Batch;
				if (!DiscardBatches.IsEmpty())
				{
					Batch = DiscardBatches[0];
				}
				else
				{
					Batch.Sequence = DiscardCheckpoint->FirstEventSequence;
					Batch.CardInstanceIds = DiscardedCardIds;
					Batch.DiscardPileCountAfter = DiscardCheckpoint->Snapshot.PileCounts.DiscardCount;
				}
				Phase.PileTransferInitialDiscardCount = FMath::Max(
					0,
					Batch.DiscardPileCountAfter - DiscardedCardIds.Num());
				FWacomFirstPersonCardPileTransferHint Hint;
				Hint.EventSequence = Batch.Sequence;
				Hint.CardInstanceIds = DiscardedCardIds;
				Hint.TransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
				Hint.TargetAnchorKind = EWacomFirstPersonCardPresentationAnchorKind::DiscardPile;
				Hint.Seed = HashCombineFast(GetTypeHash(Batch.Sequence), GetTypeHash(DiscardedCardIds.Num()));
				Phase.PileTransferHints.Add(MoveTemp(Hint));
				Plan.Phases.Add(MoveTemp(Phase));
			}
		}

		TArray<FGuid> RetainedSealCardIds;
		if (RetainCheckpoint)
		{
			RetainedSealCardIds = BuildRetainedPhaseFeedbackCardIds(
				RetainCheckpoint->Snapshot,
				RetainCheckpoint->CardInstanceIds);
			if (!RetainedSealCardIds.IsEmpty())
			{
				FWacomBattlePresentationPhase Phase;
				Phase.Kind = EWacomBattlePresentationPhaseKind::TurnEndRetain;
				Phase.Snapshot = RetainCheckpoint->Snapshot;
				Phase.FeedbackHints = BuildRetainedFeedbackHintsForCardIds(
					RetainedSealCardIds,
					true);
				Plan.Phases.Add(MoveTemp(Phase));
			}
		}

		const FBattlePresentationCheckpoint* LastHandCheckpointBeforeEnemy =
			RetainCheckpoint ? RetainCheckpoint : DiscardCheckpoint;
		const FBattleSnapshot& HandSnapshotBeforeDraw =
			LastHandCheckpointBeforeEnemy
			? LastHandCheckpointBeforeEnemy->Snapshot
			: PostCommandSnapshot;
		const int32 EnemyFirstSequence =
			LastHandCheckpointBeforeEnemy && LastHandCheckpointBeforeEnemy->LastEventSequence != INDEX_NONE
			? LastHandCheckpointBeforeEnemy->LastEventSequence + 1
			: INDEX_NONE;
		const int32 EnemyLastSequence =
			DrawCheckpoint && DrawCheckpoint->FirstEventSequence != INDEX_NONE
			? DrawCheckpoint->FirstEventSequence - 1
			: INDEX_NONE;
		TArray<FBattleEvent> EnemyEvents = FilterEventsBySequenceRange(
			Events,
			EnemyFirstSequence,
			EnemyLastSequence);
		if (!EnemyEvents.IsEmpty())
		{
			FWacomBattlePresentationPhase Phase;
			Phase.Kind = EWacomBattlePresentationPhaseKind::EnemyAction;
			Phase.Snapshot = LastHandCheckpointBeforeEnemy
				? LastHandCheckpointBeforeEnemy->Snapshot
				: PostCommandSnapshot;
			Phase.Events = MoveTemp(EnemyEvents);
			Phase.EnemyActionSteps = FilterEnemyActionStepsForEvents(
				Journal.EnemyActionSteps,
				Phase.Events);
			if (const FBattlePresentationEnemyActionStep* FirstActionStep =
				FindFirstEnemyActionStep(Phase.EnemyActionSteps))
			{
				Phase.Snapshot = BuildSnapshotWithCombatFacts(
					Phase.Snapshot,
					FirstActionStep->SnapshotBefore);
			}
			Plan.Phases.Add(MoveTemp(Phase));
		}

		if (DrawCheckpoint)
		{
			const TArray<FGuid> NewHandAnchorCardIds = CollectNewHandAnchorCardIds(
				HandSnapshotBeforeDraw,
				DrawCheckpoint->Snapshot);
			if (!Journal.DeckSteps.IsEmpty())
			{
				AppendDeckStepPhases(
					Plan,
					Journal.DeckSteps,
					HandSnapshotBeforeDraw,
					DrawCheckpoint->Snapshot,
					NewHandAnchorCardIds);
			}
			else
			{
				const FBattleSnapshot DrawPhaseSnapshot = BuildSnapshotWithoutHandCardIds(
					DrawCheckpoint->Snapshot,
					NewHandAnchorCardIds);
				const TArray<FGuid> DrawnCardIds = SortCardIdsByPhaseSnapshotOrder(
					DrawPhaseSnapshot,
					BuildUniqueValidCardIds(
						DrawCheckpoint->CardInstanceIds,
						[&DrawPhaseSnapshot](const FGuid& CardInstanceId)
						{
							return ContainsNormalHandCardId(DrawPhaseSnapshot, CardInstanceId);
						}));
				if (!DrawnCardIds.IsEmpty())
				{
					FWacomBattlePresentationPhase Phase;
					Phase.Kind = EWacomBattlePresentationPhaseKind::TurnStartDraw;
					Phase.Snapshot = DrawPhaseSnapshot;
					Phase.TransitionHints = BuildTransitionHintsForCardIds(
						DrawnCardIds,
						EWacomFirstPersonCardSlotTransitionKind::Drawn);
					Plan.Phases.Add(MoveTemp(Phase));
				}
			}
			if (!NewHandAnchorCardIds.IsEmpty())
			{
				FWacomBattlePresentationPhase Phase;
				Phase.Kind = EWacomBattlePresentationPhaseKind::TurnStartHandAnchorEnter;
				Phase.Snapshot = DrawCheckpoint->Snapshot;
				Phase.TransitionHints = BuildTransitionHintsForCardIds(
					SortCardIdsByPhaseSnapshotOrder(DrawCheckpoint->Snapshot, NewHandAnchorCardIds),
					EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered);
				Plan.Phases.Add(MoveTemp(Phase));
			}
		}

		if (!RetainedSealCardIds.IsEmpty()
			&& PostCommandSnapshot.Phase != EBattlePhase::BattleEnd)
		{
			const FBattleSnapshot& ReleaseSnapshot = DrawCheckpoint
				? DrawCheckpoint->Snapshot
				: PostCommandSnapshot;
			const TArray<FGuid> ReleaseCardIds = SortCardIdsByPhaseSnapshotOrder(
				ReleaseSnapshot,
				BuildUniqueValidCardIds(
					RetainedSealCardIds,
					[&ReleaseSnapshot](const FGuid& CardInstanceId)
					{
						return ContainsNormalHandCardId(ReleaseSnapshot, CardInstanceId);
					}));
			if (!ReleaseCardIds.IsEmpty())
			{
				FWacomBattlePresentationPhase Phase;
				Phase.Kind = EWacomBattlePresentationPhaseKind::TurnStartRetainRelease;
				Phase.Snapshot = ReleaseSnapshot;
				Phase.FeedbackHints = BuildRetainedReleaseHintsForCardIds(ReleaseCardIds);
				Plan.Phases.Add(MoveTemp(Phase));
			}
		}

		return Plan;
	}
}

FWacomBattleHUDPresentationCoordinator::FWacomBattleHUDPresentationCoordinator(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
	, PresentationTimerOwner(MakeShared<FWacomBattlePresentationTimerOwner>())
{
}

FWacomBattleHUDPresentationCoordinator::~FWacomBattleHUDPresentationCoordinator()
{
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->AbandonWithoutWorldAccess();
		BattleEventPresentationQueue.Reset();
	}
	if (PresentationTimerOwner)
	{
		PresentationTimerOwner->AbandonAllWithoutWorldAccess();
	}
}

void FWacomBattleHUDPresentationCoordinator::Shutdown()
{
	ClearQueue();
	if (PresentationTimerOwner)
	{
		PresentationTimerOwner->CancelAll();
	}
}

int32 FWacomBattleHUDPresentationCoordinator::AppendStackEntry(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const FBattleSnapshot& PreCommandSnapshot)
{
	if (CommandContext.CommandKind != EWacomBattleCombatLogCommandKind::PlayCard
		|| !CommandContext.CardInstanceId.IsValid())
	{
		return INDEX_NONE;
	}

	const FHandCardSnapshot* CardSnapshot = nullptr;
	for (const FHandCardSnapshot& Candidate : PreCommandSnapshot.Hand.Cards)
	{
		if (Candidate.InstanceId == CommandContext.CardInstanceId)
		{
			CardSnapshot = &Candidate;
			break;
		}
	}
	if (!CardSnapshot || !CardSnapshot->Definition)
	{
		return INDEX_NONE;
	}

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = NextBattlePresentationStackEntryId++;
	Entry.CardInstanceId = CommandContext.CardInstanceId;
	Entry.CardViewData = CommandContext.CardTargetPreview.bHasPreview
		? WacomBattleCardPresentation::BuildCardViewData(*CardSnapshot, CommandContext.CardTargetPreview)
		: WacomBattleCardPresentation::BuildCardViewData(*CardSnapshot);
	BattlePresentationStackEntries.Add(Entry);
	SyncStackWidget();
	// Stack authoring happens in the middle of command-result assembly. Refreshing
	// the first-person hand here would consume the accepted-play commit before the
	// command presentation phase has paired it with CardPlayed facts.
	RefreshCommandBarOnly();
	return Entry.EntryId;
}

void FWacomBattleHUDPresentationCoordinator::BeginStackEntryExit(int32 EntryId)
{
	if (EntryId == INDEX_NONE)
	{
		return;
	}

	FWacomBattlePresentationStackEntryView* FoundEntry = BattlePresentationStackEntries.FindByPredicate(
		[EntryId](const FWacomBattlePresentationStackEntryView& Candidate)
		{
			return Candidate.EntryId == EntryId;
		});
	if (!FoundEntry)
	{
		return;
	}

	if (!FoundEntry->bIsExiting)
	{
		FoundEntry->bIsExiting = true;
		BattlePresentationStackExitingEntryIds.AddUnique(EntryId);
		SyncStackWidget();
	}

	if (UWorld* World = GetWorld())
	{
		PresentationTimerOwner->ScheduleOnce(
			World,
			FWacomBattlePresentationTimerKey::StackEntryExit(EntryId),
			BattlePresentationStackExitSeconds,
			[this, EntryId]()
			{
				FinishStackEntryExit(EntryId);
			});
		return;
	}

	FinishStackEntryExit(EntryId);
}

void FWacomBattleHUDPresentationCoordinator::FinishStackEntryExit(int32 EntryId)
{
	if (EntryId == INDEX_NONE)
	{
		return;
	}

	PresentationTimerOwner->Cancel(
		FWacomBattlePresentationTimerKey::StackEntryExit(EntryId));
	BattlePresentationStackExitingEntryIds.Remove(EntryId);

	const int32 Removed = BattlePresentationStackEntries.RemoveAll(
		[EntryId](const FWacomBattlePresentationStackEntryView& Entry)
		{
			return Entry.EntryId == EntryId;
		});
	if (Removed <= 0)
	{
		return;
	}

	SyncStackWidget();
	RefreshCommandBar();
	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::ClearStack()
{
	PresentationTimerOwner->CancelKind(
		EWacomBattlePresentationTimerKind::StackEntryExit);
	BattlePresentationStackExitingEntryIds.Reset();
	BattlePresentationStackEntries.Reset();
	SyncStackWidget();
}

void FWacomBattleHUDPresentationCoordinator::EnqueueEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId,
	bool bTargetAlreadyConfirmed)
{
	EnqueueEvents(
		Events,
		TArray<FBattlePresentationEnemyActionStep>(),
		PresentationStackEntryId,
		bTargetAlreadyConfirmed);
}

void FWacomBattleHUDPresentationCoordinator::EnqueueEvents(
	const TArray<FBattleEvent>& Events,
	const TArray<FBattlePresentationEnemyActionStep>& EnemyActionSteps,
	int32 PresentationStackEntryId,
	bool bTargetAlreadyConfirmed)
{
	if (Events.IsEmpty())
	{
		if (PresentationStackEntryId != INDEX_NONE)
		{
			BeginStackEntryExit(PresentationStackEntryId);
		}
		return;
	}

	if (!BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue = MakeShared<FWacomBattleEventPresentationQueue>(
			*this,
			*PresentationTimerOwner);
	}

	BattleEventPresentationQueue->EnqueueEvents(
		Events,
		EnemyActionSteps,
		PresentationStackEntryId,
		Runtime.Host().GetCardPresentationStackMinimumHoldSeconds(),
		bTargetAlreadyConfirmed);
}

bool FWacomBattleHUDPresentationCoordinator::EnqueueEndTurnPresentationPlan(
	const FBattlePresentationJournal& Journal,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PostCommandSnapshot)
{
	if (Journal.IsEmpty() || bProcessingPresentationPlan)
	{
		return false;
	}

	FWacomBattlePresentationPlan NewPlan =
		BuildEndTurnPresentationPlan(Journal, Events, PostCommandSnapshot);
	if (NewPlan.IsEmpty())
	{
		return false;
	}

	ClearPresentationPlan();
	PresentationPlan = MoveTemp(NewPlan);
	bProcessingPresentationPlan = true;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bWaitingForPresentationPlanEventQueue = false;
#if WITH_AUTOMATION_TESTS
	StartedPresentationPlanPhaseNamesForTest.Reset();
#endif
	HandleQueueStarted();
	RefreshCommandBar();
	StartNextPresentationPlanPhase();
	return true;
}

bool FWacomBattleHUDPresentationCoordinator::EnqueueResolvedCommandPresentationPlan(
	const FBattlePresentationJournal& Journal,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot,
	int32 PresentationStackEntryId)
{
	const TArray<FHandDiscardPresentationBatch> DiscardBatches =
		BuildHandDiscardPresentationBatches(Events);
	TArray<const FBattlePresentationCheckpoint*> GainedCheckpoints;
	for (const FBattlePresentationCheckpoint& Checkpoint : Journal.Checkpoints)
	{
		if (Checkpoint.Type == EBattlePresentationCheckpointType::CardGainedResolved)
		{
			GainedCheckpoints.Add(&Checkpoint);
		}
	}
	if ((Journal.DeckSteps.IsEmpty()
			&& DiscardBatches.IsEmpty()
			&& GainedCheckpoints.IsEmpty()
			&& Journal.EnemyActionSteps.IsEmpty())
		|| bProcessingPresentationPlan)
	{
		return false;
	}

	TArray<FGuid> FutureDrawnIds;
	for (const FBattlePresentationDeckStep& Step : Journal.DeckSteps)
	{
		if (Step.Kind == EBattlePresentationDeckStepKind::DrawBatch)
		{
			FutureDrawnIds.Append(Step.CardInstanceIds);
		}
	}
	const FBattleSnapshot BaseSnapshot = BuildSnapshotWithoutHandCardIds(
		PostCommandSnapshot,
		FutureDrawnIds);
	const FBattlePresentationEnemyActionStep* FirstEnemyActionStep =
		FindFirstEnemyActionStep(Journal.EnemyActionSteps);
	const FBattleSnapshot PreActionBaseSnapshot = FirstEnemyActionStep
		? BuildSnapshotWithCombatFacts(BaseSnapshot, FirstEnemyActionStep->SnapshotBefore)
		: BaseSnapshot;
	const TArray<FBattleEvent> NonDeckEvents = FilterNonDeckPresentationEvents(Events);
	const bool bHasGainedCheckpoint = !GainedCheckpoints.IsEmpty();
	const TArray<FBattleEvent> HandResolutionEvents = NonDeckEvents.FilterByPredicate(
		[bHasGainedCheckpoint](const FBattleEvent& Event)
		{
			return Event.Type != EBattleEventType::CardDiscarded
				&& Event.Type != EBattleEventType::HandLimitDiscarded
				&& (!bHasGainedCheckpoint || Event.Type != EBattleEventType::CardGained);
		});

	FWacomBattlePresentationPlan NewPlan;
	FBattleSnapshot PreviousHandSnapshot = PreCommandSnapshot;
	for (const FBattlePresentationCheckpoint* Checkpoint : GainedCheckpoints)
	{
		if (!Checkpoint)
		{
			continue;
		}
		const TArray<FBattleEvent> GainedEvents = NonDeckEvents.FilterByPredicate(
			[Checkpoint](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::CardGained
					&& Event.Sequence >= Checkpoint->FirstEventSequence
					&& Event.Sequence <= Checkpoint->LastEventSequence;
			});
		Runtime.StoreFirstPersonCardTransitionEvents(GainedEvents);
		FWacomBattlePresentationPhase GainedPhase;
		GainedPhase.Kind = EWacomBattlePresentationPhaseKind::CommandCardGained;
		GainedPhase.Snapshot = Checkpoint->Snapshot;
		GainedPhase.TransitionHints = Runtime.BuildFirstPersonCardTransitionHints(
			PreviousHandSnapshot,
			Checkpoint->Snapshot);
		GainedPhase.FeedbackHints = Runtime.BuildFirstPersonCardFeedbackHints(
			Checkpoint->Snapshot);
		Runtime.ClearPendingFirstPersonCardTransitionEvents();
		if (!GainedPhase.TransitionHints.IsEmpty() || !GainedPhase.FeedbackHints.IsEmpty())
		{
			NewPlan.Phases.Add(MoveTemp(GainedPhase));
		}
		PreviousHandSnapshot = Checkpoint->Snapshot;
	}

	Runtime.StoreFirstPersonCardTransitionEvents(HandResolutionEvents);
	FWacomBattlePresentationPhase HandPhase;
	HandPhase.Kind = EWacomBattlePresentationPhaseKind::CommandHandResolution;
	HandPhase.Snapshot = BaseSnapshot;
	HandPhase.TransitionHints = Runtime.BuildFirstPersonCardTransitionHints(
		PreviousHandSnapshot,
		BaseSnapshot);
	HandPhase.FeedbackHints = Runtime.BuildFirstPersonCardFeedbackHints(BaseSnapshot);
	Runtime.ClearPendingFirstPersonCardTransitionEvents();
	TSet<FGuid> GlyphDiscardCardIds;
	for (const FHandDiscardPresentationBatch& Batch : DiscardBatches)
	{
		GlyphDiscardCardIds.Append(Batch.CardInstanceIds);
	}
	HandPhase.TransitionHints.RemoveAll(
		[&GlyphDiscardCardIds](const FWacomFirstPersonCardLayerTransitionHint& Hint)
		{
			return GlyphDiscardCardIds.Contains(Hint.CardInstanceId);
		});
	HandPhase.FeedbackHints.RemoveAll(
		[&GlyphDiscardCardIds](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return GlyphDiscardCardIds.Contains(Hint.CardInstanceId);
		});
	const FGuid HandTargetImpactCardId = [&HandPhase]()
	{
		for (const FWacomFirstPersonCardLayerFeedbackHint& Hint : HandPhase.FeedbackHints)
		{
			if (Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact
				&& Hint.CardInstanceId.IsValid())
			{
				return Hint.CardInstanceId;
			}
		}
		return FGuid();
	}();
	const bool bHasHandPhase = !HandPhase.TransitionHints.IsEmpty()
		|| !HandPhase.FeedbackHints.IsEmpty()
		|| (DiscardBatches.IsEmpty()
			&& GainedCheckpoints.IsEmpty()
			&& Journal.EnemyActionSteps.IsEmpty());
	bool bHandPhaseAdded = false;
	FBattleSnapshot DiscardSnapshot = PreviousHandSnapshot;
	for (const FHandDiscardPresentationBatch& Batch : DiscardBatches)
	{
		RemoveHandCards(DiscardSnapshot, Batch.CardInstanceIds);
		DiscardSnapshot.PileCounts.DiscardCount = Batch.DiscardPileCountAfter;
		FWacomBattlePresentationPhase DiscardPhase =
			MakeHandDiscardGlyphPhase(Batch, DiscardSnapshot);
		const bool bContainsImpactedTarget = HandTargetImpactCardId.IsValid()
			&& Batch.CardInstanceIds.Contains(HandTargetImpactCardId);
		if (bContainsImpactedTarget && !bHandPhaseAdded)
		{
			DiscardPhase.Snapshot = BaseSnapshot;
			DiscardPhase.TransitionHints.Append(HandPhase.TransitionHints);
			DiscardPhase.FeedbackHints = HandPhase.FeedbackHints;
			NewPlan.Phases.Add(MoveTemp(DiscardPhase));
			bHandPhaseAdded = true;
		}
		else
		{
			NewPlan.Phases.Add(MoveTemp(DiscardPhase));
		}
	}
	if (bHasHandPhase && !bHandPhaseAdded)
	{
		NewPlan.Phases.Add(MoveTemp(HandPhase));
	}

	if (!NonDeckEvents.IsEmpty())
	{
		FWacomBattlePresentationPhase EventPhase;
		EventPhase.Kind = EWacomBattlePresentationPhaseKind::EnemyAction;
		EventPhase.Snapshot = PreActionBaseSnapshot;
		EventPhase.Events = NonDeckEvents;
		EventPhase.EnemyActionSteps = FilterEnemyActionStepsForEvents(
			Journal.EnemyActionSteps,
			EventPhase.Events);
		EventPhase.PresentationStackEntryId = PresentationStackEntryId;
		NewPlan.Phases.Add(MoveTemp(EventPhase));
	}
	AppendDeckStepPhases(
		NewPlan,
		Journal.DeckSteps,
		BaseSnapshot,
		PostCommandSnapshot,
		TArray<FGuid>());

	if (NewPlan.IsEmpty())
	{
		return false;
	}

	ClearPresentationPlan();
	PresentationPlan = MoveTemp(NewPlan);
	bProcessingPresentationPlan = true;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bWaitingForPresentationPlanEventQueue = false;
	ActivePileTransferEventSequence = INDEX_NONE;
	ActivePileTransferTotalCount = 0;
	ActivePileTransferExpectedDurationSeconds = 0.0f;
	ActivePileTransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
	ActivePileTransferInitialDrawCount = 0;
	ActivePileTransferInitialDiscardCount = 0;
	ActivePileTransferFinalDrawCount = 0;
	ActivePileTransferFinalDiscardCount = 0;
	ActivePileTransferPlayedCount = 0;
	ActivePileTransferLastLaunchedCount = 0;
	ActivePileTransferLastArrivedCount = 0;
	HandleQueueStarted();
	RefreshCommandBar();
	StartNextPresentationPlanPhase();
	return true;
}

bool FWacomBattleHUDPresentationCoordinator::EnqueuePlayCardPresentationPlan(
	const FWacomBattleCommandPresentationContext& Context,
	const FBattleResolution& Resolution,
	int32 PresentationStackEntryId)
{
	if (bProcessingPresentationPlan
		|| !Context.PlayCardCommit.IsSet()
		|| !Context.PlayCardCommit->CardInstanceId.IsValid())
	{
		return false;
	}

	const FGuid SourceCardId = Context.PlayCardCommit->CardInstanceId;
	TArray<FGuid> FutureDrawnIds;
	for (const FBattlePresentationDeckStep& Step : Resolution.PresentationJournal.DeckSteps)
	{
		if (Step.Kind == EBattlePresentationDeckStepKind::DrawBatch)
		{
			FutureDrawnIds.Append(Step.CardInstanceIds);
		}
	}
	const FBattleSnapshot BasePostSnapshot = BuildSnapshotWithoutHandCardIds(
		Resolution.PostSnapshot,
		FutureDrawnIds);
	const FBattlePresentationEnemyActionStep* FirstEnemyActionStep =
		FindFirstEnemyActionStep(Resolution.PresentationJournal.EnemyActionSteps);
	const FBattleSnapshot PreActionBasePostSnapshot = FirstEnemyActionStep
		? BuildSnapshotWithCombatFacts(BasePostSnapshot, FirstEnemyActionStep->SnapshotBefore)
		: BasePostSnapshot;
	const TArray<FHandDiscardPresentationBatch> DiscardBatches =
		BuildHandDiscardPresentationBatches(Resolution.Events);
	TArray<FGuid> DiscardedCardIds;
	for (const FHandDiscardPresentationBatch& Batch : DiscardBatches)
	{
		DiscardedCardIds.Append(Batch.CardInstanceIds);
	}
	const FBattleSnapshot PreDiscardOutcomeSnapshot = BuildSnapshotRestoringHandCardIds(
		PreActionBasePostSnapshot,
		Context.PreCommandSnapshot,
		DiscardedCardIds);

	Runtime.StoreFirstPersonCardTransitionEvents(Resolution.Events);
	const TArray<FWacomFirstPersonCardLayerTransitionHint> AllTransitionHints =
		Runtime.BuildFirstPersonCardTransitionHints(
			Context.PreCommandSnapshot,
			BasePostSnapshot);
	TArray<FWacomFirstPersonCardLayerFeedbackHint> AllFeedbackHints =
		Runtime.BuildFirstPersonCardFeedbackHints(BasePostSnapshot);
	// Command orchestration owns its before/after snapshots. Rebuild cost rewrites
	// from those explicit facts instead of depending on whichever snapshot happened
	// to be the last rendered frame (preview and battle-entry gates can differ).
	AllFeedbackHints.RemoveAll(
		[](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return Hint.FeedbackKind
					== EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite
				|| Hint.FeedbackKind
					== EWacomFirstPersonCardLayerFeedbackKind::EffectBadgeChange;
		});
	AllFeedbackHints.Append(BuildCommandDataRewriteHints(
		Context.PreCommandSnapshot,
		BasePostSnapshot,
		Resolution.Events,
		SourceCardId));
	AllFeedbackHints.Append(BuildCommandEffectBadgeChangeHints(
		Context.PreCommandSnapshot,
		BasePostSnapshot,
		Resolution.Events,
		SourceCardId));
	Runtime.ClearPendingFirstPersonCardTransitionEvents();

	const bool bSourceReturns = ContainsNormalHandCardId(
		Resolution.PostSnapshot,
		SourceCardId);
	const bool bHasSourcePlayedFact = Resolution.Events.ContainsByPredicate(
		[&SourceCardId](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardPlayed
				&& Event.CardInstanceId == SourceCardId;
		});
	if (bSourceReturns && bHasSourcePlayedFact
		&& !AllFeedbackHints.ContainsByPredicate(
			[&SourceCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == SourceCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
			}))
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = SourceCardId;
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
		AllFeedbackHints.Add(MoveTemp(Hint));
	}
	const FWacomFirstPersonCardLayerFeedbackHint* SourceReformHint =
		AllFeedbackHints.FindByPredicate(
			[&SourceCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == SourceCardId
					&& Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
			});

	FWacomBattlePresentationPlan NewPlan;
	NewPlan.CompletionStackEntryId = PresentationStackEntryId;

	FWacomBattlePresentationPhase SourcePhase;
	SourcePhase.Kind = EWacomBattlePresentationPhaseKind::CommandSourceOut;
	SourcePhase.Snapshot = bSourceReturns
		? Context.PreCommandSnapshot
		: BuildSnapshotWithoutHandCardIds(
			Context.PreCommandSnapshot,
			TArray<FGuid>({ SourceCardId }));
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : AllTransitionHints)
	{
		if (Hint.CardInstanceId == SourceCardId)
		{
			SourcePhase.TransitionHints.Add(Hint);
		}
	}
	if (bSourceReturns && SourceReformHint)
	{
		FWacomFirstPersonCardLayerFeedbackHint ReformOutHint = *SourceReformHint;
		ReformOutHint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardUseReformOut;
		SourcePhase.FeedbackHints.Add(MoveTemp(ReformOutHint));
	}
	if (!SourcePhase.TransitionHints.IsEmpty() || !SourcePhase.FeedbackHints.IsEmpty())
	{
		NewPlan.Phases.Add(MoveTemp(SourcePhase));
	}

	const FGuid HandTargetCardId =
		Context.CombatLogContext.CardTargetPreview.bHasPreview
		&& Context.CombatLogContext.CardTargetPreview.TargetKind
			== EWacomBattleCardPreviewTargetKind::HandCard
		? Context.CombatLogContext.CardTargetPreview.TargetHandCardInstanceId
		: FGuid();
	if (HandTargetCardId.IsValid()
		&& ContainsHandCardId(Context.PreCommandSnapshot, HandTargetCardId)
		&& !AllFeedbackHints.ContainsByPredicate(
			[&HandTargetCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == HandTargetCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
			}))
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = HandTargetCardId;
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
		AllFeedbackHints.Add(MoveTemp(Hint));
	}
	const FWacomFirstPersonCardLayerFeedbackHint* HandTargetHint =
		AllFeedbackHints.FindByPredicate(
			[&HandTargetCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return HandTargetCardId.IsValid()
					&& Hint.CardInstanceId == HandTargetCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
			});
	if (HandTargetHint)
	{
		FWacomBattlePresentationPhase TargetPhase;
		TargetPhase.Kind = EWacomBattlePresentationPhaseKind::CommandPrimaryTarget;
		TargetPhase.Snapshot = bSourceReturns
			? Context.PreCommandSnapshot
			: BuildSnapshotWithoutHandCardIds(
				Context.PreCommandSnapshot,
				TArray<FGuid>({ SourceCardId }));
		TargetPhase.FeedbackHints.Add(*HandTargetHint);
		TargetPhase.CompletionPolicy =
			EWacomBattlePresentationPhaseCompletionPolicy::HandTargetImpactPeak;
		TargetPhase.CompletionCardInstanceId = HandTargetCardId;
		NewPlan.Phases.Add(MoveTemp(TargetPhase));
	}
	else if (Context.PlayCardCommit->TargetPartIdentity.IsValidSlot())
	{
		FWacomBattlePresentationPhase TargetPhase;
		TargetPhase.Kind = EWacomBattlePresentationPhaseKind::CommandPrimaryTarget;
		TargetPhase.Snapshot = bSourceReturns
			? Context.PreCommandSnapshot
			: BuildSnapshotWithoutHandCardIds(
				Context.PreCommandSnapshot,
				TArray<FGuid>({ SourceCardId }));
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = EWacomBattlePresentationTargetCueKind::TargetConfirmed;
		Cue.TargetPartKey = Context.PlayCardCommit->TargetPartIdentity;
		Cue.Duration = 0.24f;
		Cue.Seed = static_cast<int32>(HashCombineFast(
			GetTypeHash(SourceCardId),
			GetTypeHash(Cue.TargetPartKey)) & 0x7FFFFFFFu);
		TargetPhase.TargetCue = MoveTemp(Cue);
		TargetPhase.CompletionPolicy =
			EWacomBattlePresentationPhaseCompletionPolicy::EventQueue;
		NewPlan.Phases.Add(MoveTemp(TargetPhase));
	}

	FWacomBattlePresentationPlan OutcomePlan;
	FWacomBattlePresentationPhase HandOutcomePhase;
	HandOutcomePhase.Kind = EWacomBattlePresentationPhaseKind::CommandOutcome;
	HandOutcomePhase.Snapshot = PreDiscardOutcomeSnapshot;
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : AllTransitionHints)
	{
		if (Hint.CardInstanceId == SourceCardId
			|| Hint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Drawn
			|| Hint.TransitionKind == EWacomFirstPersonCardSlotTransitionKind::Discarded)
		{
			continue;
		}
		HandOutcomePhase.TransitionHints.Add(Hint);
	}
	for (const FWacomFirstPersonCardLayerFeedbackHint& Hint : AllFeedbackHints)
	{
		if (Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::CardUseReform
			|| Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact)
		{
			continue;
		}
		FWacomFirstPersonCardLayerFeedbackHint OutcomeHint = Hint;
		if (OutcomeHint.FeedbackKind
				== EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite
			|| OutcomeHint.FeedbackKind
				== EWacomFirstPersonCardLayerFeedbackKind::EffectBadgeChange)
		{
			OutcomeHint.bBlocksPresentationPhase = true;
		}
		HandOutcomePhase.FeedbackHints.Add(MoveTemp(OutcomeHint));
	}
	HandOutcomePhase.OrderingSequence = FindFirstHandOutcomeSequence(
		Resolution.Events,
		HandOutcomePhase.TransitionHints,
		HandOutcomePhase.FeedbackHints);
	if (!HandOutcomePhase.TransitionHints.IsEmpty()
		|| !HandOutcomePhase.FeedbackHints.IsEmpty())
	{
		OutcomePlan.Phases.Add(MoveTemp(HandOutcomePhase));
	}

	FBattleSnapshot DiscardSnapshot = PreDiscardOutcomeSnapshot;
	for (const FHandDiscardPresentationBatch& Batch : DiscardBatches)
	{
		RemoveHandCards(DiscardSnapshot, Batch.CardInstanceIds);
		DiscardSnapshot.PileCounts.DiscardCount = Batch.DiscardPileCountAfter;
		OutcomePlan.Phases.Add(MakeHandDiscardGlyphPhase(Batch, DiscardSnapshot));
	}

	const TArray<FBattleEvent> QueueEvents = Resolution.Events.FilterByPredicate(
		[](const FBattleEvent& Event)
		{
			return IsImmediateEventQueuePresentationEvent(Event);
		});
	if (!QueueEvents.IsEmpty())
	{
		FWacomBattlePresentationPhase EventPhase;
		EventPhase.Kind = EWacomBattlePresentationPhaseKind::CommandOutcome;
		EventPhase.Snapshot = PreActionBasePostSnapshot;
		EventPhase.Events = QueueEvents;
		EventPhase.EnemyActionSteps = FilterEnemyActionStepsForEvents(
			Resolution.PresentationJournal.EnemyActionSteps,
			EventPhase.Events);
		EventPhase.OrderingSequence = FindFirstPositiveEventSequence(QueueEvents);
		EventPhase.bTargetAlreadyConfirmed =
			Context.PlayCardCommit->TargetPartIdentity.IsValidSlot();
		EventPhase.CompletionPolicy =
			EWacomBattlePresentationPhaseCompletionPolicy::EventQueue;
		OutcomePlan.Phases.Add(MoveTemp(EventPhase));
	}
	AppendDeckStepPhases(
		OutcomePlan,
		Resolution.PresentationJournal.DeckSteps,
		BasePostSnapshot,
		Resolution.PostSnapshot,
		TArray<FGuid>());
	OutcomePlan.Phases.StableSort(
		[](const FWacomBattlePresentationPhase& A, const FWacomBattlePresentationPhase& B)
		{
			if (A.OrderingSequence == INDEX_NONE)
			{
				return false;
			}
			if (B.OrderingSequence == INDEX_NONE)
			{
				return true;
			}
			return A.OrderingSequence < B.OrderingSequence;
		});
	NewPlan.Phases.Append(MoveTemp(OutcomePlan.Phases));

	if (bSourceReturns && SourceReformHint)
	{
		FWacomBattlePresentationPhase ReturnPhase;
		ReturnPhase.Kind = EWacomBattlePresentationPhaseKind::CommandSourceReturn;
		ReturnPhase.Snapshot = Resolution.PostSnapshot;
		FWacomFirstPersonCardLayerFeedbackHint ReformInHint = *SourceReformHint;
		ReformInHint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::CardUseReformIn;
		ReturnPhase.FeedbackHints.Add(MoveTemp(ReformInHint));
		NewPlan.Phases.Add(MoveTemp(ReturnPhase));
	}

	const TArray<FBattleEvent> BlockingDialogEvents = Resolution.Events.FilterByPredicate(
		[](const FBattleEvent& Event)
		{
			return IsBlockingDialogPresentationEvent(Event);
		});
	if (!BlockingDialogEvents.IsEmpty())
	{
		FWacomBattlePresentationPhase DialogPhase;
		DialogPhase.Kind = EWacomBattlePresentationPhaseKind::CommandBlockingDialog;
		DialogPhase.Events = BlockingDialogEvents;
		DialogPhase.OrderingSequence = FindFirstPositiveEventSequence(BlockingDialogEvents);
		DialogPhase.CompletionPolicy =
			EWacomBattlePresentationPhaseCompletionPolicy::EventQueue;
		NewPlan.Phases.Add(MoveTemp(DialogPhase));
	}

	if (NewPlan.IsEmpty())
	{
		return false;
	}

	ClearPresentationPlan();
	PresentationPlan = MoveTemp(NewPlan);
	bProcessingPresentationPlan = true;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanCompletionPolicy =
		EWacomBattlePresentationPhaseCompletionPolicy::PlaybackIdle;
	ActivePresentationPlanCompletionCardId.Invalidate();
	ActivePresentationPlanCompletionStackEntryId = PresentationPlan.CompletionStackEntryId;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bWaitingForPresentationPlanEventQueue = false;
	HandleQueueStarted();
	RefreshCommandBar();
	StartNextPresentationPlanPhase();
	return true;
}

void FWacomBattleHUDPresentationCoordinator::HandlePileTransferProgress(
	const FWacomFirstPersonCardPileTransferProgressView& Progress)
{
	if (!bProcessingPresentationPlan
		|| !IsGlyphTransferPhase(ActivePresentationPlanPhaseKind)
		|| Progress.EventSequence != ActivePileTransferEventSequence
		|| Progress.TransferKind != ActivePileTransferKind)
	{
		return;
	}
	const int32 Total = FMath::Max(0, ActivePileTransferTotalCount);
	const int32 Launched = FMath::Clamp(Progress.LaunchedCount, 0, Total);
	const int32 Arrived = FMath::Clamp(Progress.ArrivedCount, 0, Total);
	ActivePileTransferExpectedDurationSeconds = FMath::Max(
		ActivePileTransferExpectedDurationSeconds,
		FMath::Max(0.0f, Progress.ExpectedDurationSeconds));
	if (Progress.TransferKind == FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile)
	{
		if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
		{
			const int32 MonotonicArrived = FMath::Max(
				ActivePileTransferLastArrivedCount,
				Arrived);
			const int32 NewlyArrived = MonotonicArrived - ActivePileTransferLastArrivedCount;
			const int32 Count = ActivePileTransferInitialDiscardCount + MonotonicArrived;
			DiscardPileView->SetCount(Count);
			DiscardPileView->SetCountDisplayText(FText::AsNumber(Count));
			if (Progress.bWasForceCompleted)
			{
				DiscardPileView->ResetReceiveFeedback();
			}
			else if (NewlyArrived > 0)
			{
				DiscardPileView->PlayReceiveFeedback(
					NewlyArrived,
					MonotonicArrived >= Total,
					Progress.bReducedMotion);
			}
			ActivePileTransferLastArrivedCount = MonotonicArrived;
		}
		return;
	}

	const int32 MonotonicLaunched = FMath::Max(ActivePileTransferLastLaunchedCount, Launched);
	const int32 MonotonicArrived = FMath::Max(ActivePileTransferLastArrivedCount, Arrived);
	const int32 NewlyLaunched = MonotonicLaunched - ActivePileTransferLastLaunchedCount;
	const int32 NewlyArrived = MonotonicArrived - ActivePileTransferLastArrivedCount;
	if (Progress.bWasForceCompleted)
	{
		if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
		{
			DiscardPileView->SetCount(ActivePileTransferFinalDiscardCount);
			DiscardPileView->SetCountDisplayText(
				WacomBattlePileCountPresentation::BuildDiscardPileCountDisplayText(
					ActivePileTransferFinalDiscardCount,
					ActivePileTransferPlayedCount));
			DiscardPileView->ResetSendFeedback();
		}
		if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
		{
			DrawPileView->SetCount(ActivePileTransferFinalDrawCount);
			DrawPileView->ResetReceiveFeedback();
		}
		ActivePileTransferLastLaunchedCount = Total;
		ActivePileTransferLastArrivedCount = Total;
		return;
	}

	if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
	{
		const int32 Count = FMath::Max(
			ActivePileTransferFinalDiscardCount,
			ActivePileTransferInitialDiscardCount - MonotonicLaunched);
		DiscardPileView->SetCount(Count);
		DiscardPileView->SetCountDisplayText(
			WacomBattlePileCountPresentation::BuildDiscardPileCountDisplayText(
				Count,
				ActivePileTransferPlayedCount));
		if (NewlyLaunched > 0)
		{
			DiscardPileView->PlaySendFeedback(
				NewlyLaunched,
				MonotonicLaunched >= Total,
				Progress.LaunchDirection,
				Progress.bReducedMotion);
		}
	}
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		const int32 Count = FMath::Min(
			ActivePileTransferFinalDrawCount,
			ActivePileTransferInitialDrawCount + MonotonicArrived);
		DrawPileView->SetCount(Count);
		if (NewlyArrived > 0)
		{
			DrawPileView->PlayReceiveFeedback(
				NewlyArrived,
				MonotonicArrived >= Total,
				Progress.bReducedMotion);
		}
	}
	ActivePileTransferLastLaunchedCount = MonotonicLaunched;
	ActivePileTransferLastArrivedCount = MonotonicArrived;
}

void FWacomBattleHUDPresentationCoordinator::ClearQueue()
{
	ClearPresentationPlan();
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->Clear();
		BattleEventPresentationQueue.Reset();
	}
	ClearStack();
	ClearPendingTurnBoundaryCommand();
}

bool FWacomBattleHUDPresentationCoordinator::IsQueueBusy() const
{
	return BattleEventPresentationQueue && BattleEventPresentationQueue->IsBusy();
}

FName FWacomBattleHUDPresentationCoordinator::GetActivePresentationPlanPhaseName() const
{
	return FName(BattlePresentationPhaseKindToString(ActivePresentationPlanPhaseKind));
}

void FWacomBattleHUDPresentationCoordinator::QueuePendingTurnBoundaryCommand(
	EWacomBattleHUDTurnBoundaryCommand Command)
{
	if (Command == EWacomBattleHUDTurnBoundaryCommand::None
		|| PendingTurnBoundaryCommand != EWacomBattleHUDTurnBoundaryCommand::None)
	{
		return;
	}

	Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("PendingTurnBoundary"));
	PendingTurnBoundaryCommand = Command;
	if (Runtime.GetUIState() == EBattleUIState::TargetSelect)
	{
		Runtime.ClearPendingTargetingCardId();
		Runtime.SetUIState(EBattleUIState::Idle);
	}
	RefreshCommandBar();
	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::ClearPendingTurnBoundaryCommand()
{
	if (PendingTurnBoundaryCommand == EWacomBattleHUDTurnBoundaryCommand::None)
	{
		return;
	}

	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
	RefreshCommandBar();
}

FText FWacomBattleHUDPresentationCoordinator::GetPendingTurnBoundaryCommandText() const
{
	switch (PendingTurnBoundaryCommand)
	{
	case EWacomBattleHUDTurnBoundaryCommand::Wait:
		return NSLOCTEXT("BattleHUD", "PendingTurnBoundaryWait", "等待排队中");
	case EWacomBattleHUDTurnBoundaryCommand::EndTurn:
		return NSLOCTEXT("BattleHUD", "PendingTurnBoundaryEndTurn", "结束回合排队中");
	case EWacomBattleHUDTurnBoundaryCommand::None:
	default:
		return FText::GetEmpty();
	}
}

void FWacomBattleHUDPresentationCoordinator::TryExecutePendingTurnBoundaryCommand()
{
	if (PendingTurnBoundaryCommand == EWacomBattleHUDTurnBoundaryCommand::None
		|| HasStackEntries()
		|| IsQueueBusy()
		|| IsPresentationPlanBusy())
	{
		return;
	}

	UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		ClearPendingTurnBoundaryCommand();
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd
		|| Snapshot.Phase == EBattlePhase::PendingKnockdownChoice
		|| Snapshot.Phase != EBattlePhase::PlayerAction)
	{
		ClearPendingTurnBoundaryCommand();
		return;
	}

	const EWacomBattleHUDTurnBoundaryCommand CommandToExecute = PendingTurnBoundaryCommand;
	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
	RefreshCommandBar();
	ExecuteTurnBoundaryCommandNow(CommandToExecute);
}

void FWacomBattleHUDPresentationCoordinator::HandleQueueStarted()
{
	Runtime.HideCardDetailPanel();
}

void FWacomBattleHUDPresentationCoordinator::HandleQueueFinished()
{
	if (bProcessingPresentationPlan && bWaitingForPresentationPlanEventQueue)
	{
		bWaitingForPresentationPlanEventQueue = false;
		StartNextPresentationPlanPhase();
		return;
	}

	UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		Runtime.SetUIState(EBattleUIState::BattleEnd);
		return;
	}

	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::HandleBattleEndStep()
{
	if (bProcessingPresentationPlan)
	{
		// BattleEnd is an authority boundary, not another authored outcome phase.
		// Drop deferred return/dialog phases before the final snapshot broadcasts
		// battle end, and settle any source card held hidden by reform-out.
		PresentationPlan.Phases.Reset();
		ActivePresentationPlanCompletionStackEntryId = INDEX_NONE;
		if (UWacomFirstPersonCardAnchorComponent* Anchor =
			Runtime.ResolveActiveFirstPersonCardAnchor())
		{
			Anchor->ForceSettleCardLayerPresentationPlayback();
		}
		ClearStack();
	}
	if (UBattleSession* CurrentSession = Runtime.GetSession())
	{
		Runtime.NativeRefreshFromSnapshot(CurrentSession->BuildSnapshot());
	}
	Runtime.GetSceneEnemyTargetCoordinator().ClearRetiringHosts(false);
}

void FWacomBattleHUDPresentationCoordinator::HandleKnockdownChoiceDialogStep()
{
	Runtime.PushPendingKnockdownChoiceDialog();
}

void FWacomBattleHUDPresentationCoordinator::HandleTargetCueStep(
	const FWacomBattlePresentationTargetCue& Cue)
{
	Runtime.PlayBattlePresentationCue(Cue);
}

void FWacomBattleHUDPresentationCoordinator::HandleSceneEnemyAnimationStep(
	const FBattlePartSlotIdentity& ActingPartKey,
	FName IntentId,
	bool bDestroyed,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	FWacomBattleHUDSceneEnemyTargetCoordinator& SceneEnemyCoordinator =
		Runtime.GetSceneEnemyTargetCoordinator();
	if (bDestroyed)
	{
		SceneEnemyCoordinator.PlayHostDestroyedAnimation(
			ActingPartKey.GetEffectiveEnemySlotId(),
			MoveTemp(Callbacks.OnCompleted));
		return;
	}
	SceneEnemyCoordinator.PlaySceneEnemyActionAnimation(
		ActingPartKey,
		IntentId,
		MoveTemp(Callbacks));
}

void FWacomBattleHUDPresentationCoordinator::HandleSceneEnemyActionImpact(
	const FBattlePresentationEnemyActionStep& ActionStep)
{
	if (!ActionStep.IsValid())
	{
		return;
	}

	Runtime.RefreshCombatPresentationFrame(ActionStep.SnapshotAfter);
	if (UPlayerStatusBar* PlayerStatusBar = Runtime.Host().GetPlayerStatusBar())
	{
		PlayerStatusBar->PlayEnemyActionImpactFeedback(
			ActionStep.SnapshotBefore.Player,
			ActionStep.SnapshotAfter.Player);
	}
}

void FWacomBattleHUDPresentationCoordinator::HandleCardStackBoundaryStep(int32 EntryId)
{
	BeginStackEntryExit(EntryId);
}

UWorld* FWacomBattleHUDPresentationCoordinator::GetWorld() const
{
	return Runtime.GetWorld();
}

#if WITH_AUTOMATION_TESTS
void FWacomBattleHUDPresentationCoordinator::AdvanceQueueOnce()
{
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->AdvanceForTest();
	}
}

void FWacomBattleHUDPresentationCoordinator::AdvancePresentationPlanOnce()
{
	PollActivePresentationPlanPhase();
}

void FWacomBattleHUDPresentationCoordinator::PrimeDiscardPileReceiveFeedbackForTest(
	int32 EventSequence,
	int32 TotalCount,
	int32 InitialDiscardCount)
{
	bProcessingPresentationPlan = true;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::HandDiscardGlyphTransfer;
	ActivePileTransferEventSequence = EventSequence;
	ActivePileTransferTotalCount = FMath::Max(0, TotalCount);
	ActivePileTransferExpectedDurationSeconds = 0.0f;
	ActivePileTransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile;
	ActivePileTransferInitialDrawCount = 0;
	ActivePileTransferInitialDiscardCount = FMath::Max(0, InitialDiscardCount);
	ActivePileTransferFinalDrawCount = 0;
	ActivePileTransferFinalDiscardCount = ActivePileTransferInitialDiscardCount + ActivePileTransferTotalCount;
	ActivePileTransferPlayedCount = 0;
	ActivePileTransferLastLaunchedCount = 0;
	ActivePileTransferLastArrivedCount = 0;
}

void FWacomBattleHUDPresentationCoordinator::PrimeReshufflePileFeedbackForTest(
	int32 EventSequence,
	int32 TotalCount,
	int32 InitialDrawCount,
	int32 FinalDrawCount,
	int32 InitialDiscardCount,
	int32 FinalDiscardCount,
	int32 PlayedCount)
{
	bProcessingPresentationPlan = true;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::DeckReshuffle;
	ActivePileTransferEventSequence = EventSequence;
	ActivePileTransferTotalCount = FMath::Max(0, TotalCount);
	ActivePileTransferExpectedDurationSeconds = 0.0f;
	ActivePileTransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
	ActivePileTransferInitialDrawCount = FMath::Max(0, InitialDrawCount);
	ActivePileTransferInitialDiscardCount = FMath::Max(0, InitialDiscardCount);
	ActivePileTransferFinalDrawCount = FMath::Max(0, FinalDrawCount);
	ActivePileTransferFinalDiscardCount = FMath::Max(0, FinalDiscardCount);
	ActivePileTransferPlayedCount = FMath::Max(0, PlayedCount);
	ActivePileTransferLastLaunchedCount = 0;
	ActivePileTransferLastArrivedCount = 0;
}
#endif

void FWacomBattleHUDPresentationCoordinator::SyncStackWidget()
{
	if (UBattlePresentationStackWidget* BattlePresentationStack = Runtime.Host().GetBattlePresentationStack())
	{
		BattlePresentationStack->SetPresentationStackEntries(BattlePresentationStackEntries);
	}
}

void FWacomBattleHUDPresentationCoordinator::RestoreActiveReshufflePileCounts()
{
	if (!bProcessingPresentationPlan
		|| ActivePileTransferKind
			!= FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw
		|| !IsGlyphTransferPhase(ActivePresentationPlanPhaseKind))
	{
		return;
	}

	if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
	{
		DiscardPileView->SetCount(ActivePileTransferFinalDiscardCount);
		DiscardPileView->SetCountDisplayText(
			WacomBattlePileCountPresentation::BuildDiscardPileCountDisplayText(
				ActivePileTransferFinalDiscardCount,
				ActivePileTransferPlayedCount));
	}
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->SetCount(ActivePileTransferFinalDrawCount);
	}
}

void FWacomBattleHUDPresentationCoordinator::ResetActivePileTransferFeedback()
{
	if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
	{
		DiscardPileView->ResetReceiveFeedback();
		DiscardPileView->ResetSendFeedback();
	}
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->ResetReceiveFeedback();
	}
}

void FWacomBattleHUDPresentationCoordinator::ClearPresentationPlan()
{
	Runtime.CompleteActiveDrawPileFeedbackBatch();
	if (bProcessingPresentationPlan && IsGlyphTransferPhase(ActivePresentationPlanPhaseKind))
	{
		if (UWacomFirstPersonCardAnchorComponent* Anchor = Runtime.ResolveActiveFirstPersonCardAnchor())
		{
			Anchor->ForceSettleCardLayerPresentationPlayback();
		}
	}
	RestoreActiveReshufflePileCounts();
	ResetActivePileTransferFeedback();
	StopPresentationPlanTimer();
	PresentationPlan = FWacomBattlePresentationPlan();
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanCompletionPolicy =
		EWacomBattlePresentationPhaseCompletionPolicy::PlaybackIdle;
	ActivePresentationPlanCompletionCardId.Invalidate();
	ActivePresentationPlanCompletionStackEntryId = INDEX_NONE;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bProcessingPresentationPlan = false;
	bWaitingForPresentationPlanEventQueue = false;
	ActivePileTransferEventSequence = INDEX_NONE;
	ActivePileTransferTotalCount = 0;
	ActivePileTransferExpectedDurationSeconds = 0.0f;
	ActivePileTransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
	ActivePileTransferInitialDrawCount = 0;
	ActivePileTransferInitialDiscardCount = 0;
	ActivePileTransferFinalDrawCount = 0;
	ActivePileTransferFinalDiscardCount = 0;
	ActivePileTransferPlayedCount = 0;
	ActivePileTransferLastLaunchedCount = 0;
	ActivePileTransferLastArrivedCount = 0;
#if WITH_AUTOMATION_TESTS
	StartedPresentationPlanPhaseNamesForTest.Reset();
	SubmittedPresentationPlanFeedbackHintsForTest.Reset();
#endif
}

void FWacomBattleHUDPresentationCoordinator::StartNextPresentationPlanPhase()
{
	if (ActivePresentationPlanPhaseKind == EWacomBattlePresentationPhaseKind::TurnStartDraw)
	{
		Runtime.CompleteActiveDrawPileFeedbackBatch();
	}
	if (IsGlyphTransferPhase(ActivePresentationPlanPhaseKind))
	{
		RestoreActiveReshufflePileCounts();
		ResetActivePileTransferFeedback();
	}
	StopPresentationPlanTimer();
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanCompletionPolicy =
		EWacomBattlePresentationPhaseCompletionPolicy::PlaybackIdle;
	ActivePresentationPlanCompletionCardId.Invalidate();
	bWaitingForPresentationPlanEventQueue = false;

	if (!bProcessingPresentationPlan)
	{
		return;
	}

	if (PresentationPlan.Phases.IsEmpty())
	{
		FinishPresentationPlan();
		return;
	}

	FWacomBattlePresentationPhase Phase = MoveTemp(PresentationPlan.Phases[0]);
	PresentationPlan.Phases.RemoveAt(0);
	ActivePresentationPlanPhaseKind = Phase.Kind;
	ActivePresentationPlanCompletionPolicy = Phase.CompletionPolicy;
	ActivePresentationPlanCompletionCardId = Phase.CompletionCardInstanceId;
#if WITH_AUTOMATION_TESTS
	StartedPresentationPlanPhaseNamesForTest.Add(GetActivePresentationPlanPhaseName());
#endif

	if (Phase.HasTargetCue())
	{
		StartTargetCuePresentationPlanPhase(MoveTemp(Phase));
		return;
	}

	if (Phase.HasHandFrame())
	{
		StartHandPresentationPlanPhase(MoveTemp(Phase));
		return;
	}

	if (Phase.HasEventQueue())
	{
		StartEventPresentationPlanPhase(MoveTemp(Phase));
		return;
	}

	StartNextPresentationPlanPhase();
}

void FWacomBattleHUDPresentationCoordinator::StartHandPresentationPlanPhase(
	FWacomBattlePresentationPhase&& Phase)
{
#if WITH_AUTOMATION_TESTS
	SubmittedPresentationPlanFeedbackHintsForTest.Append(Phase.FeedbackHints);
#endif
	if (Phase.DrawPileFeedbackBatch.IsSet())
	{
		Runtime.QueueDrawPileFeedbackBatch(Phase.DrawPileFeedbackBatch.GetValue());
	}
	Runtime.RefreshFromPresentationPhase(
		Phase.Snapshot,
		Phase.TransitionHints,
		Phase.FeedbackHints);
	if (IsGlyphTransferPhase(Phase.Kind) && !Phase.PileTransferHints.IsEmpty())
	{
		ActivePileTransferEventSequence = Phase.PileTransferHints[0].EventSequence;
		ActivePileTransferTotalCount = Phase.PileTransferHints[0].CardInstanceIds.Num();
		ActivePileTransferExpectedDurationSeconds = 0.0f;
		ActivePileTransferKind = Phase.PileTransferHints[0].TransferKind;
		ActivePileTransferInitialDrawCount = Phase.PileTransferInitialDrawCount;
		ActivePileTransferInitialDiscardCount = Phase.PileTransferInitialDiscardCount;
		ActivePileTransferFinalDrawCount = Phase.PileTransferFinalDrawCount;
		ActivePileTransferFinalDiscardCount = Phase.PileTransferFinalDiscardCount;
		ActivePileTransferPlayedCount = Phase.PileTransferPlayedCount;
		ActivePileTransferLastLaunchedCount = 0;
		ActivePileTransferLastArrivedCount = 0;
		if (ActivePileTransferKind
			== FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardToPile)
		{
			if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
			{
				DiscardPileView->ResetReceiveFeedback();
				DiscardPileView->SetCount(ActivePileTransferInitialDiscardCount);
				DiscardPileView->SetCountDisplayText(
					FText::AsNumber(ActivePileTransferInitialDiscardCount));
			}
		}
		else
		{
			if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
			{
				DiscardPileView->ResetSendFeedback();
				DiscardPileView->SetCount(ActivePileTransferInitialDiscardCount);
				DiscardPileView->SetCountDisplayText(
					WacomBattlePileCountPresentation::BuildDiscardPileCountDisplayText(
						ActivePileTransferInitialDiscardCount,
						ActivePileTransferPlayedCount));
			}
			if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
			{
				DrawPileView->ResetReceiveFeedback();
				DrawPileView->SetCount(ActivePileTransferInitialDrawCount);
			}
		}
		Runtime.GetFirstPersonHandBridge().ApplyPileTransferHints(Phase.PileTransferHints);
	}
	else
	{
		ActivePileTransferEventSequence = INDEX_NONE;
		ActivePileTransferTotalCount = 0;
		ActivePileTransferExpectedDurationSeconds = 0.0f;
		ActivePileTransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
		ActivePileTransferInitialDrawCount = 0;
		ActivePileTransferInitialDiscardCount = 0;
		ActivePileTransferFinalDrawCount = 0;
		ActivePileTransferFinalDiscardCount = 0;
		ActivePileTransferPlayedCount = 0;
		ActivePileTransferLastLaunchedCount = 0;
		ActivePileTransferLastArrivedCount = 0;
	}
	if (UWacomFirstPersonCardAnchorComponent* Anchor = Runtime.ResolveActiveFirstPersonCardAnchor())
	{
		Anchor->RefreshCardLayerNow(0.0f);
	}

	if (!HasActiveFirstPersonHandPresentationPlayback()
		&& !HasPendingFirstPersonHandPresentationFrame())
	{
		StartNextPresentationPlanPhase();
		return;
	}

	SchedulePresentationPlanPoll(BattlePresentationPlanPollSeconds);
}

void FWacomBattleHUDPresentationCoordinator::StartEventPresentationPlanPhase(
	FWacomBattlePresentationPhase&& Phase)
{
	if (!Phase.EnemyActionSteps.IsEmpty())
	{
		Runtime.RefreshCombatPresentationFrame(Phase.Snapshot);
	}
	bWaitingForPresentationPlanEventQueue = true;
	EnqueueEvents(
		Phase.Events,
		Phase.EnemyActionSteps,
		Phase.PresentationStackEntryId,
		Phase.bTargetAlreadyConfirmed);
	if (bWaitingForPresentationPlanEventQueue && !IsQueueBusy())
	{
		bWaitingForPresentationPlanEventQueue = false;
		StartNextPresentationPlanPhase();
	}
}

void FWacomBattleHUDPresentationCoordinator::StartTargetCuePresentationPlanPhase(
	FWacomBattlePresentationPhase&& Phase)
{
	if (!Phase.TargetCue.IsSet())
	{
		StartNextPresentationPlanPhase();
		return;
	}
	if (!BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue = MakeShared<FWacomBattleEventPresentationQueue>(
			*this,
			*PresentationTimerOwner);
	}
	bWaitingForPresentationPlanEventQueue = true;
	BattleEventPresentationQueue->EnqueueTargetCue(Phase.TargetCue.GetValue());
	if (bWaitingForPresentationPlanEventQueue && !IsQueueBusy())
	{
		bWaitingForPresentationPlanEventQueue = false;
		StartNextPresentationPlanPhase();
	}
}

void FWacomBattleHUDPresentationCoordinator::SchedulePresentationPlanPoll(float DelaySeconds)
{
	if (UWorld* World = GetWorld())
	{
		PresentationTimerOwner->ScheduleOnce(
			World,
			FWacomBattlePresentationTimerKey::PresentationPlanPoll(),
			FMath::Max(0.01f, DelaySeconds),
			[this]()
			{
				PollActivePresentationPlanPhase();
			});
		return;
	}

	PollActivePresentationPlanPhase();
}

void FWacomBattleHUDPresentationCoordinator::StopPresentationPlanTimer()
{
	PresentationTimerOwner->Cancel(
		FWacomBattlePresentationTimerKey::PresentationPlanPoll());
}

void FWacomBattleHUDPresentationCoordinator::PollActivePresentationPlanPhase()
{
	if (!bProcessingPresentationPlan
		|| ActivePresentationPlanPhaseKind == EWacomBattlePresentationPhaseKind::None
		|| bWaitingForPresentationPlanEventQueue)
	{
		return;
	}

	ActivePresentationPlanPhaseElapsedSeconds += BattlePresentationPlanPollSeconds;
	const bool bPlaybackFinished =
		!HasActiveFirstPersonHandPresentationPlayback()
		&& !HasPendingFirstPersonHandPresentationFrame();
	bool bCompletionReached = bPlaybackFinished;
	if (ActivePresentationPlanCompletionPolicy
		== EWacomBattlePresentationPhaseCompletionPolicy::HandTargetImpactPeak)
	{
		const UWacomFirstPersonCardAnchorComponent* Anchor =
			Runtime.ResolveActiveFirstPersonCardAnchor();
		bCompletionReached = (Anchor
				&& Anchor->HasHandTargetImpactReachedPeak(
					ActivePresentationPlanCompletionCardId))
			|| bPlaybackFinished;
	}
	const float PhaseTimeoutSeconds =
		IsGlyphTransferPhase(ActivePresentationPlanPhaseKind)
			&& ActivePileTransferExpectedDurationSeconds > 0.0f
		? FMath::Max(
			BattlePresentationPlanHandPhaseTimeoutSeconds,
			ActivePileTransferExpectedDurationSeconds + 1.0f)
		: BattlePresentationPlanHandPhaseTimeoutSeconds;
	const bool bTimedOut = ActivePresentationPlanPhaseElapsedSeconds >= PhaseTimeoutSeconds;
	if (bCompletionReached || bTimedOut)
	{
		if (bTimedOut)
		{
			if (UWacomFirstPersonCardAnchorComponent* Anchor = Runtime.ResolveActiveFirstPersonCardAnchor())
			{
				Anchor->ForceSettleCardLayerPresentationPlayback();
			}
		}
		StartNextPresentationPlanPhase();
		return;
	}

	SchedulePresentationPlanPoll(BattlePresentationPlanPollSeconds);
}

void FWacomBattleHUDPresentationCoordinator::FinishPresentationPlan()
{
	const int32 CompletionStackEntryId = ActivePresentationPlanCompletionStackEntryId;
	StopPresentationPlanTimer();
	PresentationPlan = FWacomBattlePresentationPlan();
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanCompletionPolicy =
		EWacomBattlePresentationPhaseCompletionPolicy::PlaybackIdle;
	ActivePresentationPlanCompletionCardId.Invalidate();
	ActivePresentationPlanCompletionStackEntryId = INDEX_NONE;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bProcessingPresentationPlan = false;
	bWaitingForPresentationPlanEventQueue = false;
	ActivePileTransferEventSequence = INDEX_NONE;
	ActivePileTransferTotalCount = 0;
	ActivePileTransferExpectedDurationSeconds = 0.0f;
	ActivePileTransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
	ActivePileTransferInitialDrawCount = 0;
	ActivePileTransferInitialDiscardCount = 0;
	ActivePileTransferFinalDrawCount = 0;
	ActivePileTransferFinalDiscardCount = 0;
	ActivePileTransferPlayedCount = 0;
	ActivePileTransferLastLaunchedCount = 0;
	ActivePileTransferLastArrivedCount = 0;
	ResetActivePileTransferFeedback();

	if (UBattleSession* CurrentSession = Runtime.GetSession())
	{
		Runtime.NativeRefreshFromSnapshot(CurrentSession->BuildSnapshot());
	}
	if (CompletionStackEntryId != INDEX_NONE)
	{
		BeginStackEntryExit(CompletionStackEntryId);
	}
	TryExecutePendingTurnBoundaryCommand();
}

bool FWacomBattleHUDPresentationCoordinator::HasActiveFirstPersonHandPresentationPlayback() const
{
	const UWacomFirstPersonCardAnchorComponent* Anchor = Runtime.ResolveActiveFirstPersonCardAnchor();
	return Anchor && Anchor->HasActiveCardLayerPresentationPlayback();
}

bool FWacomBattleHUDPresentationCoordinator::HasPendingFirstPersonHandPresentationFrame() const
{
	return Runtime.GetFirstPersonHandBridge().HasPendingPresentationFrame();
}

void FWacomBattleHUDPresentationCoordinator::ExecuteTurnBoundaryCommandNow(
	EWacomBattleHUDTurnBoundaryCommand Command)
{
	switch (Command)
	{
	case EWacomBattleHUDTurnBoundaryCommand::Wait:
		Runtime.OnWaitRequested();
		break;
	case EWacomBattleHUDTurnBoundaryCommand::EndTurn:
		Runtime.OnEndTurnRequested();
		break;
	case EWacomBattleHUDTurnBoundaryCommand::None:
	default:
		break;
	}
}

void FWacomBattleHUDPresentationCoordinator::RefreshCommandBarOnly()
{
	UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
}

void FWacomBattleHUDPresentationCoordinator::RefreshCommandBar()
{
	UBattleSession* CurrentSession = Runtime.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
	if (!IsPresentationPlanBusy())
	{
		Runtime.SyncFirstPersonBattleHandLayer(Snapshot);
	}
	Runtime.SyncBattleEnemyPartWorldTargets(Snapshot);
}
