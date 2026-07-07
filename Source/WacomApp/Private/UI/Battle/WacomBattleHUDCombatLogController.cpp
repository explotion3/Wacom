// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCombatLogController.h"

#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

FWacomBattleHUDCombatLogController::FWacomBattleHUDCombatLogController(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDCombatLogController::AppendBlock(const FWacomBattleCombatLogBlockView& Block)
{
	if (!Block.bShouldDisplay)
	{
		return;
	}

	BattleCombatLogHistory.Add(Block);
	UE_LOG(LogTemp, Display, TEXT("[BattleCombatLog] %s"),
		*UWacomBattleCombatLogBuilder::FormatCombatLogBlockForLog(Block));
	Trim();
	SyncFeed();
}

void FWacomBattleHUDCombatLogController::AppendBlock(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	AppendBlock(UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
		CommandContext,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot));
}

void FWacomBattleHUDCombatLogController::Clear()
{
	BattleCombatLogHistory.Reset();
	SyncFeed();
}

void FWacomBattleHUDCombatLogController::Trim()
{
	const int32 SafeMaxEntries = FMath::Max(1, Runtime.Host().GetBattleCombatLogMaxBlocks());
	if (BattleCombatLogHistory.Num() > SafeMaxEntries)
	{
		BattleCombatLogHistory.RemoveAt(0, BattleCombatLogHistory.Num() - SafeMaxEntries);
	}
}

void FWacomBattleHUDCombatLogController::SyncFeed()
{
	if (UBattleCombatLogFeedWidget* CombatLogFeed = Runtime.Host().GetCombatLogFeed())
	{
		CombatLogFeed->MaxVisibleBlocks = FMath::Max(1, Runtime.Host().GetBattleCombatLogMaxBlocks());
		CombatLogFeed->SetCombatLogBlocks(BattleCombatLogHistory);
	}
}
