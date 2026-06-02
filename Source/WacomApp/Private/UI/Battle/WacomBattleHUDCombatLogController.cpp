// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCombatLogController.h"

#include "UI/Battle/BattleHUD.h"

#include "UI/Battle/BattleCombatLogFeedWidget.h"

FWacomBattleHUDCombatLogController::FWacomBattleHUDCombatLogController(UBattleHUD& InHUD)
	: HUD(InHUD)
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
	const int32 SafeMaxEntries = FMath::Max(1, HUD.BattleCombatLogMaxBlocks);
	if (BattleCombatLogHistory.Num() > SafeMaxEntries)
	{
		BattleCombatLogHistory.RemoveAt(0, BattleCombatLogHistory.Num() - SafeMaxEntries);
	}
}

void FWacomBattleHUDCombatLogController::SyncFeed()
{
	if (HUD.CombatLogFeed)
	{
		HUD.CombatLogFeed->MaxVisibleBlocks = FMath::Max(1, HUD.BattleCombatLogMaxBlocks);
		HUD.CombatLogFeed->SetCombatLogBlocks(BattleCombatLogHistory);
	}
}
