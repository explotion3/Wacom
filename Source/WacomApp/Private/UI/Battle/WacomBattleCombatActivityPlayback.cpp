// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatActivityPlayback.h"

void FWacomBattleCombatActivityPlaybackConfig::Normalize()
{
	MaxVisibleRows = FMath::Max(1, MaxVisibleRows);
	EnterSeconds = FMath::Max(0.0f, EnterSeconds);
	ResultStaggerSeconds = FMath::Max(0.0f, ResultStaggerSeconds);
	MinimumReadableSeconds = FMath::Max(0.0f, MinimumReadableSeconds);
	ShiftSeconds = FMath::Max(0.0f, ShiftSeconds);
	EmptyHoldSeconds = FMath::Max(0.0f, EmptyHoldSeconds);
	CollapseSeconds = FMath::Max(0.0f, CollapseSeconds);
	RowShiftDistancePixels = FMath::Max(0.0f, RowShiftDistancePixels);
}

void FWacomBattleCombatActivityPlayback::Enqueue(const FWacomBattleCombatActivityBatchView& Batch)
{
	if (Batch.bSetTurnImmediately)
	{
		SetPresentedTurnNumber(Batch.PresentedTurnNumber);
	}
	if (Batch.Groups.IsEmpty())
	{
		if (Batch.bAdvanceTurnAfterPlayback)
		{
			SetPresentedTurnNumber(Batch.PresentedTurnNumber);
		}
		return;
	}
	PendingBatches.Add(Batch);
	EmptyElapsed = 0.0f;
}

void FWacomBattleCombatActivityPlayback::SetPresentedTurnNumber(int32 TurnNumber)
{
	PresentedTurnNumber = FMath::Max(0, TurnNumber);
}

void FWacomBattleCombatActivityPlayback::SetLastRootAction(
	const FWacomBattleCombatActivityRowView& RootAction)
{
	LastRootAction = RootAction;
}

void FWacomBattleCombatActivityPlayback::Tick(
	float DeltaTime,
	const FWacomBattleCombatActivityPlaybackConfig& InConfig)
{
	FWacomBattleCombatActivityPlaybackConfig Config = InConfig;
	Config.Normalize();
	const float SafeDelta = FMath::Max(0.0f, DeltaTime);
	for (FVisibleRow& VisibleRow : VisibleRows)
	{
		VisibleRow.EnterElapsed += SafeDelta;
		VisibleRow.ShiftElapsed += SafeDelta;
	}

	if (!ActiveBatch.IsSet())
	{
		StartNextBatch(Config);
	}

	if (ActiveBatch.IsSet() && ActiveGroupIndex != INDEX_NONE)
	{
		const FWacomBattleCombatActivityGroupView& Group = ActiveBatch->Groups[ActiveGroupIndex];
		TimeSinceLastEmission += SafeDelta;
		const float Stagger = Config.bReducedMotion ? 0.0f : Config.ResultStaggerSeconds;
		while (NextResultRowIndex >= 0
			&& NextResultRowIndex < Group.ResultRows.Num()
			&& TimeSinceLastEmission + KINDA_SMALL_NUMBER >= Stagger)
		{
			TimeSinceLastEmission = FMath::Max(0.0f, TimeSinceLastEmission - Stagger);
			EmitRow(Group.ResultRows[NextResultRowIndex++], Config);
			if (Stagger > 0.0f)
			{
				break;
			}
		}

		if (NextResultRowIndex >= Group.ResultRows.Num()
			&& TimeSinceLastEmission >= Config.MinimumReadableSeconds)
		{
			AdvanceAfterCurrentGroup(Config);
		}
	}

	if (!ActiveBatch.IsSet() && PendingBatches.IsEmpty())
	{
		EmptyElapsed += SafeDelta;
		const float CollapseEnd = Config.EmptyHoldSeconds + Config.CollapseSeconds;
		if (!VisibleRows.IsEmpty()
			&& (Config.CollapseSeconds <= 0.0f ? EmptyElapsed >= Config.EmptyHoldSeconds : EmptyElapsed >= CollapseEnd))
		{
			VisibleRows.Reset();
		}
	}
	else
	{
		EmptyElapsed = 0.0f;
	}

	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::Reset()
{
	PendingBatches.Reset();
	ActiveBatch.Reset();
	ActiveGroupIndex = INDEX_NONE;
	NextResultRowIndex = INDEX_NONE;
	TimeSinceLastEmission = 0.0f;
	EmptyElapsed = 0.0f;
	VisibleRows.Reset();
	VisibleRowViews.Reset();
	LastRootAction.Reset();
	PresentedTurnNumber = 0;
}

const FWacomBattleCombatActivityRowView* FWacomBattleCombatActivityPlayback::GetLastRootAction() const
{
	return LastRootAction.IsSet() ? &LastRootAction.GetValue() : nullptr;
}

bool FWacomBattleCombatActivityPlayback::HasPendingPlayback() const
{
	return ActiveBatch.IsSet() || !PendingBatches.IsEmpty();
}

bool FWacomBattleCombatActivityPlayback::IsTickRequired() const
{
	return HasPendingPlayback() || !VisibleRows.IsEmpty();
}

void FWacomBattleCombatActivityPlayback::StartNextBatch(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (PendingBatches.IsEmpty())
	{
		return;
	}
	ActiveBatch = MoveTemp(PendingBatches[0]);
	PendingBatches.RemoveAt(0);
	ActiveGroupIndex = 0;
	StartCurrentGroup(Config);
}

void FWacomBattleCombatActivityPlayback::StartCurrentGroup(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (!ActiveBatch.IsSet() || !ActiveBatch->Groups.IsValidIndex(ActiveGroupIndex))
	{
		return;
	}
	const FWacomBattleCombatActivityGroupView& Group = ActiveBatch->Groups[ActiveGroupIndex];
	LastRootAction = Group.RootAction;
	if (Group.TurnNumber > 0 && PresentedTurnNumber <= 0)
	{
		PresentedTurnNumber = Group.TurnNumber;
	}
	EmitRow(Group.RootAction, Config);
	NextResultRowIndex = 0;
	TimeSinceLastEmission = 0.0f;
}

void FWacomBattleCombatActivityPlayback::AdvanceAfterCurrentGroup(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (!ActiveBatch.IsSet())
	{
		return;
	}
	++ActiveGroupIndex;
	if (ActiveBatch->Groups.IsValidIndex(ActiveGroupIndex))
	{
		StartCurrentGroup(Config);
		return;
	}
	if (ActiveBatch->bAdvanceTurnAfterPlayback)
	{
		SetPresentedTurnNumber(ActiveBatch->PresentedTurnNumber);
	}
	ActiveBatch.Reset();
	ActiveGroupIndex = INDEX_NONE;
	NextResultRowIndex = INDEX_NONE;
	TimeSinceLastEmission = 0.0f;
	StartNextBatch(Config);
}

void FWacomBattleCombatActivityPlayback::EmitRow(
	const FWacomBattleCombatActivityRowView& Row,
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	const bool bWillPushRowsUp = VisibleRows.Num() >= Config.MaxVisibleRows;
	if (bWillPushRowsUp)
	{
		for (FVisibleRow& Existing : VisibleRows)
		{
			Existing.ShiftElapsed = 0.0f;
		}
	}
	FVisibleRow& Added = VisibleRows.AddDefaulted_GetRef();
	Added.Row = Row;
	Added.EnterElapsed = Config.bReducedMotion ? Config.EnterSeconds : 0.0f;
	Added.ShiftElapsed = Config.ShiftSeconds;
	if (VisibleRows.Num() > Config.MaxVisibleRows)
	{
		VisibleRows.RemoveAt(0, VisibleRows.Num() - Config.MaxVisibleRows);
	}
}

void FWacomBattleCombatActivityPlayback::RebuildViews(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	VisibleRowViews.Reset(VisibleRows.Num());
	float CollapseOpacity = 1.0f;
	if (!HasPendingPlayback() && EmptyElapsed > Config.EmptyHoldSeconds)
	{
		CollapseOpacity = Config.CollapseSeconds <= 0.0f
			? 0.0f
			: 1.0f - FMath::Clamp(
				(EmptyElapsed - Config.EmptyHoldSeconds) / Config.CollapseSeconds,
				0.0f,
				1.0f);
	}
	for (const FVisibleRow& VisibleRow : VisibleRows)
	{
		FWacomBattleCombatActivityRowPlaybackView& View = VisibleRowViews.AddDefaulted_GetRef();
		View.Row = VisibleRow.Row;
		const float EnterAmount = Config.EnterSeconds <= 0.0f
			? 1.0f
			: FMath::Clamp(VisibleRow.EnterElapsed / Config.EnterSeconds, 0.0f, 1.0f);
		const float ShiftAmount = Config.ShiftSeconds <= 0.0f
			? 1.0f
			: FMath::Clamp(VisibleRow.ShiftElapsed / Config.ShiftSeconds, 0.0f, 1.0f);
		View.Opacity = EnterAmount * CollapseOpacity;
		View.TranslationY = Config.bReducedMotion
			? 0.0f
			: (2.0f - EnterAmount - ShiftAmount) * Config.RowShiftDistancePixels;
	}
}
