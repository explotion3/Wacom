// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomBattleHUDRuntime;
struct FBattleSnapshot;

class FWacomBattleHUDSnapshotPresenter
{
public:
	explicit FWacomBattleHUDSnapshotPresenter(FWacomBattleHUDRuntime& InRuntime);

	void RefreshFromSnapshot(const FBattleSnapshot& Snapshot);
	void RefreshFromPresentationPhase(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints);

private:
	FWacomBattleHUDRuntime& Runtime;

	void RefreshPileViews(const FBattleSnapshot& Snapshot);
	void RefreshBoundBattleWidgets(const FBattleSnapshot& Snapshot);
};
