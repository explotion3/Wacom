// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleFirstPersonDropResolver.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Battle/WacomBattleHandPresentationController.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomBattleHUDRuntime;
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

struct FWacomBattleActionPreviewRequestKey
{
	int32 SnapshotVersion = INDEX_NONE;
	EWacomBattleCardDropIntentKind IntentKind = EWacomBattleCardDropIntentKind::None;
	FGuid SourceCardInstanceId;
	EWacomInteractionTargetKind TargetKind = EWacomInteractionTargetKind::None;
	FGuid WorldTargetId;
	FGuid CardInstanceId;
	FName ZoneId = NAME_None;
	FGameplayTag TargetTag;
	FName StableTargetId = NAME_None;
	FName EncounterId = NAME_None;
	FName EnemySlotId = NAME_None;
	FName PartSlotId = NAME_None;
	bool bCanSubmit = false;
};

class FWacomBattleHUDFirstPersonHandBridge
{
public:
	explicit FWacomBattleHUDFirstPersonHandBridge(FWacomBattleHUDRuntime& InRuntime);
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
	void TickPendingPresentationFrames(float DeltaTime);
	void RecordPlayCommit(
		const FGuid& CardInstanceId,
		const FBattlePartSlotIdentity& TargetPartKey,
		const TOptional<FVector2D>& TargetWidgetPosition = TOptional<FVector2D>());
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildTransitionHints(
		const FBattleSnapshot& PreviousSnapshot,
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
	void HandlePointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandlePointerLeft();
	void HandleDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	bool TryStartDragByHandIndex(
		int32 OneBasedIndex,
		const TOptional<FVector2D>& InitialPointerWidgetPosition);
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
	void ResetActiveTargetPreviewState(bool bResetActionPreviewState = true);
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
		FWacomFirstPersonCardLayerPresentationFrame&& Frame);
	void ApplyPointerCameraLookOverrideToBattleCamera(
		const FWacomFirstPersonCardPointerView& PointerView);
	void ClearPointerCameraLookOverride();
	void ApplyDragCameraLookOverrideToBattleCamera(
		const FWacomFirstPersonCardDragView& DragView);
	void ClearDragCameraLookOverride();

	FWacomBattleHUDRuntime& Runtime;
	FWacomBattleFirstPersonDropResolver DropResolver;
	FWacomBattleHandPresentationController PresentationController;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> LastAnchor;
	TWeakObjectPtr<UWacomBattleEnemyPartPresentationComponent> CurrentDragPreviewPresentation;
	FWacomBattleCardTargetPreviewPresentationStateKey ActiveTargetPreviewState;
	FWacomBattleActionPreviewRequestKey ActiveActionPreviewRequestKey;
	FWacomFirstPersonCardDragView ActiveDragView;
	FGuid ActiveDragCardInstanceId;
	FWacomInteractionTargetHandle ActiveCardTargetHandle;
	bool bFirstPersonBattleHandLayerRuntimeActive = false;
	bool bFirstPersonCardDragActiveForBattleSceneHover = false;
	bool bHasActiveTargetPreviewLayer = false;
	bool bHasActiveTargetPreviewState = false;
	bool bHasActiveActionPreviewRequestKey = false;
	bool bHasActiveDragView = false;
	bool bHasActiveCardTargetHandle = false;
	float PendingHandAnchorEnterFrameElapsedSeconds = 0.0f;

	const FHandCardSnapshot* FindLastBattleHandCardSnapshot(const FGuid& CardInstanceId) const;
};
