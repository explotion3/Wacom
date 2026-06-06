// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Battle/BattleHUD.h"

class UBattleHUD;
class UBattleSession;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
class UWacomFirstPersonCardAnchorComponent;
struct FBattleEvent;
struct FBattleSnapshot;
struct FHandCardSnapshot;
struct FWacomFirstPersonCardDragView;
struct FWacomFirstPersonCardLayerSlotView;

class FWacomBattleHUDFirstPersonHandBridge
{
public:
	explicit FWacomBattleHUDFirstPersonHandBridge(UBattleHUD& InHUD);
	~FWacomBattleHUDFirstPersonHandBridge();

	void SyncLayer(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints =
			TArray<FWacomFirstPersonCardLayerTransitionHint>());
	void ClearLayer();

	bool ShouldUseFirstPersonBattleHandLayer() const;
	bool ShouldEnableFirstPersonBattleHandInteraction() const;
	bool IsFirstPersonCardDragActiveForBattleSceneHover() const
	{
		return bFirstPersonCardDragActiveForBattleSceneHover;
	}

	UWacomFirstPersonCardAnchorComponent* ResolveAnchor() const;
	UWacomFirstPersonCardAnchorComponent* ResolveActiveAnchor() const;

	void StoreTransitionEvents(const TArray<FBattleEvent>& Events);
	void ClearPendingTransitionEvents();
	void RecordPlayCommit(const FGuid& CardInstanceId, const FGuid& TargetPartInstanceId);
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHints(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const;

	void ClearTransitionSnapshot();
	bool CanBuildTransitionHintsFor(const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHintsForRefresh(
		const FBattleSnapshot& NextSnapshot) const;
	void SetTransitionSnapshot(const FBattleSnapshot& Snapshot);

	void HandleCardClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleHoveredCardLayoutUpdated(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandlePointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandlePointerLeft();

	void ApplyDragCameraLookOverride(const FWacomFirstPersonCardDragView& DragView);
	void ClearDragCameraLookOverride();
	void ApplyPointerCameraLookOverride(const FWacomFirstPersonCardPointerView& PointerView);
	void ClearPointerCameraLookOverride();
	void UpdateDragTargetFeedback(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void ClearDragTargetFeedback(bool bClearFirstPersonCardLayerFeedback = true);

	FWacomBattleCardDropResolveResult ResolveDropIntent(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	TArray<FWacomFirstPersonCardTargetAffordance> BuildCardTargetAffordances(
		const FGuid& SourceCardId,
		const FBattleSnapshot& Snapshot,
		const UBattleSession& BattleSession) const;
	bool ProbeDragTarget(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		bool& bOutValidTarget) const;
	bool ShouldShowDragInspectDetail(const FWacomFirstPersonCardDragView& DragView) const;

	void BindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);
	void UnbindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);

private:
	struct FPlayCommitHint
	{
		FGuid CardInstanceId;
	};

	UBattleHUD& HUD;
	FBattleSnapshot LastTransitionSnapshot;
	TArray<FBattleEvent> PendingTransitionEvents;
	TArray<FPlayCommitHint> PendingPlayCommitHints;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> LastAnchor;
	TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> CurrentDragPreviewBridge;
	bool bHasTransitionSnapshot = false;
	bool bFirstPersonBattleHandLayerRuntimeActive = false;
	bool bFirstPersonCardDragActiveForBattleSceneHover = false;

	const FHandCardSnapshot* FindLastBattleHandCardSnapshot(const FGuid& CardInstanceId) const;
};
