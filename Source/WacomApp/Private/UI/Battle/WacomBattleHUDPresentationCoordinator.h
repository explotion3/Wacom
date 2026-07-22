// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/WacomBattlePresentationPlan.h"
#include "UI/Battle/WacomBattlePresentationProgress.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

class FWacomBattleEventPresentationQueue;
class FWacomBattlePresentationTimerOwner;
struct FBattleEvent;
struct FBattlePartSlotIdentity;
struct FBattlePresentationJournal;
struct FBattlePresentationEnemyActionStep;
struct FWacomBattleEnemyActionPlaybackCallbacks;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;
struct FWacomBattleCommandPresentationContext;
struct FWacomBattlePresentationTargetCue;
struct FBattleResolution;
class FWacomBattleHUDRuntime;

class FWacomBattleHUDPresentationCoordinator
{
public:
	explicit FWacomBattleHUDPresentationCoordinator(FWacomBattleHUDRuntime& InRuntime);
	~FWacomBattleHUDPresentationCoordinator();

	void Shutdown();

	int32 AppendStackEntry(
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const FBattleSnapshot& PreCommandSnapshot);
	void BeginStackEntryExit(int32 EntryId);
	void FinishStackEntryExit(int32 EntryId);
	void ClearStack();
	bool HasStackEntries() const { return BattlePresentationStackEntries.Num() > 0; }
	const TArray<FWacomBattlePresentationStackEntryView>& GetStackEntries() const
	{
		return BattlePresentationStackEntries;
	}

	void EnqueueEvents(
		const TArray<FBattleEvent>& Events,
		int32 PresentationStackEntryId = INDEX_NONE,
		bool bTargetAlreadyConfirmed = false);
	void EnqueueEvents(
		const TArray<FBattleEvent>& Events,
		const TArray<FBattlePresentationEnemyActionStep>& EnemyActionSteps,
		int32 PresentationStackEntryId,
		bool bTargetAlreadyConfirmed = false);
	bool EnqueueEndTurnPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot,
		uint64 ActivityTransactionId = 0);
	bool EnqueueResolvedCommandPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		int32 PresentationStackEntryId = INDEX_NONE,
		uint64 ActivityTransactionId = 0);
	bool EnqueuePlayCardPresentationPlan(
		const FWacomBattleCommandPresentationContext& Context,
		const FBattleResolution& Resolution,
		int32 PresentationStackEntryId,
		uint64 ActivityTransactionId = 0);
	void HandlePileTransferProgress(const FWacomFirstPersonCardPileTransferProgressView& Progress);
	void ClearQueue();
	bool IsQueueBusy() const;
	bool IsPresentationPlanBusy() const { return bProcessingPresentationPlan; }
	int32 GetPendingPresentationPlanPhaseCount() const { return PresentationPlan.Phases.Num(); }
	FName GetActivePresentationPlanPhaseName() const;
	bool IsBusy() const { return IsQueueBusy() || HasStackEntries() || IsPresentationPlanBusy(); }

	void QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand Command);
	void ClearPendingTurnBoundaryCommand();
	bool HasPendingTurnBoundaryCommand() const
	{
		return PendingTurnBoundaryCommand != EWacomBattleHUDTurnBoundaryCommand::None;
	}
	EWacomBattleHUDTurnBoundaryCommand GetPendingTurnBoundaryCommand() const
	{
		return PendingTurnBoundaryCommand;
	}
	FText GetPendingTurnBoundaryCommandText() const;
	void TryExecutePendingTurnBoundaryCommand();

#if WITH_AUTOMATION_TESTS
	void AdvanceQueueOnce();
	void AdvancePresentationPlanOnce();
	void PrimeDiscardPileReceiveFeedbackForTest(
		int32 EventSequence,
		int32 TotalCount,
		int32 InitialDiscardCount);
	void PrimeReshufflePileFeedbackForTest(
		int32 EventSequence,
		int32 TotalCount,
		int32 InitialDrawCount,
		int32 FinalDrawCount,
		int32 InitialDiscardCount,
		int32 FinalDiscardCount,
		int32 PlayedCount);
	const TArray<FName>& GetStartedPresentationPlanPhaseNamesForTest() const
	{
		return StartedPresentationPlanPhaseNamesForTest;
	}
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& GetSubmittedPresentationPlanFeedbackHintsForTest() const
	{
		return SubmittedPresentationPlanFeedbackHintsForTest;
	}
#endif

private:
	friend class FWacomBattleEventPresentationQueue;

	FWacomBattleHUDRuntime& Runtime;
	TSharedPtr<FWacomBattlePresentationTimerOwner> PresentationTimerOwner;
	TArray<FWacomBattlePresentationStackEntryView> BattlePresentationStackEntries;
	TSharedPtr<FWacomBattleEventPresentationQueue> BattleEventPresentationQueue;
	TArray<int32> BattlePresentationStackExitingEntryIds;
	FWacomBattlePresentationPlan PresentationPlan;
	EWacomBattlePresentationPhaseKind ActivePresentationPlanPhaseKind =
		EWacomBattlePresentationPhaseKind::None;
	EWacomBattlePresentationPhaseCompletionPolicy ActivePresentationPlanCompletionPolicy =
		EWacomBattlePresentationPhaseCompletionPolicy::PlaybackIdle;
	FGuid ActivePresentationPlanCompletionCardId;
	int32 ActivePresentationPlanCompletionStackEntryId = INDEX_NONE;
	float ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	int32 NextBattlePresentationStackEntryId = 1;
	EWacomBattleHUDTurnBoundaryCommand PendingTurnBoundaryCommand =
		EWacomBattleHUDTurnBoundaryCommand::None;
	bool bProcessingPresentationPlan = false;
	bool bWaitingForPresentationPlanEventQueue = false;
	int32 ActivePileTransferEventSequence = INDEX_NONE;
	int32 ActivePileTransferTotalCount = 0;
	float ActivePileTransferExpectedDurationSeconds = 0.0f;
	FWacomFirstPersonCardPileTransferHint::ETransferKind ActivePileTransferKind =
		FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
	int32 ActivePileTransferInitialDrawCount = 0;
	int32 ActivePileTransferInitialDiscardCount = 0;
	int32 ActivePileTransferFinalDrawCount = 0;
	int32 ActivePileTransferFinalDiscardCount = 0;
	int32 ActivePileTransferPlayedCount = 0;
	int32 ActivePileTransferLastLaunchedCount = 0;
	int32 ActivePileTransferLastArrivedCount = 0;
#if WITH_AUTOMATION_TESTS
	TArray<FName> StartedPresentationPlanPhaseNamesForTest;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> SubmittedPresentationPlanFeedbackHintsForTest;
#endif

	void SyncStackWidget();
	void HandleQueueStarted();
	void HandleQueueFinished();
	void HandleBattleEndStep();
	void HandleKnockdownChoiceDialogStep();
	void HandleTargetCueStep(const FWacomBattlePresentationTargetCue& Cue);
	void HandleSceneEnemyActionStarted(int32 EventSequence);
	void HandleSceneEnemyAnimationStep(
		const FBattlePartSlotIdentity& ActingPartKey,
		FName IntentId,
		bool bDestroyed,
		int32 EventSequence,
		FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks);
	void HandleSceneEnemyActionImpact(const FBattlePresentationEnemyActionStep& ActionStep);
	void HandleCardStackBoundaryStep(int32 EntryId);
	UWorld* GetWorld() const;
	void ExecuteTurnBoundaryCommandNow(EWacomBattleHUDTurnBoundaryCommand Command);
	void RefreshCommandBarOnly();
	void RefreshCommandBar();
	void RestoreActiveReshufflePileCounts();
	void ResetActivePileTransferFeedback();
	void ClearPresentationPlan();
	void NotifyPresentationPlanStarted();
	void NotifyPresentationPhaseStarted(const FWacomBattlePresentationPhase& Phase);
	void NotifyPresentationPlanCompleted();
	void NotifyPresentationPlanCancelled(EWacomBattlePresentationCancelPolicy CancelPolicy);
	void StartNextPresentationPlanPhase();
	void StartHandPresentationPlanPhase(FWacomBattlePresentationPhase&& Phase);
	void StartEventPresentationPlanPhase(FWacomBattlePresentationPhase&& Phase);
	void StartTargetCuePresentationPlanPhase(FWacomBattlePresentationPhase&& Phase);
	void SchedulePresentationPlanPoll(float DelaySeconds);
	void StopPresentationPlanTimer();
	void PollActivePresentationPlanPhase();
	void FinishPresentationPlan();
	bool HasActiveFirstPersonHandPresentationPlayback() const;
	bool HasPendingFirstPersonHandPresentationFrame() const;
};
