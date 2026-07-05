// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"

#include "UI/Battle/BattleHUD.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/Widget.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleSession.h"
#include "UI/Battle/ActionPanel.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleEventPresentationQueue.h"
#include "UI/Battle/WacomBattleHUDCommandFlow.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDTargetingFlow.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

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
		FeedbackCardIds.Append(CollectHandAnchorCardIds(Snapshot));
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

	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildRetainedFeedbackHintsForCardIds(
		const TArray<FGuid>& CardInstanceIds)
	{
		TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints;
		Hints.Reserve(CardInstanceIds.Num());
		const int32 SequenceCount = CardInstanceIds.Num();
		for (int32 Index = 0; Index < CardInstanceIds.Num(); ++Index)
		{
			FWacomFirstPersonCardLayerFeedbackHint Hint;
			Hint.CardInstanceId = CardInstanceIds[Index];
			Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
			Hint.SequenceIndex = Index;
			Hint.SequenceCount = FMath::Max(1, SequenceCount);
			Hints.Add(Hint);
		}
		return Hints;
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
				Plan.Phases.Add(MoveTemp(Phase));
			}
		}

		if (RetainCheckpoint)
		{
			const TArray<FGuid> RetainedFeedbackCardIds = BuildRetainedPhaseFeedbackCardIds(
				RetainCheckpoint->Snapshot,
				RetainCheckpoint->CardInstanceIds);
			if (!RetainedFeedbackCardIds.IsEmpty())
			{
				FWacomBattlePresentationPhase Phase;
				Phase.Kind = EWacomBattlePresentationPhaseKind::TurnEndRetain;
				Phase.Snapshot = RetainCheckpoint->Snapshot;
				Phase.FeedbackHints = BuildRetainedFeedbackHintsForCardIds(RetainedFeedbackCardIds);
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
			Plan.Phases.Add(MoveTemp(Phase));
		}

		if (DrawCheckpoint)
		{
			const TArray<FGuid> NewHandAnchorCardIds = CollectNewHandAnchorCardIds(
				HandSnapshotBeforeDraw,
				DrawCheckpoint->Snapshot);
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

		return Plan;
	}
}

FWacomBattleHUDPresentationCoordinator::FWacomBattleHUDPresentationCoordinator(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

FWacomBattleHUDPresentationCoordinator::~FWacomBattleHUDPresentationCoordinator()
{
	PresentationPlanTimerHandle = FTimerHandle();
	PresentationPlan = FWacomBattlePresentationPlan();
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bProcessingPresentationPlan = false;
	bWaitingForPresentationPlanEventQueue = false;
#if WITH_AUTOMATION_TESTS
	StartedPresentationPlanPhaseNamesForTest.Reset();
#endif
	if (BattleEventPresentationQueue)
	{
		BattleEventPresentationQueue->AbandonWithoutWorldAccess();
		BattleEventPresentationQueue.Reset();
	}
	BattlePresentationStackExitTimerHandles.Reset();
	BattlePresentationStackExitingEntryIds.Reset();
	BattlePresentationStackEntries.Reset();
	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
}

void FWacomBattleHUDPresentationCoordinator::Shutdown()
{
	ClearQueue();
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
	RefreshCommandAvailabilityWidgets();
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
		FTimerHandle& TimerHandle = BattlePresentationStackExitTimerHandles.FindOrAdd(EntryId);
		World->GetTimerManager().ClearTimer(TimerHandle);
		World->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateRaw(this, &FWacomBattleHUDPresentationCoordinator::FinishStackEntryExit, EntryId),
			BattlePresentationStackExitSeconds,
			false);
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

	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* TimerHandle = BattlePresentationStackExitTimerHandles.Find(EntryId))
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}
	}
	BattlePresentationStackExitTimerHandles.Remove(EntryId);
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
	RefreshCommandAvailabilityWidgets();
	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::ClearStack()
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<int32, FTimerHandle>& Pair : BattlePresentationStackExitTimerHandles)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	BattlePresentationStackExitTimerHandles.Reset();
	BattlePresentationStackExitingEntryIds.Reset();
	BattlePresentationStackEntries.Reset();
	SyncStackWidget();
}

void FWacomBattleHUDPresentationCoordinator::EnqueueEvents(
	const TArray<FBattleEvent>& Events,
	int32 PresentationStackEntryId)
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
		BattleEventPresentationQueue = MakeShared<FWacomBattleEventPresentationQueue>(*this);
	}

	BattleEventPresentationQueue->EnqueueEvents(
		Events,
		PresentationStackEntryId,
		HUD.CardPresentationStackMinimumHoldSeconds);
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
	RefreshCommandAvailabilityWidgets();
	StartNextPresentationPlanPhase();
	return true;
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

	HUD.ClearBattleSceneEnemyPartHoverProbe(TEXT("PendingTurnBoundary"));
	PendingTurnBoundaryCommand = Command;
	if (HUD.UIState == EBattleUIState::TargetSelect)
	{
		HUD.PendingTargetingCardId.Invalidate();
		HUD.SetUIState(EBattleUIState::Idle);
	}
	RefreshCommandAvailabilityWidgets();
	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::ClearPendingTurnBoundaryCommand()
{
	if (PendingTurnBoundaryCommand == EWacomBattleHUDTurnBoundaryCommand::None)
	{
		return;
	}

	PendingTurnBoundaryCommand = EWacomBattleHUDTurnBoundaryCommand::None;
	RefreshCommandAvailabilityWidgets();
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

	UBattleSession* CurrentSession = HUD.GetSession();
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
	RefreshCommandAvailabilityWidgets();
	ExecuteTurnBoundaryCommandNow(CommandToExecute);
}

void FWacomBattleHUDPresentationCoordinator::HandleQueueStarted()
{
	HUD.HideCardDetailPanel();
}

void FWacomBattleHUDPresentationCoordinator::HandleQueueFinished()
{
	if (bProcessingPresentationPlan && bWaitingForPresentationPlanEventQueue)
	{
		bWaitingForPresentationPlanEventQueue = false;
		StartNextPresentationPlanPhase();
		return;
	}

	UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		HUD.SetUIState(EBattleUIState::BattleEnd);
		return;
	}

	TryExecutePendingTurnBoundaryCommand();
}

void FWacomBattleHUDPresentationCoordinator::HandleBattleEndStep()
{
	if (UBattleSession* CurrentSession = HUD.GetSession())
	{
		HUD.RefreshFromSnapshot(CurrentSession->BuildSnapshot());
	}
}

void FWacomBattleHUDPresentationCoordinator::HandleKnockdownChoiceDialogStep()
{
	HUD.PushPendingKnockdownChoiceDialog();
}

void FWacomBattleHUDPresentationCoordinator::HandleTargetCueStep(
	const FWacomBattlePresentationTargetCue& Cue)
{
	HUD.PlayBattlePresentationCue(Cue);
}

void FWacomBattleHUDPresentationCoordinator::HandleCardStackBoundaryStep(int32 EntryId)
{
	BeginStackEntryExit(EntryId);
}

UWorld* FWacomBattleHUDPresentationCoordinator::GetWorld() const
{
	return HUD.GetWorld();
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
#endif

void FWacomBattleHUDPresentationCoordinator::SyncStackWidget()
{
	if (HUD.BattlePresentationStack)
	{
		HUD.BattlePresentationStack->SetPresentationStackEntries(BattlePresentationStackEntries);
	}
}

void FWacomBattleHUDPresentationCoordinator::ClearPresentationPlan()
{
	StopPresentationPlanTimer();
	PresentationPlan = FWacomBattlePresentationPlan();
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bProcessingPresentationPlan = false;
	bWaitingForPresentationPlanEventQueue = false;
#if WITH_AUTOMATION_TESTS
	StartedPresentationPlanPhaseNamesForTest.Reset();
#endif
}

void FWacomBattleHUDPresentationCoordinator::StartNextPresentationPlanPhase()
{
	StopPresentationPlanTimer();
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
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
#if WITH_AUTOMATION_TESTS
	StartedPresentationPlanPhaseNamesForTest.Add(GetActivePresentationPlanPhaseName());
#endif

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
	HUD.RefreshFromPresentationPhase(
		Phase.Snapshot,
		Phase.TransitionHints,
		Phase.FeedbackHints);
	if (UWacomFirstPersonCardAnchorComponent* Anchor = HUD.ResolveActiveFirstPersonCardAnchor())
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
	bWaitingForPresentationPlanEventQueue = true;
	EnqueueEvents(Phase.Events, INDEX_NONE);
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
		World->GetTimerManager().ClearTimer(PresentationPlanTimerHandle);
		World->GetTimerManager().SetTimer(
			PresentationPlanTimerHandle,
			FTimerDelegate::CreateRaw(this, &FWacomBattleHUDPresentationCoordinator::PollActivePresentationPlanPhase),
			FMath::Max(0.01f, DelaySeconds),
			false);
		return;
	}

	PollActivePresentationPlanPhase();
}

void FWacomBattleHUDPresentationCoordinator::StopPresentationPlanTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PresentationPlanTimerHandle);
	}
	PresentationPlanTimerHandle = FTimerHandle();
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
	if ((!HasActiveFirstPersonHandPresentationPlayback()
			&& !HasPendingFirstPersonHandPresentationFrame())
		|| ActivePresentationPlanPhaseElapsedSeconds >= BattlePresentationPlanHandPhaseTimeoutSeconds)
	{
		StartNextPresentationPlanPhase();
		return;
	}

	SchedulePresentationPlanPoll(BattlePresentationPlanPollSeconds);
}

void FWacomBattleHUDPresentationCoordinator::FinishPresentationPlan()
{
	StopPresentationPlanTimer();
	PresentationPlan = FWacomBattlePresentationPlan();
	ActivePresentationPlanPhaseKind = EWacomBattlePresentationPhaseKind::None;
	ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	bProcessingPresentationPlan = false;
	bWaitingForPresentationPlanEventQueue = false;

	if (UBattleSession* CurrentSession = HUD.GetSession())
	{
		HUD.RefreshFromSnapshot(CurrentSession->BuildSnapshot());
	}
	TryExecutePendingTurnBoundaryCommand();
}

bool FWacomBattleHUDPresentationCoordinator::HasActiveFirstPersonHandPresentationPlayback() const
{
	const UWacomFirstPersonCardAnchorComponent* Anchor = HUD.ResolveActiveFirstPersonCardAnchor();
	return Anchor && Anchor->HasActiveCardLayerPresentationPlayback();
}

bool FWacomBattleHUDPresentationCoordinator::HasPendingFirstPersonHandPresentationFrame() const
{
	return HUD.GetFirstPersonHandBridge().HasPendingPresentationFrame();
}

void FWacomBattleHUDPresentationCoordinator::ExecuteTurnBoundaryCommandNow(
	EWacomBattleHUDTurnBoundaryCommand Command)
{
	switch (Command)
	{
	case EWacomBattleHUDTurnBoundaryCommand::Wait:
		FWacomBattleHUDCommandFlow::SubmitWait(HUD);
		break;
	case EWacomBattleHUDTurnBoundaryCommand::EndTurn:
		FWacomBattleHUDCommandFlow::SubmitEndTurn(HUD);
		break;
	case EWacomBattleHUDTurnBoundaryCommand::None:
	default:
		break;
	}
}

void FWacomBattleHUDPresentationCoordinator::RefreshCommandAvailabilityWidgets()
{
	UBattleSession* CurrentSession = HUD.GetSession();
	if (!CurrentSession)
	{
		return;
	}

	const FBattleSnapshot Snapshot = CurrentSession->BuildSnapshot();
	if (HUD.ActionPanel)
	{
		HUD.ActionPanel->RefreshFromSnapshot(Snapshot);
	}
	if (!IsPresentationPlanBusy())
	{
		HUD.SyncFirstPersonBattleHandLayer(Snapshot);
	}
	HUD.SyncBattleEnemyPartWorldTargets(Snapshot);
}
