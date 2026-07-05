// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomBattleHandPresentationFrame
{
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	TArray<FWacomFirstPersonCardLayerTransitionHint> TransitionHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints;
	bool bHasTransitionFrame = false;
};

class FWacomBattleHandPresentationController
{
public:
	void Reset();
	void ClearPendingTransitionEvents();
	void StoreTransitionEvents(const TArray<FBattleEvent>& Events);
	void RecordPlayCommit(const FGuid& CardInstanceId);
	bool HasPendingTransitionPresentation() const;
	void PreservePendingEntryRevealForNextRefresh();
	void DiscardSubmittedTransitionFrame();

	FWacomBattleHandPresentationFrame BuildFrame(
		const FBattleSnapshot& Snapshot,
		bool bSuppressed);
	FWacomBattleHandPresentationFrame BuildExplicitFrame(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints =
			TArray<FWacomFirstPersonCardLayerFeedbackHint>());

	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHints(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHintsForRefresh(
		const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildFeedbackHints(
		const FBattleSnapshot& NextSnapshot) const;
	void SetTransitionSnapshot(const FBattleSnapshot& Snapshot);
	void ClearTransitionSnapshot();

	const FBattleSnapshot& GetLastPresentedSnapshot() const { return LastPresentedSnapshot; }
	bool HasLastPresentedSnapshot() const { return bHasLastPresentedSnapshot; }

private:
	struct FPlayCommitHint
	{
		FGuid CardInstanceId;
	};

	FBattleSnapshot BuildEmptyHandBaseline(const FBattleSnapshot& Snapshot) const;
	void MarkSnapshotPresented(const FBattleSnapshot& Snapshot);
	void RecordSubmittedTransitionFrame();
	void RestoreSubmittedEntryRevealEventsIfNeeded();

	FBattleSnapshot LastPresentedSnapshot;
	FBattleSnapshot LastTransitionSnapshot;
	TArray<FBattleEvent> PendingTransitionEvents;
	TArray<FPlayCommitHint> PendingPlayCommitHints;
	TArray<FBattleEvent> SubmittedTransitionEvents;
	TArray<FPlayCommitHint> SubmittedPlayCommitHints;
	bool bHasLastPresentedSnapshot = false;
	bool bHasTransitionSnapshot = false;
	bool bUseEmptyHandSnapshotForNextTransitionRefresh = false;
};
