// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"

class AActor;
class AWacomPlayerController;
class AWacomRunPathBranchTargetActor;

/**
 * App-private Run 道路选择表现控制器。
 *
 * 只把 Coordinator 的只读路线状态映射为场景焦点与玩家选择意图；
 * 不读取地图规则，也不直接调用 RunSession。
 */
class FWacomRunPathBranchSelectionController
{
public:
	void Initialize(
		AWacomPlayerController& InOwner,
		const TArray<TWeakObjectPtr<AWacomRunPathBranchTargetActor>>& InTargets);
	void Shutdown();

	void ApplyRouteChoiceState(const FWacomRunRouteChoiceState& InState);
	void SetPresentationEnabled(bool bEnabled);
	void TickPointerHover();

	bool ShiftFocus(int32 Direction);
	bool ConfirmFocused();
	bool TrySelectHitActor(AActor* HitActor);
	void PulseAvailable();

	bool IsChoiceRequired() const;
	bool IsPresentationValid() const { return bPresentationValid; }
	FName GetFocusedEdgeId() const;
	FText BuildInteractionPrompt() const;

private:
	void RebuildChoiceTargets();
	void ApplyTargetPresentation();
	void HideAllTargets();
	int32 FindTargetIndex(const AWacomRunPathBranchTargetActor* Target) const;
	int32 ChooseInitialFocusIndex() const;

	TWeakObjectPtr<AWacomPlayerController> Owner;
	TArray<TWeakObjectPtr<AWacomRunPathBranchTargetActor>> AllTargets;
	TArray<TWeakObjectPtr<AWacomRunPathBranchTargetActor>> OrderedChoiceTargets;
	FWacomRunRouteChoiceState RouteChoiceState;
	int32 FocusedIndex = INDEX_NONE;
	bool bPresentationEnabled = false;
	bool bPresentationValid = false;
};
