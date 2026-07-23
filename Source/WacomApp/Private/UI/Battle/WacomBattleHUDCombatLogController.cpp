// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCombatLogController.h"

#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/WacomBattleCombatActivitySynchronizer.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

FWacomBattleHUDCombatLogController::FWacomBattleHUDCombatLogController(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
	, ActivitySynchronizer(MakeUnique<FWacomBattleCombatActivitySynchronizer>())
{
}

FWacomBattleHUDCombatLogController::~FWacomBattleHUDCombatLogController() = default;

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
	AppendDetailsBatch(UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
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

uint64 FWacomBattleHUDCombatLogController::StageResolvedCommand(
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
	const FWacomBattleCombatLogDetailsBatchView DetailsBatch =
		UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
			CommandContext,
			Events,
			PreCommandSnapshot,
			PostCommandSnapshot);
	AppendDetailsBatch(DetailsBatch);
	const FWacomBattleCombatActivityBatchView ActivityBatch =
		UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
			CommandContext,
			Events,
			PreCommandSnapshot,
			PostCommandSnapshot);
	return ActivitySynchronizer->Stage(ActivityBatch);
}

void FWacomBattleHUDCombatLogController::ApplyPresentationProgress(
	const FWacomBattlePresentationProgress& Progress)
{
	bool bFlushedRemainder = false;
	const TArray<FWacomBattleCombatActivityEmission> Emissions =
		ActivitySynchronizer->ApplyProgress(Progress, bFlushedRemainder);
	if (bFlushedRemainder)
	{
		TArray<FString> FlushedSequences;
		for (const FWacomBattleCombatActivityEmission& Emission : Emissions)
		{
			if (Emission.Kind == EWacomBattleCombatActivityEmissionKind::BeginGroup
				&& Emission.RootAction.EventSequence > 0)
			{
				FlushedSequences.Add(FString::FromInt(Emission.RootAction.EventSequence));
			}
			for (const FWacomBattleCombatActivityRowView& Row : Emission.ResultRows)
			{
				if (Row.EventSequence > 0)
				{
					FlushedSequences.Add(FString::FromInt(Row.EventSequence));
				}
			}
		}
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleCombatActivity] Presentation transaction %llu completed with pending activity sequences: %s"),
			Progress.ActivityTransactionId,
			*FString::Join(FlushedSequences, TEXT(",")));
	}
	ApplyActivityEmissions(Emissions);
}

void FWacomBattleHUDCombatLogController::FlushActivityTransaction(
	const uint64 TransactionId)
{
	FWacomBattlePresentationProgress Progress;
	Progress.ActivityTransactionId = TransactionId;
	Progress.Kind = EWacomBattlePresentationProgressKind::PlanCompleted;
	ApplyPresentationProgress(Progress);
}

void FWacomBattleHUDCombatLogController::DiscardActivityTransaction(
	const uint64 TransactionId)
{
	ActivitySynchronizer->Discard(TransactionId);
}

void FWacomBattleHUDCombatLogController::PresentInitialTurnActivity(const int32 TurnNumber)
{
	SubmitActivityBatch(
		UWacomBattleCombatLogBuilder::BuildInitialTurnActivityBatch(TurnNumber));
}

void FWacomBattleHUDCombatLogController::Clear()
{
	BattleCombatLogHistory.Reset();
	BattleCombatLogDetailsHistory.Reset();
	ActivitySynchronizer->Clear();
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

void FWacomBattleHUDCombatLogController::ApplyActivityEmissions(
	const TArray<FWacomBattleCombatActivityEmission>& Emissions)
{
	UBattleCombatLogFeedWidget* CombatLogFeed = Runtime.Host().GetCombatLogFeed();
	for (const FWacomBattleCombatActivityEmission& Emission : Emissions)
	{
		switch (Emission.Kind)
		{
		case EWacomBattleCombatActivityEmissionKind::BeginGroup:
			LastProjectedRootAction = Emission.RootAction;
			if (LastProjectedTurnNumber <= 0 && Emission.TurnNumber > 0)
			{
				LastProjectedTurnNumber = Emission.TurnNumber;
			}
			if (CombatLogFeed)
			{
				CombatLogFeed->BeginSynchronizedCombatActivityGroup(
					Emission.TransactionId,
					Emission.GroupIndex,
					Emission.RootAction,
					Emission.TurnNumber);
			}
			break;

		case EWacomBattleCombatActivityEmissionKind::AppendResults:
			if (CombatLogFeed)
			{
				CombatLogFeed->ReleaseSynchronizedCombatActivityResults(
					Emission.TransactionId,
					Emission.GroupIndex,
					Emission.ResultRows);
			}
			break;

		case EWacomBattleCombatActivityEmissionKind::SetTurn:
			LastProjectedTurnNumber = FMath::Max(0, Emission.TurnNumber);
			if (CombatLogFeed)
			{
				CombatLogFeed->SetPresentedTurnNumber(LastProjectedTurnNumber);
			}
			break;

		case EWacomBattleCombatActivityEmissionKind::CompleteTransaction:
			if (CombatLogFeed)
			{
				CombatLogFeed->CompleteSynchronizedCombatActivityTransaction(
					Emission.TransactionId);
			}
			break;
		}
	}
}

void FWacomBattleHUDCombatLogController::SubmitActivityBatch(
	const FWacomBattleCombatActivityBatchView& Batch)
{
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
	const FWacomBattleCombatLogDetailsBatchView& Batch)
{
	if (Batch.bSetTurnImmediately)
	{
		EnsureDetailsTurnSection(Batch.PresentedTurnNumber);
	}

	for (const FWacomBattleCombatLogDetailsGroupView& Group : Batch.Groups)
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
