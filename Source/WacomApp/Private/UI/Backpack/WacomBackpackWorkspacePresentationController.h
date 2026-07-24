// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"

class FWacomBackpackWorkspaceRuntimeHost;
#if WITH_AUTOMATION_TESTS
struct FWacomBackpackScreenTestAccess;
#endif

/** Workspace scene-level presentation state; UMG application remains in the Widget adapter. */
class FWacomBackpackWorkspacePresentationController
{
public:
	struct FExpandedPileFocusState
	{
		EZoneKind Zone = EZoneKind::Backpack;
		FGuid OwnerInstanceId;
		FSlateRect HeaderRect;
		FSlateRect CorridorRect;
		TArray<FWacomBackpackExpandedPileFocusCard> Cards;
		int32 FocusIndex = INDEX_NONE;
		float LensFocus = 0.0f;
		int32 LensLeftStackCount = 0;
		int32 LensExpandedStartIndex = INDEX_NONE;
		int32 LensExpandedCardCount = 0;
		int32 LensRightStackCount = 0;
		bool bHasLensLayout = false;
		FVector2D PointerLocal = FVector2D::ZeroVector;
		float ExitDelayRemainingSeconds = 0.0f;
		bool bExitPending = false;
	};

	void SetSimplifiedMotion(bool bInSimplified) { bSimplifiedMotion = bInSimplified; }
	bool IsSimplifiedMotion() const { return bSimplifiedMotion; }

	void SetCarryInputSuspended(bool bInSuspended) { bCarryInputSuspended = bInSuspended; }
	bool IsCarryInputSuspended() const { return bCarryInputSuspended; }

	void Reset()
	{
		*this = FWacomBackpackWorkspacePresentationController();
	}

	void RequestRefresh(
		FWacomBackpackWorkspaceRuntimeHost& Host,
		EWacomBackpackWorkspacePresentationDirty Reasons,
		TConstArrayView<FGuid> CardInstanceIds = {},
		bool bAllCards = false,
		bool bFlushImmediately = true);
	void Flush(FWacomBackpackWorkspaceRuntimeHost& Host);
	void WakeFrame(FWacomBackpackWorkspaceRuntimeHost& Host);
	void RefreshFrameWork(FWacomBackpackWorkspaceRuntimeHost& Host);
	EActiveTimerReturnType TickFrame(
		FWacomBackpackWorkspaceRuntimeHost& Host,
		uint64 TimerGeneration,
		float DeltaSeconds);

private:
	TWeakObjectPtr<UWacomDeckCardWidget> HoveredCardWidget;
	EZoneKind ExpandedContentZone = EZoneKind::Backpack;
	FGuid ExpandedContentOwnerInstanceId;
	FSlateRect ExpandedContentBounds;
	bool bHasExpandedContentBounds = false;
	EZoneKind HoverExpandZone = EZoneKind::Backpack;
	FGuid HoverExpandOwnerInstanceId;
	bool bHoverExpandTimerActive = false;
	float HoverExpandElapsedSeconds = 0.0f;
	bool bPileCollapseAnimationPending = false;
	EZoneKind CollapsingPileZone = EZoneKind::Backpack;
	FGuid CollapsingPileOwnerInstanceId;

	FVector2D CarryAnchorLocal = FVector2D::ZeroVector;
	FVector2D CarryVisualAnchorLocal = FVector2D::ZeroVector;
	bool bCarryVisualAnchorInitialized = false;
	FVector2D QueuedCarryPointerLocal = FVector2D::ZeroVector;
	FVector2D QueuedPilePointerLocal = FVector2D::ZeroVector;
	bool bHasQueuedCarryPointer = false;
	bool bHasQueuedPilePointer = false;
	bool bCarryStripLayoutDirty = false;
	TArray<FGuid> LastCarryStripInstanceIds;
	TWeakObjectPtr<UWacomDeckCardWidget> PreviousCarryCurrentCard;
	bool bCarryCurrentExplicitlySelectedByWheel = false;
	bool bCarryDropValid = false;
	bool bCarryDropRejected = false;
	int32 LastCarryStripCurrentIndex = INDEX_NONE;
	int32 LastCarryStripDefaultIndex = INDEX_NONE;
	int32 LastCarryStripWindowStartIndex = INDEX_NONE;
	int32 CarryStripLayoutRebuildCount = 0;
	int32 StaticCardPresentationUpdateCount = 0;
	int32 CarryVisualAnchorApplyCount = 0;

	FVector2D StableLayoutSize = FVector2D::ZeroVector;
	bool bHasStableLayoutSize = false;

	FExpandedPileFocusState ExpandedPileFocus;
	bool bExpandedPileLensInputLocked = false;
	EZoneKind SelectionFrozenZone = EZoneKind::Backpack;
	FGuid SelectionFrozenOwnerInstanceId;
	int32 ExpandedPileFocusLayoutRebuildCount = 0;

	bool bSimplifiedMotion = false;
	bool bCarryInputSuspended = false;

#if WITH_AUTOMATION_TESTS
	struct FAutomationMetrics
	{
		int32 PresentationFlushCount = 0;
		int32 NavigationTargetsApplyCount = 0;
		int32 CarryTopologyApplyCount = 0;
		int32 CarryStripApplyCount = 0;
		int32 StaticCardStageApplyCount = 0;
		int32 CardSemanticsStageApplyCount = 0;
		int32 MotionTargetApplyCount = 0;
		int32 NavigationPresentationApplyCount = 0;
		int32 AccessibilityApplyCount = 0;
		int32 PaintInvalidationApplyCount = 0;
		int32 FrameSchedulerTickCount = 0;
		int32 BaseCardLayoutTransitionTickCount = 0;
		int32 BaseCardLayoutTransitionApplyCount = 0;
		bool bLastPresentationAppliedAllCards = false;
		TArray<FGuid> LastPresentationAppliedInstanceIds;
		TArray<FName> LastFramePhaseOrder;
	};

	FAutomationMetrics AutomationMetrics;
#endif

	friend class FWacomBackpackWorkspaceRuntimeHost;
	friend class UWacomBackpackWorkspaceWidget;
#if WITH_AUTOMATION_TESTS
	friend struct FWacomBackpackScreenTestAccess;
#endif
};
