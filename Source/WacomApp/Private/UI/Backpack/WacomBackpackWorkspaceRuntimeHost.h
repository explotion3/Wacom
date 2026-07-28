// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomBackpackWorkspaceWidget;
class UWacomBackpackWorkspaceStyle;
class UWacomBackpackZonePileWidget;
class UWacomDeckCardWidget;
class FWacomBackpackWorkspaceFrameScheduler;
class FWacomBackpackWorkspaceInteractionModel;
struct FPointerEvent;
struct FKeyEvent;
struct FWacomBackpackPendingPilePress;
struct FWacomBackpackPileMoveVisualSnapshot;
struct FWacomBackpackWorkspacePileMoveState;
struct FWacomBackpackZoneKey;
struct FWacomBackpackWorkspacePresentationRequest;
enum class EWacomBackpackWorkspaceReleaseTargetKind : uint8;

/**
 * Workspace Runtime Controller 与 UMG/Slate Adapter 之间的唯一 seam。
 *
 * Host 不拥有规则、手势、导航或表现状态；它只暴露当前几何、Style、
 * Interaction/Registry 事实，以及必须落到 UWidget/Slate 的应用操作与
 * 原生意图广播。各 Controller 通过语义操作使用 Host，不直接组合 Adapter 字段。
 */
class WACOMAPP_API FWacomBackpackWorkspaceRuntimeHost
{
public:
	explicit FWacomBackpackWorkspaceRuntimeHost(
		UWacomBackpackWorkspaceWidget& InAdapter)
		: Adapter(InAdapter)
	{
	}

	/** Runtime is present and the Adapter has not completed destruction. */
	bool IsValid() const;
	bool HasInteractionModel() const;
	FWacomBackpackWorkspaceInteractionModel* GetInteractionModel();
	const FWacomBackpackWorkspaceInteractionModel* GetInteractionModel() const;
	uint64 GetCurrentStorageRevision() const;
	FWacomBackpackWorkspaceFrameScheduler& GetFrameScheduler();
	const UWacomBackpackWorkspaceStyle& GetStyle() const;
	FVector2D ToLocalPointer(const FPointerEvent& Event) const;

	void WakeFrameScheduler();
	void EnsureFrameSchedulerRunning();
	void FlushPresentation();
	void RefreshFrameWork();

	void RecordPresentationFlush(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ApplyNavigationTargets();
	void ApplyCarryTopology();
	bool IsCarryStripDirty() const;
	void ApplyCarryStrip();
	void ApplyStaticCards(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ApplyCardSemantics(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ApplyMotionTarget();
	void ApplyNavigationPresentation(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ApplyAccessibility(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ApplyPaintInvalidation();
	void CollapseCompatibilityMarquee();

	void RecordFrameTick();
	void BeginFramePhaseRecording();
	void RecordFramePhase(FName Phase);
	FVector2D GetLocalGeometrySize() const;
	FVector2D GetLayoutSpaceSize() const;
	bool AcceptStableLayoutGeometry(FVector2D LayoutSize);
	bool IsCarrying() const;
	bool IsPileMoving() const;
	bool IsCarryInputSuspended() const;
	void RelinquishSemanticNavigationForPointerInput();
	void SyncExpandedPileLensInputLock(const FPointerEvent& Event);
	void SetExpandedPileLensInputLocked(bool bLocked, bool bResumeImmediately);
	UWacomDeckCardWidget* ResolveExpandedPileVisualCard(
		FVector2D PointerLocal) const;
	UWacomBackpackZonePileWidget* FindPileHeaderAt(
		FVector2D PointerLocal) const;
	bool DoesPileMatchExpandedFocus(
		const UWacomBackpackZonePileWidget& Pile) const;
	FWacomBackpackZoneKey ResolveMarqueeSource(
		FVector2D PointerLocal) const;
	void ReconcileExpandedPileFocusForMarqueeSource(
		const FWacomBackpackZoneKey& SourceZone);
	void ClearExpandedPileFocus(bool bAnimateReturn);
	void UpdateExpandedPileFocus(FVector2D PointerLocal);
	void BeginExpandedPileFocusExit();
	bool HasPresentationFocusedCard() const;
	void UpdateMotionPointer(FVector2D PointerLocal);
	void BeginSelectionVisualFreeze(const FWacomBackpackZoneKey& SourceZone);
	void EndSelectionVisualFreeze(bool bAnimateReturn);
	void UpdateSelectionVisualFreezeLifetime();
	void NotifyCarryStarted(
		FVector2D PointerLocal,
		TConstArrayView<FGuid> InstanceIds);
	void NotifySelectionChanged(
		TConstArrayView<FGuid> ChangedInstanceIds,
		bool bBroadcast = true);
	void InvalidatePaint();
	void QueueCarryPointer(FVector2D PointerLocal);
	void SyncCarryPointerForRelease(FVector2D PointerLocal);
	void BroadcastPointerRelease(bool bReleaseAll);
	void BroadcastInteractionChanged();
	void BroadcastPileExpansion(UWacomBackpackZonePileWidget& Pile);
	FWacomBackpackPileMoveVisualSnapshot CapturePileMoveVisualSnapshot(
		UWacomBackpackZonePileWidget& Pile,
		const FWacomBackpackZoneKey& Zone) const;
	void RestorePileMoveVisualSnapshot(
		const FWacomBackpackPileMoveVisualSnapshot& Snapshot);
	void QueuePilePointer(FVector2D PointerLocal);
	void FlushPilePointer();
	void ApplyActivePileMove();
	TArray<FSlateRect> CollectOccupiedPileHeaders(
		const FWacomBackpackZoneKey& ExcludedZone) const;
	void CommitPileMoveVisual(
		const FWacomBackpackWorkspacePileMoveState& Completed,
		FVector2D SnappedTopLeft);
	void BroadcastPileMoveCommitted(
		const FWacomBackpackWorkspacePileMoveState& Completed,
		FVector2D SnappedTopLeft);
	void ClearQueuedPilePointer();
	void RememberPreviousCarryCurrentCard(FGuid InstanceId);
	void NotifyCarryCurrentChanged(
		TConstArrayView<FGuid> ChangedInstanceIds,
		bool bIncludeCarryTopology,
		bool bCurrentChanged);
	void ReconcileNavigationTargetsForInput();
	void NotifyNavigationMoved(
		TConstArrayView<FGuid> ChangedInstanceIds);
	void BroadcastRelease(
		bool bReleaseAll,
		EWacomBackpackWorkspaceReleaseTargetKind TargetKind,
		const FWacomBackpackZoneKey& TargetZone);
	void BroadcastPileExpansion(
		const FWacomBackpackZoneKey& Zone);
	UWacomDeckCardWidget* FindBoundCard(FGuid InstanceId) const;
	void BroadcastControlsHelpRequested();
	bool IsExpandedPileLensInputLocked() const;
	bool HasCancelableInteraction() const;
	void CancelInteraction(bool bAnimateCarryReturn);
	bool HasExpandedContent() const;
	void BroadcastCollapseExpandedPileRequested();
	bool TryGetCursorLocalPosition(FVector2D& OutPointerLocal) const;
	void UpdateCarryAnchor(FVector2D PointerLocal);
	void ApplyCarryVisualAnchor(float DeltaSeconds);
	void QueueAndFlushPilePointer(FVector2D PointerLocal);
	void AdvanceHoverExpandDelay(float DeltaSeconds);
	bool AdvanceBaseCardLayoutTransitions(float DeltaSeconds);
	void AdvanceLayoutAndMotion(float DeltaSeconds);
	bool AdvanceSaleDeparture(float DeltaSeconds);
	void AdvanceFocusAndSettlement(float DeltaSeconds);
	void RefreshExpandedPileVisualHitAtCachedPointer();
	void AdvanceExpandedPileFocusExit(float DeltaSeconds);
	void FinalizeCompletedSettlements();
	void CompletePileCollapseIfReady();
	void ExecuteDeferredCardFaceRender();

	bool IsHoverExpandDelayActive() const;
	bool HasBaseLayoutTransitions() const;
	bool MotionWantsTick() const;
	bool HasActiveSettlements() const;
	bool IsFocusExitPending() const;
	bool IsPileCollapsePending() const;
	bool CanAdvanceSaleDeparture() const;
	bool CanWakeDeferredCardFaceRender() const;

private:
	UWacomBackpackWorkspaceWidget& Adapter;
};
