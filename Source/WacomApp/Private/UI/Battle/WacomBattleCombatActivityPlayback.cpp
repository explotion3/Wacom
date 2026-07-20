// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatActivityPlayback.h"

namespace
{
	constexpr int32 MaxResidentRows = 32;
	constexpr int32 MaxEmissionsPerTick = 8;

	float ResolveTopProximity(
		const float RowCenterY,
		const FWacomBattleCombatActivityPlaybackConfig& Config)
	{
		return 1.0f - FMath::Clamp(RowCenterY / Config.TopFadeBandPixels, 0.0f, 1.0f);
	}
}

void FWacomBattleCombatActivityPlaybackConfig::Normalize()
{
	EnterSeconds = FMath::Max(0.0f, EnterSeconds);
	ResultStaggerSeconds = FMath::Max(0.0f, ResultStaggerSeconds);
	MinimumResultStaggerSeconds = FMath::Clamp(
		MinimumResultStaggerSeconds, 0.0f, ResultStaggerSeconds);
	BurstStaggerThreshold = FMath::Max(1, BurstStaggerThreshold);
	BurstStaggerFullCompressionCount = FMath::Max(
		BurstStaggerThreshold + 1, BurstStaggerFullCompressionCount);
	MinimumReadableSeconds = FMath::Max(0.0f, MinimumReadableSeconds);
	ShiftSeconds = FMath::Max(0.0f, ShiftSeconds);
	BottomRowHoldSeconds = FMath::Max(0.0f, BottomRowHoldSeconds);
	BottomRowFadeSeconds = FMath::Max(0.0f, BottomRowFadeSeconds);
	TopRowHoldSeconds = FMath::Clamp(TopRowHoldSeconds, 0.0f, BottomRowHoldSeconds);
	TopRowFadeSeconds = FMath::Clamp(TopRowFadeSeconds, 0.0f, BottomRowFadeSeconds);
	ActivityViewportHeightPixels = FMath::Max(1.0f, ActivityViewportHeightPixels);
	RowHeightPixels = FMath::Clamp(RowHeightPixels, 1.0f, ActivityViewportHeightPixels);
	TopFadeBandPixels = FMath::Clamp(
		TopFadeBandPixels, 1.0f, ActivityViewportHeightPixels);
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

	AdvanceRows(SafeDelta, Config);

	if (!ActiveBatch.IsSet())
	{
		StartNextBatch(Config);
	}

	if (ActiveBatch.IsSet() && ActiveGroupIndex != INDEX_NONE)
	{
		const FWacomBattleCombatActivityGroupView& Group = ActiveBatch->Groups[ActiveGroupIndex];
		TimeSinceLastEmission += SafeDelta;
		int32 EmittedThisTick = 0;
		while (NextResultRowIndex >= 0
			&& NextResultRowIndex < Group.ResultRows.Num()
			&& EmittedThisTick < MaxEmissionsPerTick)
		{
			const int32 RemainingResultCount = Group.ResultRows.Num() - NextResultRowIndex;
			const float Stagger = Config.bReducedMotion
				? 0.0f
				: ResolveResultStaggerSeconds(RemainingResultCount, Config);
			if (TimeSinceLastEmission + KINDA_SMALL_NUMBER < Stagger)
			{
				break;
			}
			TimeSinceLastEmission = FMath::Max(0.0f, TimeSinceLastEmission - Stagger);
			EmitRow(Group.ResultRows[NextResultRowIndex++], false, Config);
			++EmittedThisTick;
		}

		if (!Group.ResultRows.IsEmpty()
			&& NextResultRowIndex >= Group.ResultRows.Num()
			&& ActiveRootPlaybackId != 0)
		{
			ReleaseActiveRoot(Config);
		}

		if (NextResultRowIndex >= Group.ResultRows.Num()
			&& TimeSinceLastEmission >= Config.MinimumReadableSeconds)
		{
			AdvanceAfterCurrentGroup(Config);
		}
	}

	RetargetRows(Config);
	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::Reset()
{
	PendingBatches.Reset();
	ActiveBatch.Reset();
	ActiveGroupIndex = INDEX_NONE;
	NextResultRowIndex = INDEX_NONE;
	TimeSinceLastEmission = 0.0f;
	VisibleRows.Reset();
	VisibleRowViews.Reset();
	LastRootAction.Reset();
	ActiveRootPlaybackId = 0;
	LastRootPlaybackId = 0;
	NextPlaybackId = 1;
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
	ReleasePreviousRootLane();
	const FWacomBattleCombatActivityGroupView& Group = ActiveBatch->Groups[ActiveGroupIndex];
	LastRootAction = Group.RootAction;
	if (Group.TurnNumber > 0 && PresentedTurnNumber <= 0)
	{
		PresentedTurnNumber = Group.TurnNumber;
	}
	ActiveRootPlaybackId = EmitRow(Group.RootAction, true, Config);
	LastRootPlaybackId = ActiveRootPlaybackId;
	NextResultRowIndex = 0;
	TimeSinceLastEmission = 0.0f;
}

void FWacomBattleCombatActivityPlayback::ReleasePreviousRootLane()
{
	for (FVisibleRow& Row : VisibleRows)
	{
		if (Row.bExitingRoot)
		{
			Row.bExitingRoot = false;
		}
	}
}

void FWacomBattleCombatActivityPlayback::AdvanceAfterCurrentGroup(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (!ActiveBatch.IsSet())
	{
		return;
	}
	ReleaseActiveRoot(Config);
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

uint64 FWacomBattleCombatActivityPlayback::EmitRow(
	const FWacomBattleCombatActivityRowView& Row,
	const bool bPinnedRoot,
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	FVisibleRow& Added = VisibleRows.AddDefaulted_GetRef();
	Added.PlaybackId = NextPlaybackId++;
	if (Added.PlaybackId == 0)
	{
		Added.PlaybackId = NextPlaybackId++;
	}
	Added.Row = Row;
	Added.EnterElapsed = Config.bReducedMotion ? Config.EnterSeconds : 0.0f;
	Added.bRetirementProtected = bPinnedRoot;
	Added.bPinnedRoot = bPinnedRoot;
	const float RootActionLaneY = Config.ActivityViewportHeightPixels - Config.RowHeightPixels;
	Added.CurrentY = RootActionLaneY;
	Added.ShiftStartY = Added.CurrentY;
	Added.TargetY = Added.CurrentY;
	Added.ShiftElapsed = Config.ShiftSeconds;
	const uint64 AddedPlaybackId = Added.PlaybackId;
	RetargetRows(Config);

	while (VisibleRows.Num() > MaxResidentRows)
	{
		int32 RemovalIndex = INDEX_NONE;
		for (int32 Index = 0; Index < VisibleRows.Num(); ++Index)
		{
			if (!VisibleRows[Index].bRetirementProtected
				&& VisibleRows[Index].CurrentY + Config.RowHeightPixels <= 0.0f)
			{
				RemovalIndex = Index;
				break;
			}
		}
		if (RemovalIndex == INDEX_NONE)
		{
			RemovalIndex = VisibleRows.IndexOfByPredicate([](const FVisibleRow& Row)
			{
				return !Row.bRetirementProtected;
			});
		}
		if (RemovalIndex == INDEX_NONE)
		{
			break;
		}
		if (VisibleRows[RemovalIndex].PlaybackId == ActiveRootPlaybackId)
		{
			ActiveRootPlaybackId = 0;
		}
		if (VisibleRows[RemovalIndex].PlaybackId == LastRootPlaybackId)
		{
			LastRootPlaybackId = 0;
		}
		VisibleRows.RemoveAt(RemovalIndex);
	}
	return AddedPlaybackId;
}

void FWacomBattleCombatActivityPlayback::ReleaseActiveRoot(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (ActiveRootPlaybackId == 0)
	{
		return;
	}
	for (FVisibleRow& Row : VisibleRows)
	{
		if (Row.PlaybackId != ActiveRootPlaybackId)
		{
			continue;
		}
		Row.bRetirementProtected = false;
		Row.bPinnedRoot = false;
		Row.bExitingRoot = true;
		Row.UnprotectedElapsed = 0.0f;
		break;
	}
	ActiveRootPlaybackId = 0;
	RetargetRows(Config);
}

void FWacomBattleCombatActivityPlayback::RetargetRows(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	const float RootActionLaneY = Config.ActivityViewportHeightPixels - Config.RowHeightPixels;
	int32 ResultDepth = 0;
	for (int32 Index = VisibleRows.Num() - 1; Index >= 0; --Index)
	{
		FVisibleRow& Row = VisibleRows[Index];
		float NewTargetY = 0.0f;
		if (Row.bPinnedRoot || Row.bExitingRoot)
		{
			NewTargetY = RootActionLaneY;
		}
		else
		{
			NewTargetY = RootActionLaneY
				- Config.RowHeightPixels * static_cast<float>(++ResultDepth);
		}

		if (FMath::IsNearlyEqual(Row.TargetY, NewTargetY, KINDA_SMALL_NUMBER))
		{
			continue;
		}
		Row.ShiftStartY = Row.CurrentY;
		Row.TargetY = NewTargetY;
		Row.ShiftElapsed = Config.bReducedMotion ? Config.ShiftSeconds : 0.0f;
		if (Config.bReducedMotion || Config.ShiftSeconds <= 0.0f)
		{
			Row.CurrentY = NewTargetY;
		}
	}
}

void FWacomBattleCombatActivityPlayback::AdvanceRows(
	const float DeltaTime,
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	for (FVisibleRow& Row : VisibleRows)
	{
		Row.EnterElapsed += DeltaTime;
		Row.ShiftElapsed += DeltaTime;
		const float ShiftAmount = Config.ShiftSeconds <= 0.0f
			? 1.0f
			: FMath::Clamp(Row.ShiftElapsed / Config.ShiftSeconds, 0.0f, 1.0f);
		const float SmoothedShift = FMath::InterpEaseOut(0.0f, 1.0f, ShiftAmount, 2.0f);
		Row.CurrentY = Config.bReducedMotion
			? Row.TargetY
			: FMath::Lerp(Row.ShiftStartY, Row.TargetY, SmoothedShift);

		if (Row.bRetirementProtected)
		{
			continue;
		}
		Row.UnprotectedElapsed += DeltaTime;
		const float RowCenterY = Row.CurrentY + Config.RowHeightPixels * 0.5f;
		const float TopProximity = ResolveTopProximity(RowCenterY, Config);
		const float HoldSeconds = FMath::Lerp(
			Config.BottomRowHoldSeconds, Config.TopRowHoldSeconds, TopProximity);
		if (Row.UnprotectedElapsed + KINDA_SMALL_NUMBER < HoldSeconds)
		{
			continue;
		}
		const float FadeSeconds = FMath::Lerp(
			Config.BottomRowFadeSeconds, Config.TopRowFadeSeconds, TopProximity);
		Row.RetirementProgress = FadeSeconds <= 0.0f
			? 1.0f
			: FMath::Min(1.0f, Row.RetirementProgress + DeltaTime / FadeSeconds);
	}

	for (int32 Index = VisibleRows.Num() - 1; Index >= 0; --Index)
	{
		const FVisibleRow& Row = VisibleRows[Index];
		const bool bOutsideViewport = Row.CurrentY + Config.RowHeightPixels <= 0.0f;
		if (Row.RetirementProgress >= 1.0f || (bOutsideViewport && !Row.bRetirementProtected))
		{
			if (Row.PlaybackId == ActiveRootPlaybackId)
			{
				ActiveRootPlaybackId = 0;
			}
			if (Row.PlaybackId == LastRootPlaybackId)
			{
				LastRootPlaybackId = 0;
			}
			VisibleRows.RemoveAt(Index);
		}
	}
}

float FWacomBattleCombatActivityPlayback::ResolveResultStaggerSeconds(
	const int32 RemainingResultCount,
	const FWacomBattleCombatActivityPlaybackConfig& Config) const
{
	if (RemainingResultCount <= Config.BurstStaggerThreshold)
	{
		return Config.ResultStaggerSeconds;
	}
	const float CompressionAmount = FMath::Clamp(
		static_cast<float>(RemainingResultCount - Config.BurstStaggerThreshold)
			/ static_cast<float>(Config.BurstStaggerFullCompressionCount - Config.BurstStaggerThreshold),
		0.0f,
		1.0f);
	return FMath::Lerp(
		Config.ResultStaggerSeconds, Config.MinimumResultStaggerSeconds, CompressionAmount);
}

void FWacomBattleCombatActivityPlayback::RebuildViews(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	VisibleRowViews.Reset(VisibleRows.Num());
	for (const FVisibleRow& VisibleRow : VisibleRows)
	{
		FWacomBattleCombatActivityRowPlaybackView& View = VisibleRowViews.AddDefaulted_GetRef();
		View.PlaybackId = VisibleRow.PlaybackId;
		View.Row = VisibleRow.Row;
		View.LayoutY = VisibleRow.CurrentY;
		View.bPinnedRoot = VisibleRow.bPinnedRoot;
		View.bRootActionLane = VisibleRow.bPinnedRoot || VisibleRow.bExitingRoot;
		View.bFooterHandoffSource = VisibleRow.PlaybackId == LastRootPlaybackId;
		const float EnterAmount = Config.EnterSeconds <= 0.0f
			? 1.0f
			: FMath::Clamp(VisibleRow.EnterElapsed / Config.EnterSeconds, 0.0f, 1.0f);
		if (VisibleRow.bExitingRoot)
		{
			View.Opacity = EnterAmount;
			View.ContentOpacity = 1.0f - VisibleRow.RetirementProgress;
			View.IconOpacity = 1.0f;
		}
		else
		{
			View.Opacity = EnterAmount * (1.0f - VisibleRow.RetirementProgress);
			View.ContentOpacity = 1.0f;
			View.IconOpacity = 1.0f;
		}
		View.TranslationY = Config.bReducedMotion
			? 0.0f
			: (1.0f - EnterAmount) * Config.RowHeightPixels;
	}
}
