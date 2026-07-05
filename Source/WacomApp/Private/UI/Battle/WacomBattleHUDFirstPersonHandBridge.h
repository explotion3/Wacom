// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHandPresentationController.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class UBattleHUD;
class UBattleSession;
class UWacomBattleEnemyPartPresentationComponent;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
class UWacomFirstPersonCardAnchorComponent;
struct FBattleCardTargetPreview;
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

	void SyncLayer(const FBattleSnapshot& Snapshot);
	void SyncLayer(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	void SyncLayer(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints);
	void ClearLayer(bool bClearPendingTransitionEvents = true);
	void SuppressLayerForEntry();

	bool ShouldUseFirstPersonBattleHandLayer() const;
	bool ShouldEnableFirstPersonBattleHandInteraction() const;
	bool IsFirstPersonCardDragActiveForBattleSceneHover() const
	{
		return bFirstPersonCardDragActiveForBattleSceneHover;
	}

	UWacomFirstPersonCardAnchorComponent* ResolveAnchor() const;
	UWacomFirstPersonCardAnchorComponent* ResolveActiveAnchor() const;

	void StoreTransitionEvents(const TArray<FBattleEvent>& Events);
	bool HasPendingTransitionPresentation() const;
	void ClearPendingTransitionEvents();
	void PreservePendingEntryRevealForNextRefresh();
	bool HasPendingPresentationFrame() const;
	void RecordPlayCommit(const FGuid& CardInstanceId, const FBattlePartSlotIdentity& TargetPartKey);
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHints(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildFeedbackHints(
		const FBattleSnapshot& NextSnapshot) const;

	void ClearTransitionSnapshot();
	bool CanBuildTransitionHintsFor(const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHintsForRefresh(
		const FBattleSnapshot& NextSnapshot) const;
	void SetTransitionSnapshot(const FBattleSnapshot& Snapshot);

	void HandleCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleHoveredCardLayoutUpdated(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardTargetHovered(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleCardTargetUnhovered(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleHoveredCardTargetUpdated(
		const FWacomInteractionTargetHandle& CardTargetHandle,
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
	void UpdateDragTargetFeedback(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		const FWacomBattleCardDropResolveResult& DropResult,
		bool bForceApplyTargetPreview = false);
	bool ApplyActiveCardTargetPreview(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& TargetSlotView);
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
	bool IsActiveDragCardTargetHandle(const FWacomInteractionTargetHandle& CardTargetHandle) const;
	FWacomFirstPersonCardDragView BuildSemanticActiveCardTargetDragView(
		const FWacomInteractionTargetHandle& CardTargetHandle,
		const FWacomFirstPersonCardLayerSlotView& TargetSlotView) const;
	void ApplyTargetPreviewPresentationToLayer(
		const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation);
	void ClearTargetPreviewLayer();
	void RestoreBaseTargetPreviewLayer();
	void RecomposeFirstPersonHandLayer(const FBattleSnapshot& Snapshot);
	bool IsSameActiveTargetPreviewState(
		const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation) const;
	void StoreActiveTargetPreviewState(
		const FWacomBattleCardTargetPreviewPresentation& TargetPreviewPresentation);
	void ResetActiveTargetPreviewState();
	void ApplyPendingTargetingFlag(TArray<FWacomFirstPersonCardLayerEntry>& Entries) const;

	void BindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);
	void UnbindLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);

private:
	void SyncLayerInternal(
		const FBattleSnapshot& Snapshot,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>* TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>* FeedbackHints);
	void ApplyPresentationFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FWacomBattleHandPresentationFrame&& Frame);

	UBattleHUD& HUD;
	FWacomBattleHandPresentationController PresentationController;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> LastAnchor;
	TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> CurrentDragPreviewPresentation;
	FWacomBattleCardTargetPreviewPresentationStateKey ActiveTargetPreviewState;
	FWacomFirstPersonCardDragView ActiveDragView;
	FGuid ActiveDragCardInstanceId;
	FWacomInteractionTargetHandle ActiveCardTargetHandle;
	bool bFirstPersonBattleHandLayerRuntimeActive = false;
	bool bFirstPersonCardDragActiveForBattleSceneHover = false;
	bool bHasActiveTargetPreviewLayer = false;
	bool bHasActiveTargetPreviewState = false;
	bool bHasActiveDragView = false;
	bool bHasActiveCardTargetHandle = false;

	const FHandCardSnapshot* FindLastBattleHandCardSnapshot(const FGuid& CardInstanceId) const;
};
