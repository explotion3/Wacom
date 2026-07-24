// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBattleFloatingCombatTextTypes.h"
#include "WacomBattlePresentationProgress.h"

struct FBattleEvent;

/**
 * App-private event-sequence synchronizer for floating combat text.
 *
 * It consumes the same semantic presentation progress as Combat Activity but
 * owns an independent projection and exactly-once release state.
 */
class WACOMAPP_API FWacomBattleFloatingCombatTextSynchronizer
{
public:
	void Stage(uint64 TransactionId, const TArray<FBattleEvent>& Events);

	TArray<FWacomBattleFloatingCombatTextEmission> ApplyProgress(
		const FWacomBattlePresentationProgress& Progress,
		bool& bOutFlushedRemainder);

	void Discard(uint64 TransactionId);
	void Clear();

private:
	struct FPendingGroup
	{
		int32 EventSequence = INDEX_NONE;
		TArray<FWacomBattleFloatingCombatTextRow> Rows;
		bool bReleased = false;
	};

	struct FPendingTransaction
	{
		TArray<FPendingGroup> Groups;
	};

	TMap<uint64, FPendingTransaction> PendingTransactions;

	static TArray<FWacomBattleFloatingCombatTextRow> BuildRows(
		const FBattleEvent& Event);
	static bool MatchesRange(int32 Sequence, int32 First, int32 Last);

	TArray<FWacomBattleFloatingCombatTextEmission> ReleaseMatching(
		uint64 TransactionId,
		TFunctionRef<bool(int32)> Predicate);
	TArray<FWacomBattleFloatingCombatTextEmission> Flush(
		uint64 TransactionId,
		bool& bOutFlushedRemainder);
};
