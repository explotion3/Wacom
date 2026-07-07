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
	void Clear();
	void Trim();
	void SyncFeed();

	int32 GetBlockCount() const { return BattleCombatLogHistory.Num(); }
	const TArray<FWacomBattleCombatLogBlockView>& GetHistory() const
	{
		return BattleCombatLogHistory;
	}

private:
	FWacomBattleHUDRuntime& Runtime;
	TArray<FWacomBattleCombatLogBlockView> BattleCombatLogHistory;
};
