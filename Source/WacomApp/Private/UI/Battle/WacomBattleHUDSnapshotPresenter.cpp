// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDSnapshotPresenter.h"

#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Common/PileCountView.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUDRuntime"

namespace
{
	FText BuildDiscardPileCountDisplayText(const FPileCountsSnapshot& PileCounts)
	{
		if (PileCounts.PlayedCount <= 0)
		{
			return FText::AsNumber(PileCounts.DiscardCount);
		}

		return FText::Format(
			LOCTEXT("DiscardPileWithPlayedCountFormat", "{0}+{1}"),
			FText::AsNumber(PileCounts.DiscardCount),
			FText::AsNumber(PileCounts.PlayedCount));
	}
}

FWacomBattleHUDSnapshotPresenter::FWacomBattleHUDSnapshotPresenter(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDSnapshotPresenter::RefreshFromSnapshot(
	const FBattleSnapshot& Snapshot)
{
	Runtime.HideCardDetailPanel();
	Runtime.SetLastBattleSnapshot(Snapshot);
	Runtime.SyncFirstPersonBattleHandLayer(Snapshot);

	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		Runtime.ClearPendingFirstPersonCardTransitionEvents();
		Runtime.ClearBattlePresentationStack();
		Runtime.ClearPendingTurnBoundaryCommand();
		Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("BattleEnd"));
		Runtime.ClearLastBattleSnapshot();
		Runtime.GetFirstPersonHandBridge().ClearTransitionSnapshot();
		Runtime.SetUIState(EBattleUIState::BattleEnd);
	}

	RefreshPileViews(Snapshot);
	RefreshBoundBattleWidgets(Snapshot);
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
	Runtime.SyncBattleEnemyPartWorldTargets(Snapshot);
}

void FWacomBattleHUDSnapshotPresenter::RefreshFromPresentationPhase(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints)
{
	Runtime.HideCardDetailPanel();
	Runtime.SetLastBattleSnapshot(Snapshot);
	RefreshBoundBattleWidgets(Snapshot);
	RefreshPileViews(Snapshot);
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
	Runtime.SyncFirstPersonBattleHandLayer(Snapshot, TransitionHints);
	Runtime.SyncBattleEnemyPartWorldTargets(Snapshot);
}

void FWacomBattleHUDSnapshotPresenter::RefreshPileViews(
	const FBattleSnapshot& Snapshot)
{
	if (UPileCountView* DrawPileView = Runtime.Host().GetDrawPileView())
	{
		DrawPileView->SetCount(Snapshot.PileCounts.DrawCount);
	}
	if (UPileCountView* DiscardPileView = Runtime.Host().GetDiscardPileView())
	{
		DiscardPileView->SetCount(Snapshot.PileCounts.DiscardCount);
		DiscardPileView->SetCountDisplayText(BuildDiscardPileCountDisplayText(Snapshot.PileCounts));
	}
	if (UPileCountView* ExhaustPileView = Runtime.Host().GetExhaustPileView())
	{
		ExhaustPileView->SetCount(Snapshot.PileCounts.ExhaustCount);
	}
}

void FWacomBattleHUDSnapshotPresenter::RefreshBoundBattleWidgets(
	const FBattleSnapshot& Snapshot)
{
	Runtime.Host().RefreshChildBattleWidgetsFromSnapshot(Snapshot);
}

#undef LOCTEXT_NAMESPACE
