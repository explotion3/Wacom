// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "WacomBattlePresentationProgress.h"

enum class EWacomBattleCombatActivityEmissionKind : uint8
{
	BeginGroup,
	AppendResults,
	SetTurn,
	CompleteTransaction,
};

/** Incremental UI-only output produced from staged Combat Activity and presentation progress. */
struct FWacomBattleCombatActivityEmission
{
	EWacomBattleCombatActivityEmissionKind Kind =
		EWacomBattleCombatActivityEmissionKind::BeginGroup;
	uint64 TransactionId = 0;
	int32 GroupIndex = INDEX_NONE;
	FWacomBattleCombatActivityRowView RootAction;
	TArray<FWacomBattleCombatActivityRowView> ResultRows;
	int32 TurnNumber = 0;
};

/**
 * Holds resolved Combat Activity until the presentation coordinator reaches
 * the matching semantic boundary. It owns no widgets and does not tick.
 */
class WACOMAPP_API FWacomBattleCombatActivitySynchronizer
{
public:
	uint64 Stage(const FWacomBattleCombatActivityBatchView& Batch);
	TArray<FWacomBattleCombatActivityEmission> ApplyProgress(
		const FWacomBattlePresentationProgress& Progress,
		bool& bOutFlushedRemainder);
	TArray<FWacomBattleCombatActivityEmission> Flush(
		uint64 TransactionId,
		bool& bOutFlushedRemainder);
	void Discard(uint64 TransactionId);
	void Clear();
	bool HasPendingTransaction(uint64 TransactionId) const;

private:
	struct FPendingGroup
	{
		FWacomBattleCombatActivityGroupView Group;
		TSet<int32> ReleasedResultIndices;
		int32 GroupIndex = INDEX_NONE;
		bool bRootReleased = false;
	};

	struct FPendingTransaction
	{
		FWacomBattleCombatActivityBatchView Batch;
		TArray<FPendingGroup> Groups;
		bool bTurnReleased = false;
	};

	TMap<uint64, FPendingTransaction> PendingTransactions;
	uint64 NextTransactionId = 1;

	static bool IsEnemyGroup(const FPendingGroup& Group);
	static bool MatchesSequenceRange(
		int32 EventSequence,
		int32 FirstEventSequence,
		int32 LastEventSequence);
	static void EmitRootIfNeeded(
		uint64 TransactionId,
		FPendingGroup& Group,
		TArray<FWacomBattleCombatActivityEmission>& OutEmissions);
	static void EmitMatchingResults(
		uint64 TransactionId,
		FPendingGroup& Group,
		TFunctionRef<bool(int32)> Predicate,
		TArray<FWacomBattleCombatActivityEmission>& OutEmissions);
	static bool HasUnreleasedContent(const FPendingTransaction& Transaction);
};
