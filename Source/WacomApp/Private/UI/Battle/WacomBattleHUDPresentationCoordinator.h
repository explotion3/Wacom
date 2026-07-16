// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/WacomBattlePresentationPlan.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

class FWacomBattleEventPresentationQueue;
struct FBattleEvent;
struct FBattlePresentationJournal;
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
	bool EnqueueEndTurnPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot);
	bool EnqueueResolvedCommandPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		int32 PresentationStackEntryId = INDEX_NONE);
	bool EnqueuePlayCardPresentationPlan(
		const FWacomBattleCommandPresentationContext& Context,
		const FBattleResolution& Resolution,
		int32 PresentationStackEntryId);
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

	TSharedPtr<FWacomBattleEventPresentationQueue> GetQueueSelfKeepAlive() const
	{
		return BattleEventPresentationQueue;
	}

	void HandleQueueStarted();
	void HandleQueueFinished();
	void HandleBattleEndStep();
	void HandleKnockdownChoiceDialogStep();
	void HandleTargetCueStep(const FWacomBattlePresentationTargetCue& Cue);
	void HandleHostAnimationStep(
		FName EnemySlotId,
		FName IntentId,
		bool bDestroyed,
		TFunction<void()>&& Completion);
	void HandleCardStackBoundaryStep(int32 EntryId);

	UWorld* GetWorld() const;

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
	FWacomBattleHUDRuntime& Runtime;
	TArray<FWacomBattlePresentationStackEntryView> BattlePresentationStackEntries;
	TSharedPtr<FWacomBattleEventPresentationQueue> BattleEventPresentationQueue;
	TArray<int32> BattlePresentationStackExitingEntryIds;
	TMap<int32, FTimerHandle> BattlePresentationStackExitTimerHandles;
	FTimerHandle PresentationPlanTimerHandle;
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
	void ExecuteTurnBoundaryCommandNow(EWacomBattleHUDTurnBoundaryCommand Command);
	void RefreshCommandBarOnly();
	void RefreshCommandBar();
	void RestoreActiveReshufflePileCounts();
	void ResetActivePileTransferFeedback();
	void ClearPresentationPlan();
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
