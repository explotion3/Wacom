// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomBackpackWorkspaceWidget;
class UWacomBackpackWorkspaceStyle;
class FWacomBackpackWorkspaceFrameScheduler;
struct FWacomBackpackWorkspacePresentationRequest;

/**
 * Presentation Runtime 与 UMG/Slate Adapter 之间的唯一 seam。
 *
 * Host 不拥有规则或表现状态；它只暴露当前几何、Style、Interaction/Registry
 * 事实和必须落到 UWidget/Slate 的应用操作。阶段顺序与帧工作推导由
 * FWacomBackpackWorkspacePresentationController 决定。
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
	FWacomBackpackWorkspaceFrameScheduler& GetFrameScheduler();
	const UWacomBackpackWorkspaceStyle& GetStyle() const;

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
	bool AcceptStableLayoutGeometry(FVector2D LayoutSize);
	bool IsCarrying() const;
	bool IsPileMoving() const;
	bool IsCarryInputSuspended() const;
	bool TryGetCursorLocalPosition(FVector2D& OutPointerLocal) const;
	void UpdateCarryAnchor(FVector2D PointerLocal);
	void ApplyCarryVisualAnchor(float DeltaSeconds);
	void QueueAndFlushPilePointer(FVector2D PointerLocal);
	void ClearQueuedPilePointer();
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
