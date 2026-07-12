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
struct FWacomBattlePresentationTargetCue;
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

	void EnqueueEvents(const TArray<FBattleEvent>& Events, int32 PresentationStackEntryId = INDEX_NONE);
	bool EnqueueEndTurnPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot);
	bool EnqueueDeckPresentationPlan(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		int32 PresentationStackEntryId = INDEX_NONE);
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
	void HandleCardStackBoundaryStep(int32 EntryId);

	UWorld* GetWorld() const;

#if WITH_AUTOMATION_TESTS
	void AdvanceQueueOnce();
	void AdvancePresentationPlanOnce();
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
	float ActivePresentationPlanPhaseElapsedSeconds = 0.0f;
	int32 NextBattlePresentationStackEntryId = 1;
	EWacomBattleHUDTurnBoundaryCommand PendingTurnBoundaryCommand =
		EWacomBattleHUDTurnBoundaryCommand::None;
	bool bProcessingPresentationPlan = false;
	bool bWaitingForPresentationPlanEventQueue = false;
	int32 ActivePileTransferEventSequence = INDEX_NONE;
	int32 ActivePileTransferTotalCount = 0;
#if WITH_AUTOMATION_TESTS
	TArray<FName> StartedPresentationPlanPhaseNamesForTest;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> SubmittedPresentationPlanFeedbackHintsForTest;
#endif

	void SyncStackWidget();
	void ExecuteTurnBoundaryCommandNow(EWacomBattleHUDTurnBoundaryCommand Command);
	void RefreshCommandBarOnly();
	void RefreshCommandBar();
	void ClearPresentationPlan();
	void StartNextPresentationPlanPhase();
	void StartHandPresentationPlanPhase(FWacomBattlePresentationPhase&& Phase);
	void StartEventPresentationPlanPhase(FWacomBattlePresentationPhase&& Phase);
	void SchedulePresentationPlanPoll(float DelaySeconds);
	void StopPresentationPlanTimer();
	void PollActivePresentationPlanPhase();
	void FinishPresentationPlan();
	bool HasActiveFirstPersonHandPresentationPlayback() const;
	bool HasPendingFirstPersonHandPresentationFrame() const;
};
