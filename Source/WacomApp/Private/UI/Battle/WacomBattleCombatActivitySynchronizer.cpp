// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatActivitySynchronizer.h"

uint64 FWacomBattleCombatActivitySynchronizer::Stage(
	const FWacomBattleCombatActivityBatchView& Batch)
{
	uint64 TransactionId = NextTransactionId++;
	if (TransactionId == 0)
	{
		TransactionId = NextTransactionId++;
	}

	FPendingTransaction& Transaction = PendingTransactions.Add(TransactionId);
	Transaction.Batch = Batch;
	Transaction.Groups.Reserve(Batch.Groups.Num());
	for (int32 GroupIndex = 0; GroupIndex < Batch.Groups.Num(); ++GroupIndex)
	{
		FPendingGroup& PendingGroup = Transaction.Groups.AddDefaulted_GetRef();
		PendingGroup.Group = Batch.Groups[GroupIndex];
		PendingGroup.GroupIndex = GroupIndex;
	}
	return TransactionId;
}

TArray<FWacomBattleCombatActivityEmission>
FWacomBattleCombatActivitySynchronizer::ApplyProgress(
	const FWacomBattlePresentationProgress& Progress,
	bool& bOutFlushedRemainder)
{
	bOutFlushedRemainder = false;
	if (Progress.PresentationTransactionId == 0)
	{
		return {};
	}

	FPendingTransaction* Transaction =
		PendingTransactions.Find(Progress.PresentationTransactionId);
	if (!Transaction)
	{
		return {};
	}

	TArray<FWacomBattleCombatActivityEmission> Emissions;
	switch (Progress.Kind)
	{
	case EWacomBattlePresentationProgressKind::PlanStarted:
		for (FPendingGroup& Group : Transaction->Groups)
		{
			if (!IsEnemyGroup(Group))
			{
				EmitRootIfNeeded(Progress.PresentationTransactionId, Group, Emissions);
				break;
			}
		}
		break;

	case EWacomBattlePresentationProgressKind::PhaseEventsReached:
	{
		TSet<int32> ReachedSequences;
		for (const int32 EventSequence : Progress.EventSequences)
		{
			ReachedSequences.Add(EventSequence);
		}
		for (FPendingGroup& Group : Transaction->Groups)
		{
			EmitMatchingResults(
				Progress.PresentationTransactionId,
				Group,
				[&ReachedSequences](const int32 EventSequence)
				{
					return EventSequence > 0 && ReachedSequences.Contains(EventSequence);
				},
				Emissions);
		}
		break;
	}

	case EWacomBattlePresentationProgressKind::EnemyActionStarted:
		for (FPendingGroup& Group : Transaction->Groups)
		{
			if (IsEnemyGroup(Group)
				&& MatchesSequenceRange(
					Group.Group.RootAction.EventSequence,
					Progress.FirstEventSequence,
					Progress.LastEventSequence))
			{
				EmitRootIfNeeded(Progress.PresentationTransactionId, Group, Emissions);
				break;
			}
		}
		break;

	case EWacomBattlePresentationProgressKind::EnemyActionImpact:
		for (FPendingGroup& Group : Transaction->Groups)
		{
			if (!IsEnemyGroup(Group)
				|| !MatchesSequenceRange(
					Group.Group.RootAction.EventSequence,
					Progress.FirstEventSequence,
					Progress.LastEventSequence))
			{
				continue;
			}
			EmitRootIfNeeded(Progress.PresentationTransactionId, Group, Emissions);
			EmitMatchingResults(
				Progress.PresentationTransactionId,
				Group,
				[&Progress](const int32 EventSequence)
				{
					return MatchesSequenceRange(
						EventSequence,
						Progress.FirstEventSequence,
						Progress.LastEventSequence);
				},
				Emissions);
		}
		break;

	case EWacomBattlePresentationProgressKind::TurnAdvanced:
		if (!Transaction->bTurnReleased && Progress.PresentedTurnNumber > 0)
		{
			FWacomBattleCombatActivityEmission& Emission = Emissions.AddDefaulted_GetRef();
			Emission.Kind = EWacomBattleCombatActivityEmissionKind::SetTurn;
			Emission.TransactionId = Progress.PresentationTransactionId;
			Emission.TurnNumber = Progress.PresentedTurnNumber;
			Transaction->bTurnReleased = true;
		}
		break;

	case EWacomBattlePresentationProgressKind::PlanCompleted:
		return Flush(Progress.PresentationTransactionId, bOutFlushedRemainder);

	case EWacomBattlePresentationProgressKind::PlanCancelled:
		if (Progress.CancelPolicy == EWacomBattlePresentationCancelPolicy::DiscardPending)
		{
			Discard(Progress.PresentationTransactionId);
			return {};
		}
		return Flush(Progress.PresentationTransactionId, bOutFlushedRemainder);
	}

	return Emissions;
}

TArray<FWacomBattleCombatActivityEmission>
FWacomBattleCombatActivitySynchronizer::Flush(
	const uint64 TransactionId,
	bool& bOutFlushedRemainder)
{
	bOutFlushedRemainder = false;
	FPendingTransaction* Transaction = PendingTransactions.Find(TransactionId);
	if (!Transaction)
	{
		return {};
	}

	bOutFlushedRemainder = HasUnreleasedContent(*Transaction);
	TArray<FWacomBattleCombatActivityEmission> Emissions;
	for (FPendingGroup& Group : Transaction->Groups)
	{
		EmitRootIfNeeded(TransactionId, Group, Emissions);
		EmitMatchingResults(
			TransactionId,
			Group,
			[](const int32 /*EventSequence*/)
			{
				return true;
			},
			Emissions);
	}

	if (!Transaction->bTurnReleased
		&& Transaction->Batch.bAdvanceTurnAfterPlayback
		&& Transaction->Batch.PresentedTurnNumber > 0)
	{
		FWacomBattleCombatActivityEmission& Emission = Emissions.AddDefaulted_GetRef();
		Emission.Kind = EWacomBattleCombatActivityEmissionKind::SetTurn;
		Emission.TransactionId = TransactionId;
		Emission.TurnNumber = Transaction->Batch.PresentedTurnNumber;
	}

	if (!Transaction->Groups.IsEmpty())
	{
		FWacomBattleCombatActivityEmission& Completion = Emissions.AddDefaulted_GetRef();
		Completion.Kind = EWacomBattleCombatActivityEmissionKind::CompleteTransaction;
		Completion.TransactionId = TransactionId;
	}

	PendingTransactions.Remove(TransactionId);
	return Emissions;
}

void FWacomBattleCombatActivitySynchronizer::Discard(const uint64 TransactionId)
{
	PendingTransactions.Remove(TransactionId);
}

void FWacomBattleCombatActivitySynchronizer::Clear()
{
	PendingTransactions.Reset();
}

bool FWacomBattleCombatActivitySynchronizer::HasPendingTransaction(
	const uint64 TransactionId) const
{
	return PendingTransactions.Contains(TransactionId);
}

bool FWacomBattleCombatActivitySynchronizer::IsEnemyGroup(const FPendingGroup& Group)
{
	return Group.Group.RootAction.SourceEventType == EBattleEventType::EnemyPartActed;
}

bool FWacomBattleCombatActivitySynchronizer::MatchesSequenceRange(
	const int32 EventSequence,
	const int32 FirstEventSequence,
	const int32 LastEventSequence)
{
	if (EventSequence <= 0 || FirstEventSequence <= 0)
	{
		return false;
	}
	const int32 SafeLastEventSequence = LastEventSequence >= FirstEventSequence
		? LastEventSequence
		: FirstEventSequence;
	return EventSequence >= FirstEventSequence && EventSequence <= SafeLastEventSequence;
}

void FWacomBattleCombatActivitySynchronizer::EmitRootIfNeeded(
	const uint64 TransactionId,
	FPendingGroup& Group,
	TArray<FWacomBattleCombatActivityEmission>& OutEmissions)
{
	if (Group.bRootReleased)
	{
		return;
	}
	FWacomBattleCombatActivityEmission& Emission = OutEmissions.AddDefaulted_GetRef();
	Emission.Kind = EWacomBattleCombatActivityEmissionKind::BeginGroup;
	Emission.TransactionId = TransactionId;
	Emission.GroupIndex = Group.GroupIndex;
	Emission.RootAction = Group.Group.RootAction;
	Emission.TurnNumber = Group.Group.TurnNumber;
	Group.bRootReleased = true;
}

void FWacomBattleCombatActivitySynchronizer::EmitMatchingResults(
	const uint64 TransactionId,
	FPendingGroup& Group,
	TFunctionRef<bool(int32)> Predicate,
	TArray<FWacomBattleCombatActivityEmission>& OutEmissions)
{
	TArray<FWacomBattleCombatActivityRowView> ReleasedRows;
	for (int32 Index = 0; Index < Group.Group.ResultRows.Num(); ++Index)
	{
		if (Group.ReleasedResultIndices.Contains(Index))
		{
			continue;
		}
		const FWacomBattleCombatActivityRowView& Row = Group.Group.ResultRows[Index];
		if (!Predicate(Row.EventSequence))
		{
			continue;
		}
		Group.ReleasedResultIndices.Add(Index);
		ReleasedRows.Add(Row);
	}
	if (ReleasedRows.IsEmpty())
	{
		return;
	}
	EmitRootIfNeeded(TransactionId, Group, OutEmissions);
	FWacomBattleCombatActivityEmission& Emission = OutEmissions.AddDefaulted_GetRef();
	Emission.Kind = EWacomBattleCombatActivityEmissionKind::AppendResults;
	Emission.TransactionId = TransactionId;
	Emission.GroupIndex = Group.GroupIndex;
	Emission.ResultRows = MoveTemp(ReleasedRows);
}

bool FWacomBattleCombatActivitySynchronizer::HasUnreleasedContent(
	const FPendingTransaction& Transaction)
{
	for (const FPendingGroup& Group : Transaction.Groups)
	{
		if (!Group.bRootReleased
			|| Group.ReleasedResultIndices.Num() < Group.Group.ResultRows.Num())
		{
			return true;
		}
	}
	return !Transaction.bTurnReleased
		&& Transaction.Batch.bAdvanceTurnAfterPlayback
		&& Transaction.Batch.PresentedTurnNumber > 0;
}
