// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomBattleHandPresentationController
{
public:
	void Reset();
	void ClearPendingTransitionEvents();
	void StoreTransitionEvents(const TArray<FBattleEvent>& Events);
	void RecordPlayCommit(
		const FGuid& CardInstanceId,
		const TOptional<FVector2D>& TargetWidgetPosition = TOptional<FVector2D>());
	void RecordHandTargetImpact(const FGuid& TargetCardInstanceId);
	bool HasPendingTransitionPresentation() const;
	void PreservePendingEntryRevealForNextRefresh();
	void DiscardSubmittedTransitionFrame();
	bool HasPendingHandAnchorEnterFrame() const;
	FWacomFirstPersonCardLayerPresentationFrame ConsumePendingHandAnchorEnterFrame();

	FWacomFirstPersonCardLayerPresentationFrame BuildFrame(
		const FBattleSnapshot& Snapshot,
		bool bSuppressed);
	FWacomFirstPersonCardLayerPresentationFrame BuildExplicitFrame(
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
		TOptional<FVector2D> TargetWidgetPosition;
	};

	FBattleSnapshot BuildEmptyHandBaseline(const FBattleSnapshot& Snapshot) const;
	void MarkSnapshotPresented(const FBattleSnapshot& Snapshot);
	void RecordSubmittedTransitionFrame();
	void RestoreSubmittedEntryRevealEventsIfNeeded();
	void StorePendingHandAnchorEnterFrame(
		const FBattleSnapshot& Snapshot,
		const TArray<FGuid>& CardInstanceIds);
	void ClearPendingHandAnchorEnterFrame();

	FBattleSnapshot LastPresentedSnapshot;
	FBattleSnapshot LastTransitionSnapshot;
	FBattleSnapshot PendingHandAnchorEnterSnapshot;
	TArray<FBattleEvent> PendingTransitionEvents;
	TArray<FBattleEvent> PendingCardDataChangeEvents;
	TArray<FPlayCommitHint> PendingPlayCommitHints;
	TArray<FGuid> PendingHandTargetImpactIds;
	TArray<FBattleEvent> SubmittedTransitionEvents;
	TArray<FPlayCommitHint> SubmittedPlayCommitHints;
	TArray<FGuid> SubmittedHandTargetImpactIds;
	TArray<FGuid> PendingHandAnchorEnterCardIds;
	bool bHasLastPresentedSnapshot = false;
	bool bHasTransitionSnapshot = false;
	bool bUseEmptyHandSnapshotForNextTransitionRefresh = false;
	bool bHasPendingHandAnchorEnterFrame = false;
};
