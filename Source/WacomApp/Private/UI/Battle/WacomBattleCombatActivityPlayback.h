// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"

struct FWacomBattleCombatActivityPlaybackConfig
{
	float EnterSeconds = 0.12f;
	float ResultStaggerSeconds = 0.16f;
	float MinimumResultStaggerSeconds = 0.08f;
	int32 BurstStaggerThreshold = 6;
	int32 BurstStaggerFullCompressionCount = 12;
	float MinimumReadableSeconds = 0.85f;
	float MinimumResultVisibleSeconds = 0.35f;
	float ShiftSeconds = 0.10f;
	float BottomRowHoldSeconds = 0.85f;
	float BottomRowFadeSeconds = 0.24f;
	float TopRowHoldSeconds = 0.18f;
	float TopRowFadeSeconds = 0.10f;
	float RootIconReplacementFadeSeconds = 0.10f;
	float ActivityViewportHeightPixels = 140.0f;
	float RowHeightPixels = 40.0f;
	float TopFadeBandPixels = 72.0f;
	bool bReducedMotion = false;

	void Normalize();
};

struct FWacomBattleCombatActivityRowPlaybackView
{
	uint64 PlaybackId = 0;
	FWacomBattleCombatActivityRowView Row;
	float Opacity = 1.0f;
	float ContentOpacity = 1.0f;
	float IconOpacity = 1.0f;
	float LayoutY = 0.0f;
	float TranslationY = 0.0f;
	bool bPinnedRoot = false;
	bool bRootActionLane = false;
	bool bLatestRootAction = false;
	bool bResidentLastActionIcon = false;
	bool bReplacingLastActionIcon = false;
};

enum class EWacomBattleCombatActivityRootVisualState : uint8
{
	None,
	StreamedRoot,
	Pinned,
	ContentRetiring,
	IconResident,
	Replacing,
};

/** App-private FIFO playback for the non-blocking BattleHUD combat activity broadcaster. */
class WACOMAPP_API FWacomBattleCombatActivityPlayback
{
public:
	void Enqueue(const FWacomBattleCombatActivityBatchView& Batch);
	void BeginSynchronizedGroup(
		uint64 TransactionId,
		int32 GroupIndex,
		const FWacomBattleCombatActivityRowView& RootAction,
		int32 TurnNumber,
		const FWacomBattleCombatActivityPlaybackConfig& InConfig);
	void AppendSynchronizedResults(
		uint64 TransactionId,
		int32 GroupIndex,
		const TArray<FWacomBattleCombatActivityRowView>& ResultRows,
		const FWacomBattleCombatActivityPlaybackConfig& InConfig);
	void CompleteSynchronizedTransaction(
		uint64 TransactionId,
		const FWacomBattleCombatActivityPlaybackConfig& InConfig);
	void SetPresentedTurnNumber(int32 TurnNumber);
	void RestoreLastRootAction(
		const FWacomBattleCombatActivityRowView& RootAction,
		const FWacomBattleCombatActivityPlaybackConfig& InConfig);
	void Tick(float DeltaTime, const FWacomBattleCombatActivityPlaybackConfig& InConfig);
	void Reset();

	const TArray<FWacomBattleCombatActivityRowPlaybackView>& GetVisibleRows() const { return VisibleRowViews; }
	const FWacomBattleCombatActivityRowView* GetLastRootAction() const;
	int32 GetPresentedTurnNumber() const { return PresentedTurnNumber; }
	bool HasPendingPlayback() const;
	bool IsTickRequired() const;

private:
	struct FGroupKey
	{
		uint64 TransactionId = 0;
		int32 GroupIndex = INDEX_NONE;
		bool bLegacy = false;

		bool operator==(const FGroupKey& Other) const
		{
			return TransactionId == Other.TransactionId
				&& GroupIndex == Other.GroupIndex
				&& bLegacy == Other.bLegacy;
		}
	};

	struct FQueuedGroup
	{
		FGroupKey Key;
		uint64 RootPlaybackId = 0;
		int32 PendingResultCount = 0;
		bool bCompleted = false;
		bool bRootReleased = false;
		bool bHadResults = false;
	};

	struct FQueuedResult
	{
		FGroupKey GroupKey;
		FWacomBattleCombatActivityRowView Row;
	};

	struct FVisibleRow
	{
		uint64 PlaybackId = 0;
		FWacomBattleCombatActivityRowView Row;
		float EnterElapsed = 0.0f;
		float UnprotectedElapsed = 0.0f;
		float PresentedElapsed = 0.0f;
		float ContentRetirementProgress = 0.0f;
		float IconRetirementProgress = 0.0f;
		float CurrentY = 0.0f;
		float ShiftStartY = 0.0f;
		float TargetY = 0.0f;
		float ShiftElapsed = 0.0f;
		bool bResultRetiring = false;
		EWacomBattleCombatActivityRootVisualState RootVisualState =
			EWacomBattleCombatActivityRootVisualState::None;
	};

	TArray<FWacomBattleCombatActivityBatchView> PendingBatches;
	TOptional<FWacomBattleCombatActivityBatchView> ActiveBatch;
	TArray<FQueuedGroup> QueuedGroups;
	TArray<FQueuedResult> PendingResults;
	TOptional<FGroupKey> ActiveLegacyGroupKey;
	int32 ActiveGroupIndex = INDEX_NONE;
	float TimeSinceLastResultAdmission = 0.0f;
	float LegacyGroupDrainedElapsed = 0.0f;
	bool bResultAdmittedSinceLastTick = false;
	TArray<FVisibleRow> VisibleRows;
	TArray<FWacomBattleCombatActivityRowPlaybackView> VisibleRowViews;
	TOptional<FWacomBattleCombatActivityRowView> LastRootAction;
	uint64 ActiveRootPlaybackId = 0;
	uint64 LastRootPlaybackId = 0;
	uint64 NextPlaybackId = 1;
	uint64 NextLegacyTransactionId = 1;
	int32 PresentedTurnNumber = 0;

	void StartNextBatch(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void StartCurrentGroup(const FWacomBattleCombatActivityPlaybackConfig& Config);
	FQueuedGroup* BeginGroup(
		const FGroupKey& Key,
		const FWacomBattleCombatActivityRowView& RootAction,
		int32 TurnNumber,
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	FQueuedGroup* FindGroup(const FGroupKey& Key);
	const FQueuedGroup* FindGroup(const FGroupKey& Key) const;
	void QueueResults(const FGroupKey& Key,
		const TArray<FWacomBattleCombatActivityRowView>& Rows);
	void CancelLegacyScheduling();
	void PreparePreviousRootForReplacement(
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	void AdvanceAfterCurrentGroup(const FWacomBattleCombatActivityPlaybackConfig& Config);
	bool TryAdmitNextResult(
		const FWacomBattleCombatActivityPlaybackConfig& Config,
		bool bIgnoreStagger);
	bool CanAdmitResult(const FWacomBattleCombatActivityPlaybackConfig& Config) const;
	int32 ResolveVisibleResultCapacity(
		const FWacomBattleCombatActivityPlaybackConfig& Config) const;
	int32 CountVisibleResults() const;
	bool AreVisibleResultsShifting(
		const FWacomBattleCombatActivityPlaybackConfig& Config) const;
	uint64 EmitRow(
		const FWacomBattleCombatActivityRowView& Row,
		EWacomBattleCombatActivityRootVisualState RootVisualState,
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	void ReleaseRoot(uint64 RootPlaybackId,
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	void UpdateCompletedGroupRoots(
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	void StartOldestResultRetirement(
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	void RetargetRows(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void AdvanceRows(float DeltaTime, const FWacomBattleCombatActivityPlaybackConfig& Config);
	float ResolveResultStaggerSeconds(int32 RemainingResultCount,
		const FWacomBattleCombatActivityPlaybackConfig& Config) const;
	void RebuildViews(const FWacomBattleCombatActivityPlaybackConfig& Config);
};
