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
	float ShiftSeconds = 0.10f;
	float BottomRowHoldSeconds = 0.85f;
	float BottomRowFadeSeconds = 0.24f;
	float TopRowHoldSeconds = 0.18f;
	float TopRowFadeSeconds = 0.10f;
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
	bool bFooterHandoffSource = false;
};

/** App-private FIFO playback for the non-blocking BattleHUD combat activity broadcaster. */
class WACOMAPP_API FWacomBattleCombatActivityPlayback
{
public:
	void Enqueue(const FWacomBattleCombatActivityBatchView& Batch);
	void SetPresentedTurnNumber(int32 TurnNumber);
	void SetLastRootAction(const FWacomBattleCombatActivityRowView& RootAction);
	void Tick(float DeltaTime, const FWacomBattleCombatActivityPlaybackConfig& InConfig);
	void Reset();

	const TArray<FWacomBattleCombatActivityRowPlaybackView>& GetVisibleRows() const { return VisibleRowViews; }
	const FWacomBattleCombatActivityRowView* GetLastRootAction() const;
	int32 GetPresentedTurnNumber() const { return PresentedTurnNumber; }
	bool HasPendingPlayback() const;
	bool IsTickRequired() const;

private:
	struct FVisibleRow
	{
		uint64 PlaybackId = 0;
		FWacomBattleCombatActivityRowView Row;
		float EnterElapsed = 0.0f;
		float UnprotectedElapsed = 0.0f;
		float RetirementProgress = 0.0f;
		float CurrentY = 0.0f;
		float ShiftStartY = 0.0f;
		float TargetY = 0.0f;
		float ShiftElapsed = 0.0f;
		bool bRetirementProtected = false;
		bool bPinnedRoot = false;
		bool bExitingRoot = false;
	};

	TArray<FWacomBattleCombatActivityBatchView> PendingBatches;
	TOptional<FWacomBattleCombatActivityBatchView> ActiveBatch;
	int32 ActiveGroupIndex = INDEX_NONE;
	int32 NextResultRowIndex = INDEX_NONE;
	float TimeSinceLastEmission = 0.0f;
	TArray<FVisibleRow> VisibleRows;
	TArray<FWacomBattleCombatActivityRowPlaybackView> VisibleRowViews;
	TOptional<FWacomBattleCombatActivityRowView> LastRootAction;
	uint64 ActiveRootPlaybackId = 0;
	uint64 LastRootPlaybackId = 0;
	uint64 NextPlaybackId = 1;
	int32 PresentedTurnNumber = 0;

	void StartNextBatch(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void StartCurrentGroup(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void ReleasePreviousRootLane();
	void AdvanceAfterCurrentGroup(const FWacomBattleCombatActivityPlaybackConfig& Config);
	uint64 EmitRow(const FWacomBattleCombatActivityRowView& Row, bool bPinnedRoot,
		const FWacomBattleCombatActivityPlaybackConfig& Config);
	void ReleaseActiveRoot(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void RetargetRows(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void AdvanceRows(float DeltaTime, const FWacomBattleCombatActivityPlaybackConfig& Config);
	float ResolveResultStaggerSeconds(int32 RemainingResultCount,
		const FWacomBattleCombatActivityPlaybackConfig& Config) const;
	void RebuildViews(const FWacomBattleCombatActivityPlaybackConfig& Config);
};
