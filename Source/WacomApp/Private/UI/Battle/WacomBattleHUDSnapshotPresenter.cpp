// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDSnapshotPresenter.h"

#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattlePileCountPresentation.h"
#include "UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Common/PileCountView.h"

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
	RefreshPileViews(Snapshot);
	Runtime.SyncFirstPersonBattleHandLayer(Snapshot);

	if (Snapshot.Phase == EBattlePhase::BattleEnd)
	{
		Runtime.ClearPendingFirstPersonCardTransitionEvents();
		Runtime.ClearBattlePresentationStack();
		Runtime.ClearPendingTurnBoundaryCommand();
		Runtime.ClearBattleSceneEnemyPartHoverProbe(TEXT("BattleEnd"));
		Runtime.ClearLastBattleSnapshot();
		Runtime.GetFirstPersonHandBridge().ClearTransitionSnapshot();
		Runtime.ResetDrawPileFeedback(Snapshot.PileCounts.DrawCount);
		Runtime.SetUIState(EBattleUIState::BattleEnd);
	}

	RefreshBoundBattleWidgets(Snapshot);
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
	Runtime.SyncBattleEnemyPartWorldTargets(Snapshot);
}

void FWacomBattleHUDSnapshotPresenter::RefreshFromPresentationPhase(
	const FBattleSnapshot& Snapshot,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints)
{
	Runtime.HideCardDetailPanel();
	Runtime.SetLastBattleSnapshot(Snapshot);
	RefreshBoundBattleWidgets(Snapshot);
	RefreshPileViews(Snapshot);
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
	Runtime.SyncFirstPersonBattleHandLayer(Snapshot, TransitionHints, FeedbackHints);
	Runtime.SyncBattleEnemyPartWorldTargets(Snapshot);
}

void FWacomBattleHUDSnapshotPresenter::RefreshCombatPresentationFrame(
	const FBattleSnapshot& Snapshot)
{
	Runtime.HideCardDetailPanel();
	Runtime.SetLastBattleSnapshot(Snapshot);
	RefreshBoundBattleWidgets(Snapshot);
	Runtime.RefreshCommandBarFromSnapshot(Snapshot);
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
		DiscardPileView->SetCountDisplayText(
			WacomBattlePileCountPresentation::BuildDiscardPileCountDisplayText(
				Snapshot.PileCounts.DiscardCount,
				Snapshot.PileCounts.PlayedCount));
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
