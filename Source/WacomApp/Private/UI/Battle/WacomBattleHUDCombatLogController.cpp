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
	AppendHistoryBlock(Block);
	SyncFeed();
}

void FWacomBattleHUDCombatLogController::AppendHistoryBlock(const FWacomBattleCombatLogBlockView& Block)
{
	if (!Block.bShouldDisplay)
	{
		return;
	}

	BattleCombatLogHistory.Add(Block);
	UE_LOG(LogTemp, Display, TEXT("[BattleCombatLog] %s"),
		*UWacomBattleCombatLogBuilder::FormatCombatLogBlockForLog(Block));
	Trim();
}

void FWacomBattleHUDCombatLogController::AppendBlock(
	const FWacomBattleCombatLogCommandContext& CommandContext,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	AppendHistoryBlock(UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
		CommandContext,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot));
	SubmitActivityBatch(UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
		CommandContext,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot));
}

void FWacomBattleHUDCombatLogController::PresentInitialTurnActivity(const int32 TurnNumber)
{
	SubmitActivityBatch(
		UWacomBattleCombatLogBuilder::BuildInitialTurnActivityBatch(TurnNumber),
		false);
}

void FWacomBattleHUDCombatLogController::Clear()
{
	BattleCombatLogHistory.Reset();
	BattleCombatLogDetailsHistory.Reset();
	LastProjectedRootAction.Reset();
	LastProjectedTurnNumber = 0;
	if (UBattleCombatLogFeedWidget* CombatLogFeed = Runtime.Host().GetCombatLogFeed())
	{
		CombatLogFeed->ClearCombatActivity();
	}
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
		CombatLogFeed->RestorePersistentState(
			LastProjectedTurnNumber,
			LastProjectedRootAction.IsSet() ? &LastProjectedRootAction.GetValue() : nullptr);
	}
}

void FWacomBattleHUDCombatLogController::SubmitActivityBatch(
	const FWacomBattleCombatActivityBatchView& Batch,
	const bool bAppendToDetailsHistory)
{
	if (bAppendToDetailsHistory)
	{
		AppendDetailsBatch(Batch);
	}
	if (Batch.bSetTurnImmediately || Batch.bAdvanceTurnAfterPlayback)
	{
		LastProjectedTurnNumber = Batch.PresentedTurnNumber;
	}
	if (!Batch.Groups.IsEmpty())
	{
		LastProjectedRootAction = Batch.Groups.Last().RootAction;
	}
	if (UBattleCombatLogFeedWidget* CombatLogFeed = Runtime.Host().GetCombatLogFeed())
	{
		CombatLogFeed->EnqueueCombatActivityBatch(Batch);
	}
}

FWacomBattleCombatLogTurnSectionView&
FWacomBattleHUDCombatLogController::EnsureDetailsTurnSection(int32 TurnNumber)
{
	const int32 SafeTurnNumber = FMath::Max(TurnNumber, 1);
	if (FWacomBattleCombatLogTurnSectionView* Existing =
		BattleCombatLogDetailsHistory.FindByPredicate(
			[SafeTurnNumber](const FWacomBattleCombatLogTurnSectionView& Section)
			{
				return Section.TurnNumber == SafeTurnNumber;
			}))
	{
		return *Existing;
	}

	FWacomBattleCombatLogTurnSectionView& Added =
		BattleCombatLogDetailsHistory.AddDefaulted_GetRef();
	Added.TurnNumber = SafeTurnNumber;
	return Added;
}

void FWacomBattleHUDCombatLogController::AppendDetailsBatch(
	const FWacomBattleCombatActivityBatchView& Batch)
{
	if (Batch.bSetTurnImmediately)
	{
		EnsureDetailsTurnSection(Batch.PresentedTurnNumber);
	}

	for (const FWacomBattleCombatActivityGroupView& Group : Batch.Groups)
	{
		EnsureDetailsTurnSection(Group.TurnNumber).Groups.Add(Group);
	}

	if (Batch.bAdvanceTurnAfterPlayback)
	{
		const int32 NextTurnNumber = FMath::Max(Batch.PresentedTurnNumber, 1);
		const int32 CompletedTurnNumber = FMath::Max(NextTurnNumber - 1, 1);
		EnsureDetailsTurnSection(CompletedTurnNumber).bCompleted = true;
		EnsureDetailsTurnSection(NextTurnNumber);
	}

	TrimDetailsHistory();
}

void FWacomBattleHUDCombatLogController::TrimDetailsHistory()
{
	const int32 SafeMaxGroups = FMath::Max(1, Runtime.Host().GetBattleCombatLogMaxBlocks());
	auto CountGroups = [this]()
	{
		int32 Count = 0;
		for (const FWacomBattleCombatLogTurnSectionView& Section : BattleCombatLogDetailsHistory)
		{
			Count += Section.Groups.Num();
		}
		return Count;
	};

	int32 GroupCount = CountGroups();
	while (GroupCount > SafeMaxGroups && BattleCombatLogDetailsHistory.Num() > 1
		&& BattleCombatLogDetailsHistory[0].bCompleted)
	{
		GroupCount -= BattleCombatLogDetailsHistory[0].Groups.Num();
		BattleCombatLogDetailsHistory.RemoveAt(0);
	}

	if (GroupCount > SafeMaxGroups && !BattleCombatLogDetailsHistory.IsEmpty())
	{
		FWacomBattleCombatLogTurnSectionView& Oldest = BattleCombatLogDetailsHistory[0];
		const int32 Excess = FMath::Min(GroupCount - SafeMaxGroups, Oldest.Groups.Num());
		if (Excess > 0)
		{
			Oldest.Groups.RemoveAt(0, Excess);
		}
	}
}
