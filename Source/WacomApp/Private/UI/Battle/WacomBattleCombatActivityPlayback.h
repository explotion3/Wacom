// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"

struct FWacomBattleCombatActivityPlaybackConfig
{
	int32 MaxVisibleRows = 3;
	float EnterSeconds = 0.12f;
	float ResultStaggerSeconds = 0.16f;
	float MinimumReadableSeconds = 0.85f;
	float ShiftSeconds = 0.10f;
	float EmptyHoldSeconds = 0.90f;
	float CollapseSeconds = 0.18f;
	float RowShiftDistancePixels = 38.0f;
	bool bReducedMotion = false;

	void Normalize();
};

struct FWacomBattleCombatActivityRowPlaybackView
{
	FWacomBattleCombatActivityRowView Row;
	float Opacity = 1.0f;
	float TranslationY = 0.0f;
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
		FWacomBattleCombatActivityRowView Row;
		float EnterElapsed = 0.0f;
		float ShiftElapsed = 0.0f;
	};

	TArray<FWacomBattleCombatActivityBatchView> PendingBatches;
	TOptional<FWacomBattleCombatActivityBatchView> ActiveBatch;
	int32 ActiveGroupIndex = INDEX_NONE;
	int32 NextResultRowIndex = INDEX_NONE;
	float TimeSinceLastEmission = 0.0f;
	float EmptyElapsed = 0.0f;
	TArray<FVisibleRow> VisibleRows;
	TArray<FWacomBattleCombatActivityRowPlaybackView> VisibleRowViews;
	TOptional<FWacomBattleCombatActivityRowView> LastRootAction;
	int32 PresentedTurnNumber = 0;

	void StartNextBatch(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void StartCurrentGroup(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void AdvanceAfterCurrentGroup(const FWacomBattleCombatActivityPlaybackConfig& Config);
	void EmitRow(const FWacomBattleCombatActivityRowView& Row, const FWacomBattleCombatActivityPlaybackConfig& Config);
	void RebuildViews(const FWacomBattleCombatActivityPlaybackConfig& Config);
};
