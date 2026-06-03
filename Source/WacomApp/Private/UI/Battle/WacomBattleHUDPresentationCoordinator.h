// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"

class FWacomBattleEventPresentationQueue;
class UBattleHUD;
struct FBattleEvent;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;
struct FWacomBattlePresentationTargetCue;

enum class EWacomBattleHUDTurnBoundaryCommand : uint8
{
	None,
	Wait,
	EndTurn,
};

class FWacomBattleHUDPresentationCoordinator
{
public:
	explicit FWacomBattleHUDPresentationCoordinator(UBattleHUD& InHUD);
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
	void ClearQueue();
	bool IsQueueBusy() const;
	bool IsBusy() const { return IsQueueBusy() || HasStackEntries(); }

	void QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand Command);
	void ClearPendingTurnBoundaryCommand();
	bool HasPendingTurnBoundaryCommand() const
	{
		return PendingTurnBoundaryCommand != EWacomBattleHUDTurnBoundaryCommand::None;
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
#endif

private:
	UBattleHUD& HUD;
	TArray<FWacomBattlePresentationStackEntryView> BattlePresentationStackEntries;
	TSharedPtr<FWacomBattleEventPresentationQueue> BattleEventPresentationQueue;
	TArray<int32> BattlePresentationStackExitingEntryIds;
	TMap<int32, FTimerHandle> BattlePresentationStackExitTimerHandles;
	int32 NextBattlePresentationStackEntryId = 1;
	EWacomBattleHUDTurnBoundaryCommand PendingTurnBoundaryCommand =
		EWacomBattleHUDTurnBoundaryCommand::None;

	void SyncStackWidget();
	void ExecuteTurnBoundaryCommandNow(EWacomBattleHUDTurnBoundaryCommand Command);
	void RefreshCommandAvailabilityWidgets();
};
