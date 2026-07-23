// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatActivityPlayback.h"

namespace
{
	bool IsStreamLaneState(const EWacomBattleCombatActivityRootVisualState State)
	{
		return State == EWacomBattleCombatActivityRootVisualState::None
			|| State == EWacomBattleCombatActivityRootVisualState::StreamedRoot;
	}

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
	MinimumResultVisibleSeconds = FMath::Max(0.0f, MinimumResultVisibleSeconds);
	ShiftSeconds = FMath::Max(0.0f, ShiftSeconds);
	BottomRowHoldSeconds = FMath::Max(0.0f, BottomRowHoldSeconds);
	BottomRowFadeSeconds = FMath::Max(0.0f, BottomRowFadeSeconds);
	TopRowHoldSeconds = FMath::Clamp(TopRowHoldSeconds, 0.0f, BottomRowHoldSeconds);
	TopRowFadeSeconds = FMath::Clamp(TopRowFadeSeconds, 0.0f, BottomRowFadeSeconds);
	RootIconReplacementFadeSeconds = FMath::Max(0.0f, RootIconReplacementFadeSeconds);
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

void FWacomBattleCombatActivityPlayback::BeginSynchronizedGroup(
	const uint64 TransactionId,
	const int32 GroupIndex,
	const FWacomBattleCombatActivityRowView& RootAction,
	const int32 TurnNumber,
	const FWacomBattleCombatActivityPlaybackConfig& InConfig)
{
	FWacomBattleCombatActivityPlaybackConfig Config = InConfig;
	Config.Normalize();
	if (TransactionId == 0 || GroupIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleCombatActivity] Rejected synchronized root with invalid identity (%llu:%d)."),
			TransactionId,
			GroupIndex);
		return;
	}

	CancelLegacyScheduling();
	const FGroupKey Key{ TransactionId, GroupIndex, false };
	if (FindGroup(Key))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[BattleCombatActivity] Ignored duplicate synchronized root (%llu:%d)."),
			TransactionId,
			GroupIndex);
		return;
	}

	BeginGroup(Key, RootAction, TurnNumber, Config);
	RetargetRows(Config);
	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::AppendSynchronizedResults(
	const uint64 TransactionId,
	const int32 GroupIndex,
	const TArray<FWacomBattleCombatActivityRowView>& ResultRows,
	const FWacomBattleCombatActivityPlaybackConfig& InConfig)
{
	if (ResultRows.IsEmpty())
	{
		return;
	}
	FWacomBattleCombatActivityPlaybackConfig Config = InConfig;
	Config.Normalize();
	const FGroupKey Key{ TransactionId, GroupIndex, false };
	FQueuedGroup* Group = FindGroup(Key);
	if (!Group)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleCombatActivity] Rejected results for unknown group (%llu:%d)."),
			TransactionId,
			GroupIndex);
		return;
	}
	if (Group->bCompleted)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleCombatActivity] Rejected late results for completed group (%llu:%d)."),
			TransactionId,
			GroupIndex);
		return;
	}

	const bool bWasQueueEmpty = PendingResults.IsEmpty();
	QueueResults(Key, ResultRows);
	if (bWasQueueEmpty)
	{
		TryAdmitNextResult(Config, true);
	}
	RetargetRows(Config);
	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::CompleteSynchronizedTransaction(
	const uint64 TransactionId,
	const FWacomBattleCombatActivityPlaybackConfig& InConfig)
{
	FWacomBattleCombatActivityPlaybackConfig Config = InConfig;
	Config.Normalize();
	bool bFoundTransaction = false;
	for (FQueuedGroup& Group : QueuedGroups)
	{
		if (!Group.Key.bLegacy && Group.Key.TransactionId == TransactionId)
		{
			Group.bCompleted = true;
			bFoundTransaction = true;
		}
	}
	if (!bFoundTransaction)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[BattleCombatActivity] Ignored completion for unknown transaction %llu."),
			TransactionId);
	}
	UpdateCompletedGroupRoots(Config);
	RetargetRows(Config);
	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::SetPresentedTurnNumber(int32 TurnNumber)
{
	PresentedTurnNumber = FMath::Max(0, TurnNumber);
}

void FWacomBattleCombatActivityPlayback::RestoreLastRootAction(
	const FWacomBattleCombatActivityRowView& RootAction,
	const FWacomBattleCombatActivityPlaybackConfig& InConfig)
{
	FWacomBattleCombatActivityPlaybackConfig Config = InConfig;
	Config.Normalize();
	LastRootAction = RootAction;
	if (LastRootPlaybackId != 0)
	{
		for (FVisibleRow& Row : VisibleRows)
		{
			if (Row.PlaybackId == LastRootPlaybackId)
			{
				Row.Row = RootAction;
				RebuildViews(Config);
				return;
			}
		}
	}

	const uint64 RestoredPlaybackId = EmitRow(
		RootAction,
		EWacomBattleCombatActivityRootVisualState::IconResident,
		Config);
	LastRootPlaybackId = RestoredPlaybackId;
	RetargetRows(Config);
	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::Tick(
	float DeltaTime,
	const FWacomBattleCombatActivityPlaybackConfig& InConfig)
{
	FWacomBattleCombatActivityPlaybackConfig Config = InConfig;
	Config.Normalize();
	const float SafeDelta = FMath::Max(0.0f, DeltaTime);

	bResultAdmittedSinceLastTick = false;
	AdvanceRows(SafeDelta, Config);

	if (!ActiveBatch.IsSet())
	{
		StartNextBatch(Config);
	}

	if (!PendingResults.IsEmpty())
	{
		TimeSinceLastResultAdmission += SafeDelta;
	}
	const bool bAdmittedResult = TryAdmitNextResult(Config, false);
	UpdateCompletedGroupRoots(Config);

	if (ActiveBatch.IsSet() && ActiveLegacyGroupKey.IsSet())
	{
		const FQueuedGroup* LegacyGroup = FindGroup(ActiveLegacyGroupKey.GetValue());
		if (LegacyGroup && LegacyGroup->PendingResultCount <= 0)
		{
			if (!bAdmittedResult)
			{
				LegacyGroupDrainedElapsed += SafeDelta;
			}
			if (LegacyGroupDrainedElapsed + KINDA_SMALL_NUMBER
				>= Config.MinimumReadableSeconds)
			{
				AdvanceAfterCurrentGroup(Config);
			}
		}
		else
		{
			LegacyGroupDrainedElapsed = 0.0f;
		}
	}

	RetargetRows(Config);
	RebuildViews(Config);
}

void FWacomBattleCombatActivityPlayback::Reset()
{
	PendingBatches.Reset();
	ActiveBatch.Reset();
	QueuedGroups.Reset();
	PendingResults.Reset();
	ActiveLegacyGroupKey.Reset();
	ActiveGroupIndex = INDEX_NONE;
	TimeSinceLastResultAdmission = 0.0f;
	LegacyGroupDrainedElapsed = 0.0f;
	bResultAdmittedSinceLastTick = false;
	VisibleRows.Reset();
	VisibleRowViews.Reset();
	LastRootAction.Reset();
	ActiveRootPlaybackId = 0;
	LastRootPlaybackId = 0;
	NextPlaybackId = 1;
	NextLegacyTransactionId = 1;
	PresentedTurnNumber = 0;
}

const FWacomBattleCombatActivityRowView* FWacomBattleCombatActivityPlayback::GetLastRootAction() const
{
	return LastRootAction.IsSet() ? &LastRootAction.GetValue() : nullptr;
}

bool FWacomBattleCombatActivityPlayback::HasPendingPlayback() const
{
	return ActiveBatch.IsSet()
		|| !PendingBatches.IsEmpty()
		|| !PendingResults.IsEmpty();
}

bool FWacomBattleCombatActivityPlayback::IsTickRequired() const
{
	if (HasPendingPlayback())
	{
		return true;
	}
	return VisibleRows.ContainsByPredicate([](const FVisibleRow& Row)
	{
		return Row.RootVisualState
			!= EWacomBattleCombatActivityRootVisualState::IconResident;
	});
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
	uint64 LegacyTransactionId = NextLegacyTransactionId++;
	if (LegacyTransactionId == 0)
	{
		LegacyTransactionId = NextLegacyTransactionId++;
	}
	const FGroupKey Key{ LegacyTransactionId, ActiveGroupIndex, true };
	const FWacomBattleCombatActivityGroupView& Group = ActiveBatch->Groups[ActiveGroupIndex];
	FQueuedGroup* QueuedGroup = BeginGroup(
		Key,
		Group.RootAction,
		Group.TurnNumber,
		Config);
	if (!QueuedGroup)
	{
		return;
	}
	ActiveLegacyGroupKey = Key;
	QueueResults(Key, Group.ResultRows);
	QueuedGroup = FindGroup(Key);
	if (QueuedGroup)
	{
		QueuedGroup->bCompleted = true;
	}
	LegacyGroupDrainedElapsed = 0.0f;
	TimeSinceLastResultAdmission = 0.0f;
}

FWacomBattleCombatActivityPlayback::FQueuedGroup*
FWacomBattleCombatActivityPlayback::BeginGroup(
	const FGroupKey& Key,
	const FWacomBattleCombatActivityRowView& RootAction,
	const int32 TurnNumber,
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (FindGroup(Key))
	{
		return nullptr;
	}
	PreparePreviousRootForReplacement(Config);
	FQueuedGroup& Group = QueuedGroups.AddDefaulted_GetRef();
	Group.Key = Key;
	LastRootAction = RootAction;
	if (TurnNumber > 0 && PresentedTurnNumber <= 0)
	{
		PresentedTurnNumber = TurnNumber;
	}
	ActiveRootPlaybackId = EmitRow(
		RootAction,
		EWacomBattleCombatActivityRootVisualState::Pinned,
		Config);
	LastRootPlaybackId = ActiveRootPlaybackId;
	Group.RootPlaybackId = ActiveRootPlaybackId;
	return &Group;
}

FWacomBattleCombatActivityPlayback::FQueuedGroup*
FWacomBattleCombatActivityPlayback::FindGroup(const FGroupKey& Key)
{
	return QueuedGroups.FindByPredicate([&Key](const FQueuedGroup& Group)
	{
		return Group.Key == Key;
	});
}

const FWacomBattleCombatActivityPlayback::FQueuedGroup*
FWacomBattleCombatActivityPlayback::FindGroup(const FGroupKey& Key) const
{
	return QueuedGroups.FindByPredicate([&Key](const FQueuedGroup& Group)
	{
		return Group.Key == Key;
	});
}

void FWacomBattleCombatActivityPlayback::QueueResults(
	const FGroupKey& Key,
	const TArray<FWacomBattleCombatActivityRowView>& Rows)
{
	FQueuedGroup* Group = FindGroup(Key);
	if (!Group || Rows.IsEmpty())
	{
		return;
	}
	Group->bHadResults = true;
	Group->PendingResultCount += Rows.Num();
	for (const FWacomBattleCombatActivityRowView& Row : Rows)
	{
		FQueuedResult& Pending = PendingResults.AddDefaulted_GetRef();
		Pending.GroupKey = Key;
		Pending.Row = Row;
	}
}

void FWacomBattleCombatActivityPlayback::CancelLegacyScheduling()
{
	PendingBatches.Reset();
	ActiveBatch.Reset();
	ActiveGroupIndex = INDEX_NONE;
	ActiveLegacyGroupKey.Reset();
	LegacyGroupDrainedElapsed = 0.0f;
	PendingResults.RemoveAll([](const FQueuedResult& Pending)
	{
		return Pending.GroupKey.bLegacy;
	});
	QueuedGroups.RemoveAll([](const FQueuedGroup& Group)
	{
		return Group.Key.bLegacy;
	});
}

void FWacomBattleCombatActivityPlayback::PreparePreviousRootForReplacement(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	// A completed resident icon still uses the authored replacement crossfade.
	// A root whose content has not finished stays readable by joining the stream
	// above the new semantic root instead of being discarded by rapid actions.
	for (int32 Index = VisibleRows.Num() - 1; Index >= 0; --Index)
	{
		if (VisibleRows[Index].RootVisualState
			== EWacomBattleCombatActivityRootVisualState::Replacing)
		{
			VisibleRows.RemoveAt(Index);
		}
	}
	if (Config.RootIconReplacementFadeSeconds <= 0.0f)
	{
		for (int32 Index = VisibleRows.Num() - 1; Index >= 0; --Index)
		{
			FVisibleRow& Row = VisibleRows[Index];
			if (Row.RootVisualState == EWacomBattleCombatActivityRootVisualState::Pinned
				|| Row.RootVisualState
					== EWacomBattleCombatActivityRootVisualState::ContentRetiring)
			{
				Row.RootVisualState = EWacomBattleCombatActivityRootVisualState::StreamedRoot;
				Row.ContentRetirementProgress = 0.0f;
				Row.IconRetirementProgress = 0.0f;
				Row.PresentedElapsed = 0.0f;
				Row.UnprotectedElapsed = 0.0f;
				Row.bResultRetiring = false;
			}
			else if (Row.RootVisualState
				== EWacomBattleCombatActivityRootVisualState::IconResident)
			{
				VisibleRows.RemoveAt(Index);
			}
		}
		ActiveRootPlaybackId = 0;
		RetargetRows(Config);
		return;
	}

	for (FVisibleRow& Row : VisibleRows)
	{
		if (IsStreamLaneState(Row.RootVisualState))
		{
			continue;
		}
		if (Row.RootVisualState == EWacomBattleCombatActivityRootVisualState::Pinned
			|| Row.RootVisualState
				== EWacomBattleCombatActivityRootVisualState::ContentRetiring)
		{
			Row.RootVisualState = EWacomBattleCombatActivityRootVisualState::StreamedRoot;
			Row.ContentRetirementProgress = 0.0f;
			Row.IconRetirementProgress = 0.0f;
			Row.PresentedElapsed = 0.0f;
			Row.UnprotectedElapsed = 0.0f;
			Row.bResultRetiring = false;
		}
		else
		{
			Row.RootVisualState = EWacomBattleCombatActivityRootVisualState::Replacing;
			Row.UnprotectedElapsed = 0.0f;
			Row.IconRetirementProgress = 0.0f;
		}
	}
	ActiveRootPlaybackId = 0;
	RetargetRows(Config);
}

void FWacomBattleCombatActivityPlayback::AdvanceAfterCurrentGroup(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (!ActiveBatch.IsSet())
	{
		return;
	}
	if (ActiveLegacyGroupKey.IsSet())
	{
		if (FQueuedGroup* Group = FindGroup(ActiveLegacyGroupKey.GetValue()))
		{
			ReleaseRoot(Group->RootPlaybackId, Config);
			Group->bRootReleased = true;
		}
		const FGroupKey CompletedKey = ActiveLegacyGroupKey.GetValue();
		QueuedGroups.RemoveAll([&CompletedKey](const FQueuedGroup& Group)
		{
			return Group.Key == CompletedKey;
		});
		ActiveLegacyGroupKey.Reset();
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
	LegacyGroupDrainedElapsed = 0.0f;
	StartNextBatch(Config);
}

bool FWacomBattleCombatActivityPlayback::TryAdmitNextResult(
	const FWacomBattleCombatActivityPlaybackConfig& Config,
	const bool bIgnoreStagger)
{
	if (PendingResults.IsEmpty() || bResultAdmittedSinceLastTick
		|| !CanAdmitResult(Config))
	{
		return false;
	}
	const float Stagger = Config.bReducedMotion
		? 0.0f
		: ResolveResultStaggerSeconds(PendingResults.Num(), Config);
	if (!bIgnoreStagger
		&& TimeSinceLastResultAdmission + KINDA_SMALL_NUMBER < Stagger)
	{
		return false;
	}

	FQueuedResult Pending = MoveTemp(PendingResults[0]);
	PendingResults.RemoveAt(0);
	if (FQueuedGroup* Group = FindGroup(Pending.GroupKey))
	{
		Group->PendingResultCount = FMath::Max(0, Group->PendingResultCount - 1);
	}
	EmitRow(
		Pending.Row,
		EWacomBattleCombatActivityRootVisualState::None,
		Config);
	TimeSinceLastResultAdmission = bIgnoreStagger
		? 0.0f
		: FMath::Max(0.0f, TimeSinceLastResultAdmission - Stagger);
	bResultAdmittedSinceLastTick = true;
	if (ActiveLegacyGroupKey.IsSet() && Pending.GroupKey == ActiveLegacyGroupKey.GetValue())
	{
		LegacyGroupDrainedElapsed = 0.0f;
	}
	UpdateCompletedGroupRoots(Config);
	return true;
}

bool FWacomBattleCombatActivityPlayback::CanAdmitResult(
	const FWacomBattleCombatActivityPlaybackConfig& Config) const
{
	return CountVisibleResults() < ResolveVisibleResultCapacity(Config)
		&& !AreVisibleResultsShifting(Config);
}

int32 FWacomBattleCombatActivityPlayback::ResolveVisibleResultCapacity(
	const FWacomBattleCombatActivityPlaybackConfig& Config) const
{
	const float RootActionLaneY = Config.ActivityViewportHeightPixels - Config.RowHeightPixels;
	return FMath::Max(0, FMath::CeilToInt(RootActionLaneY / Config.RowHeightPixels));
}

int32 FWacomBattleCombatActivityPlayback::CountVisibleResults() const
{
	int32 Count = 0;
	for (const FVisibleRow& Row : VisibleRows)
	{
		if (IsStreamLaneState(Row.RootVisualState))
		{
			++Count;
		}
	}
	return Count;
}

bool FWacomBattleCombatActivityPlayback::AreVisibleResultsShifting(
	const FWacomBattleCombatActivityPlaybackConfig& Config) const
{
	if (Config.bReducedMotion || Config.ShiftSeconds <= 0.0f)
	{
		return false;
	}
	return VisibleRows.ContainsByPredicate([&Config](const FVisibleRow& Row)
	{
		return IsStreamLaneState(Row.RootVisualState)
			&& Row.ShiftElapsed + KINDA_SMALL_NUMBER < Config.ShiftSeconds
			&& !FMath::IsNearlyEqual(Row.CurrentY, Row.TargetY, KINDA_SMALL_NUMBER);
	});
}

uint64 FWacomBattleCombatActivityPlayback::EmitRow(
	const FWacomBattleCombatActivityRowView& Row,
	const EWacomBattleCombatActivityRootVisualState RootVisualState,
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
	Added.RootVisualState = RootVisualState;
	if (RootVisualState == EWacomBattleCombatActivityRootVisualState::IconResident)
	{
		Added.EnterElapsed = Config.EnterSeconds;
		Added.ContentRetirementProgress = 1.0f;
		Added.ShiftElapsed = Config.ShiftSeconds;
	}
	const float RootActionLaneY = Config.ActivityViewportHeightPixels - Config.RowHeightPixels;
	Added.CurrentY = RootActionLaneY;
	Added.ShiftStartY = Added.CurrentY;
	Added.TargetY = Added.CurrentY;
	Added.ShiftElapsed = Config.ShiftSeconds;
	const uint64 AddedPlaybackId = Added.PlaybackId;
	RetargetRows(Config);
	return AddedPlaybackId;
}

void FWacomBattleCombatActivityPlayback::ReleaseRoot(
	const uint64 RootPlaybackId,
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (RootPlaybackId == 0)
	{
		return;
	}
	for (FVisibleRow& Row : VisibleRows)
	{
		if (Row.PlaybackId != RootPlaybackId)
		{
			continue;
		}
		if (Row.RootVisualState != EWacomBattleCombatActivityRootVisualState::Pinned)
		{
			break;
		}
		Row.RootVisualState = EWacomBattleCombatActivityRootVisualState::ContentRetiring;
		Row.UnprotectedElapsed = 0.0f;
		break;
	}
	if (ActiveRootPlaybackId == RootPlaybackId)
	{
		ActiveRootPlaybackId = 0;
	}
	RetargetRows(Config);
}

void FWacomBattleCombatActivityPlayback::UpdateCompletedGroupRoots(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	for (FQueuedGroup& Group : QueuedGroups)
	{
		if (!Group.bCompleted || Group.PendingResultCount > 0 || Group.bRootReleased)
		{
			continue;
		}
		if (Group.Key.bLegacy && !Group.bHadResults)
		{
			continue;
		}
		ReleaseRoot(Group.RootPlaybackId, Config);
		Group.bRootReleased = true;
	}
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
		if (!IsStreamLaneState(Row.RootVisualState))
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

		if (IsStreamLaneState(Row.RootVisualState))
		{
			Row.PresentedElapsed += DeltaTime;
			Row.UnprotectedElapsed += DeltaTime;
			if (Row.bResultRetiring)
			{
				const float RowCenterY = Row.CurrentY + Config.RowHeightPixels * 0.5f;
				const float TopProximity = ResolveTopProximity(RowCenterY, Config);
				const float FadeSeconds = FMath::Lerp(
					Config.BottomRowFadeSeconds, Config.TopRowFadeSeconds, TopProximity);
				Row.ContentRetirementProgress = FadeSeconds <= 0.0f
					? 1.0f
					: FMath::Min(1.0f,
						Row.ContentRetirementProgress + DeltaTime / FadeSeconds);
			}
			continue;
		}

		if (Row.RootVisualState == EWacomBattleCombatActivityRootVisualState::Pinned
			|| Row.RootVisualState == EWacomBattleCombatActivityRootVisualState::IconResident)
		{
			continue;
		}
		if (Row.RootVisualState == EWacomBattleCombatActivityRootVisualState::Replacing)
		{
			const float ReplacementStep = Config.RootIconReplacementFadeSeconds <= 0.0f
				? 1.0f
				: DeltaTime / Config.RootIconReplacementFadeSeconds;
			Row.ContentRetirementProgress = FMath::Min(
				1.0f, Row.ContentRetirementProgress + ReplacementStep);
			Row.IconRetirementProgress = FMath::Min(
				1.0f, Row.IconRetirementProgress + ReplacementStep);
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
		Row.ContentRetirementProgress = FadeSeconds <= 0.0f
			? 1.0f
			: FMath::Min(1.0f, Row.ContentRetirementProgress + DeltaTime / FadeSeconds);
		if (Row.RootVisualState
			== EWacomBattleCombatActivityRootVisualState::ContentRetiring
			&& Row.ContentRetirementProgress >= 1.0f)
		{
			Row.RootVisualState = EWacomBattleCombatActivityRootVisualState::IconResident;
			Row.UnprotectedElapsed = 0.0f;
		}
	}
	StartOldestResultRetirement(Config);

	for (int32 Index = VisibleRows.Num() - 1; Index >= 0; --Index)
	{
		const FVisibleRow& Row = VisibleRows[Index];
		const bool bTransientComplete = IsStreamLaneState(Row.RootVisualState)
			&& Row.bResultRetiring
			&& Row.ContentRetirementProgress >= 1.0f;
		const bool bReplacementComplete = Row.RootVisualState
				== EWacomBattleCombatActivityRootVisualState::Replacing
			&& Row.ContentRetirementProgress >= 1.0f
			&& Row.IconRetirementProgress >= 1.0f;
		if (bTransientComplete || bReplacementComplete)
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

void FWacomBattleCombatActivityPlayback::StartOldestResultRetirement(
	const FWacomBattleCombatActivityPlaybackConfig& Config)
{
	if (VisibleRows.ContainsByPredicate([](const FVisibleRow& Row)
		{
			return IsStreamLaneState(Row.RootVisualState)
				&& Row.bResultRetiring;
		}))
	{
		return;
	}

	FVisibleRow* OldestResult = VisibleRows.FindByPredicate([](const FVisibleRow& Row)
	{
		return IsStreamLaneState(Row.RootVisualState);
	});
	if (!OldestResult)
	{
		return;
	}
	const float RowCenterY = OldestResult->CurrentY + Config.RowHeightPixels * 0.5f;
	const float TopProximity = ResolveTopProximity(RowCenterY, Config);
	const float HoldSeconds = FMath::Lerp(
		Config.BottomRowHoldSeconds, Config.TopRowHoldSeconds, TopProximity);
	if (OldestResult->PresentedElapsed + KINDA_SMALL_NUMBER
			< Config.MinimumResultVisibleSeconds
		|| OldestResult->UnprotectedElapsed + KINDA_SMALL_NUMBER < HoldSeconds)
	{
		return;
	}
	OldestResult->bResultRetiring = true;
	const float FadeSeconds = FMath::Lerp(
		Config.BottomRowFadeSeconds, Config.TopRowFadeSeconds, TopProximity);
	if (FadeSeconds <= 0.0f)
	{
		OldestResult->ContentRetirementProgress = 1.0f;
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
		View.bPinnedRoot = VisibleRow.RootVisualState
			== EWacomBattleCombatActivityRootVisualState::Pinned;
		View.bRootActionLane = !IsStreamLaneState(VisibleRow.RootVisualState);
		View.bLatestRootAction = VisibleRow.PlaybackId == LastRootPlaybackId;
		View.bResidentLastActionIcon = VisibleRow.RootVisualState
			== EWacomBattleCombatActivityRootVisualState::IconResident;
		View.bReplacingLastActionIcon = VisibleRow.RootVisualState
			== EWacomBattleCombatActivityRootVisualState::Replacing;
		const float EnterAmount = Config.EnterSeconds <= 0.0f
			? 1.0f
			: FMath::Clamp(VisibleRow.EnterElapsed / Config.EnterSeconds, 0.0f, 1.0f);
		if (VisibleRow.RootVisualState
			== EWacomBattleCombatActivityRootVisualState::ContentRetiring
			|| VisibleRow.RootVisualState
				== EWacomBattleCombatActivityRootVisualState::IconResident)
		{
			View.Opacity = EnterAmount;
			View.ContentOpacity = 1.0f - VisibleRow.ContentRetirementProgress;
			View.IconOpacity = 1.0f;
		}
		else if (VisibleRow.RootVisualState
			== EWacomBattleCombatActivityRootVisualState::Replacing)
		{
			View.Opacity = EnterAmount;
			View.ContentOpacity = 1.0f - VisibleRow.ContentRetirementProgress;
			View.IconOpacity = 1.0f - VisibleRow.IconRetirementProgress;
		}
		else
		{
			View.Opacity = EnterAmount;
			View.ContentOpacity = 1.0f;
			View.IconOpacity = 1.0f;
			if (IsStreamLaneState(VisibleRow.RootVisualState))
			{
				View.Opacity *= 1.0f - VisibleRow.ContentRetirementProgress;
			}
		}
		View.TranslationY = Config.bReducedMotion
			? 0.0f
			: (1.0f - EnterAmount) * Config.RowHeightPixels;
	}
}
