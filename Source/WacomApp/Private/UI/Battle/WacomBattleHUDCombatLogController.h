// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattlePresentationProgress.h"

class FWacomBattleHUDRuntime;
class FWacomBattleCombatActivitySynchronizer;
struct FWacomBattleCombatActivityEmission;
struct FBattleEvent;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;

class FWacomBattleHUDCombatLogController
{
public:
	explicit FWacomBattleHUDCombatLogController(FWacomBattleHUDRuntime& InRuntime);
	~FWacomBattleHUDCombatLogController();

	void AppendBlock(const FWacomBattleCombatLogBlockView& Block);
	void AppendBlock(
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);
	uint64 StageResolvedCommand(
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);
	void ApplyPresentationProgress(const FWacomBattlePresentationProgress& Progress);
	void FlushActivityTransaction(uint64 TransactionId);
	void DiscardActivityTransaction(uint64 TransactionId);
	void PresentInitialTurnActivity(int32 TurnNumber);
	void Clear();
	void Trim();
	void SyncFeed();

	int32 GetBlockCount() const { return BattleCombatLogHistory.Num(); }
	const TArray<FWacomBattleCombatLogBlockView>& GetHistory() const
	{
		return BattleCombatLogHistory;
	}
	const TArray<FWacomBattleCombatLogTurnSectionView>& GetDetailsHistory() const
	{
		return BattleCombatLogDetailsHistory;
	}

private:
	FWacomBattleHUDRuntime& Runtime;
	TArray<FWacomBattleCombatLogBlockView> BattleCombatLogHistory;
	TArray<FWacomBattleCombatLogTurnSectionView> BattleCombatLogDetailsHistory;
	TOptional<FWacomBattleCombatActivityRowView> LastProjectedRootAction;
	TUniquePtr<FWacomBattleCombatActivitySynchronizer> ActivitySynchronizer;
	int32 LastProjectedTurnNumber = 0;

	void AppendHistoryBlock(const FWacomBattleCombatLogBlockView& Block);
	void SubmitActivityBatch(
		const FWacomBattleCombatActivityBatchView& Batch,
		bool bAppendToDetailsHistory = true);
	void ApplyActivityEmissions(
		const TArray<FWacomBattleCombatActivityEmission>& Emissions);
	FWacomBattleCombatLogTurnSectionView& EnsureDetailsTurnSection(int32 TurnNumber);
	void AppendDetailsBatch(const FWacomBattleCombatActivityBatchView& Batch);
	void TrimDetailsHistory();
};
