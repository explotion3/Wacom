// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"

class FWacomBattleHUDRuntime;
struct FBattleEvent;
struct FBattleSnapshot;
struct FWacomBattleCombatLogCommandContext;

class FWacomBattleHUDCombatLogController
{
public:
	explicit FWacomBattleHUDCombatLogController(FWacomBattleHUDRuntime& InRuntime);

	void AppendBlock(const FWacomBattleCombatLogBlockView& Block);
	void AppendBlock(
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot);
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
	int32 LastProjectedTurnNumber = 0;

	void AppendHistoryBlock(const FWacomBattleCombatLogBlockView& Block);
	void SubmitActivityBatch(
		const FWacomBattleCombatActivityBatchView& Batch,
		bool bAppendToDetailsHistory = true);
	FWacomBattleCombatLogTurnSectionView& EnsureDetailsTurnSection(int32 TurnNumber);
	void AppendDetailsBatch(const FWacomBattleCombatActivityBatchView& Batch);
	void TrimDetailsHistory();
};
