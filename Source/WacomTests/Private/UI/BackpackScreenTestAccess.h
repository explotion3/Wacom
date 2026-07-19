// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class URunSession;
class UWacomBackpackScreen;
class UWacomBackpackWorkspaceWidget;
class UWacomDeckCardWidget;
enum class EZoneKind : uint8;
struct FWacomBackpackScreenAutomationTestView;
struct FWacomBackpackWorkspaceAutomationTestView;
struct FWacomBackpackExpandedPileFocusCard;
struct FWacomBackpackZonePileView;

struct FWacomBackpackPickupPointerSequenceProbe
{
	bool bCardMovable = false;
	bool bSelectedBeforePointerDown = false;
	bool bPointerEventIsLeftMouseButton = false;
	bool bPointerEventControlDown = false;
	bool bCarryingBeforePointerDown = false;
	bool bPointerDownHandled = false;
	bool bCarryStartedOnPointerDown = false;
	bool bPickupReleaseKeptCarry = false;
	bool bInitialReleaseGuardCleared = false;
	int32 NextLeftReleaseCount = 0;
	int32 NextRightReleaseCount = 0;
	int32 FirstLeftReleaseAfterMissedPickupUpCount = 0;
	int32 FirstRightReleaseAfterMissedPickupUpCount = 0;
};

struct FWacomBackpackPileMoveCancelProbe
{
	bool bBeganMove = false;
	FVector2D PilePositionBefore = FVector2D::ZeroVector;
	FVector2D PilePositionWhileMoving = FVector2D::ZeroVector;
	FVector2D PilePositionAfterCancel = FVector2D::ZeroVector;
	int32 PileZOrderBefore = 0;
	int32 PileZOrderWhileMoving = 0;
	int32 PileZOrderAfterCancel = 0;
};

struct FWacomBackpackScreenTestAccess
{
	static UWacomBackpackScreen* Create(UObject* Outer, URunSession* RunSession);
	static UWacomBackpackScreen* CreateWithClass(UObject* Outer, URunSession* RunSession, UClass* ScreenClass);
	static void Refresh(UWacomBackpackScreen& Screen);
	static void SetRunSession(UWacomBackpackScreen& Screen, URunSession* RunSession);
	static FWacomBackpackScreenAutomationTestView View(const UWacomBackpackScreen& Screen);
	static FText BuildMoveZoneNameText(EZoneKind Zone);
	static FText BuildMoveFailureToastText(FName DisabledReason);
	static FText BuildDeleteFailureToastText(FName DisabledReason);

	static UWacomDeckCardWidget* BattleDeckCard(const UWacomBackpackScreen& Screen, int32 Index);
	static UWacomDeckCardWidget* FluxContentCard(const UWacomBackpackScreen& Screen, int32 Index);
	static UWacomDeckCardWidget* BurdenCard(const UWacomBackpackScreen& Screen, int32 Index);
	static UWacomDeckCardWidget* SpecialOwnerCard(const UWacomBackpackScreen& Screen, FGuid OwnerInstanceId);
	static UWacomDeckCardWidget* SpecialContentCard(const UWacomBackpackScreen& Screen, FGuid OwnerInstanceId, int32 Index);

	static int32 RefreshApplyCount(const UWacomBackpackScreen& Screen);
	static int32 RefreshSkipCount(const UWacomBackpackScreen& Screen);
	static int32 SnapshotBuildCount(const UWacomBackpackScreen& Screen);
	static int32 SnapshotRevisionSkipCount(const UWacomBackpackScreen& Screen);
	static int32 WorkspacePileCount(const UWacomBackpackScreen& Screen);
	static int32 WorkspaceCardCount(const UWacomBackpackScreen& Screen);
#if WITH_EDITOR
	static bool UsesEmptyPIEValidationSnapshot(const UWacomBackpackScreen& Screen);
	static bool UsesNativeFallbackVisualClasses(const UWacomBackpackScreen& Screen);
#endif
	static bool WorkspaceChildFillsHost(const UWacomBackpackScreen& Screen);
	static bool WorkspaceOwnsPointerInput(const UWacomBackpackScreen& Screen);
	static TArray<FVector2D> WorkspaceCardPositions(const UWacomBackpackScreen& Screen);
	static TArray<float> WorkspaceCardRenderOpacities(const UWacomBackpackScreen& Screen);
	static bool ApplyStableWorkspaceGeometry(UWacomBackpackScreen& Screen, FVector2D LayoutSize);
	static void FlushDeferredWorkspaceCardFaceRender(UWacomBackpackScreen& Screen);
	static void ApplyWorkspaceLayerTransition(UWacomBackpackScreen& Screen, bool bTransitioning);
	static bool MarqueeCrossingCardPreservesMouseCapture(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static bool MarqueeCompletesWhenReleasedOverCard(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static FWacomBackpackPickupPointerSequenceProbe ProbeCardPickupPointerSequence(
		UWacomBackpackWorkspaceWidget& Workspace,
		UWacomDeckCardWidget& CardWidget,
		bool bPreselectCard = true);
	static void FlushWorkspaceCarryPointer(UWacomBackpackWorkspaceWidget& Workspace);
	static void SendWorkspaceCarryPointerEvents(
		UWacomBackpackWorkspaceWidget& Workspace,
		UWacomDeckCardWidget& CardWidget,
		TConstArrayView<FVector2D> PointerLocals);
	static bool StepWorkspaceCarryCurrentByWheel(
		UWacomBackpackWorkspaceWidget& Workspace,
		float WheelDelta);
	static void TickWorkspaceCardMotion(
		UWacomBackpackWorkspaceWidget& Workspace,
		float DeltaSeconds);
	static void MoveWorkspaceBrowsePointer(
		UWacomBackpackWorkspaceWidget& Workspace,
		FVector2D PointerLocal);
	static bool PressExpandedPileVisualCard(
		UWacomBackpackWorkspaceWidget& Workspace,
		FVector2D PointerLocal);
	static bool ResolveWorkspaceCardDetailAnchorRect(
		UWacomBackpackWorkspaceWidget& Workspace,
		UWacomDeckCardWidget& Card,
		FSlateRect& OutWorkspaceLocalRect);
	static void TickWorkspaceBrowseExit(
		UWacomBackpackWorkspaceWidget& Workspace,
		float DeltaSeconds);
	static bool MarqueeWorkspacePileContents(
		UWacomBackpackWorkspaceWidget& Workspace,
		EZoneKind Zone,
		FGuid OwnerInstanceId = FGuid());
	static void ClearWorkspaceSelection(
		UWacomBackpackWorkspaceWidget& Workspace);
	static bool CommitWorkspacePileMoveWithSynchronousTargetReconcile(
		UWacomBackpackWorkspaceWidget& Workspace,
		UWacomDeckCardWidget& CardWidget,
		EZoneKind Zone,
		FVector2D HeaderStart,
		FVector2D PointerEnd,
		FVector2D TargetCardCenter);
	static FWacomBackpackPileMoveCancelProbe CancelWorkspacePileMove(
		UWacomBackpackWorkspaceWidget& Workspace,
		EZoneKind Zone,
		FGuid OwnerInstanceId,
		FVector2D HeaderStart,
		FVector2D PointerEnd);
	static void ReconcileWorkspacePilesForTest(
		UWacomBackpackWorkspaceWidget& Workspace,
		TConstArrayView<FWacomBackpackZonePileView> Views,
		TConstArrayView<FSlateRect> Frames,
		TConstArrayView<FSlateRect> Headers,
		TConstArrayView<int32> LayerRanks);
	static void ForgetWorkspacePileRegistry(UWacomBackpackWorkspaceWidget& Workspace);
	static bool CommitWorkspaceReleaseBeforeTargetReconcile(
		UWacomBackpackWorkspaceWidget& Workspace);
	static EZoneKind ActiveWorkspaceZone(const UWacomBackpackScreen& Screen);
	static FGuid ActiveWorkspaceOwnerInstanceId(const UWacomBackpackScreen& Screen);
	static void ActivateZone(UWacomBackpackScreen& Screen, EZoneKind Zone, FGuid OwnerInstanceId = FGuid());
	static bool BeginWorkspaceCarry(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static bool BeginWorkspaceCarryForIds(UWacomBackpackScreen& Screen, TConstArrayView<FGuid> InstanceIds);
	static bool BeginWorkspaceMarquee(UWacomBackpackScreen& Screen);
	static bool IsWorkspaceMarqueeActive(const UWacomBackpackScreen& Screen);
	static bool PressWorkspaceEscape(UWacomBackpackScreen& Screen);
	static bool ReleaseCurrentToPileWithSynchronousRefresh(
		UWacomBackpackScreen& Screen,
		EZoneKind TargetZone,
		FGuid TargetOwnerInstanceId = FGuid());
	static bool ReleaseAllToPileWithSynchronousRefresh(
		UWacomBackpackScreen& Screen,
		EZoneKind TargetZone,
		FGuid TargetOwnerInstanceId = FGuid());
	static bool ClickExpandedPileHeaderThroughOverlappingCard(
		UWacomBackpackScreen& Screen,
		EZoneKind Zone,
		FGuid OwnerInstanceId = FGuid());
	static void ActivateWorkspaceScreen(UWacomBackpackScreen& Screen);
	static void DeactivateWorkspaceScreen(UWacomBackpackScreen& Screen);
	static FWacomBackpackWorkspaceAutomationTestView WorkspaceView(const UWacomBackpackScreen& Screen);
	static bool TickWorkspaceBaseCardLayoutTransitions(
		UWacomBackpackWorkspaceWidget& Workspace,
		float DeltaSeconds);
	static bool BeginDeleteConfirmation(UWacomBackpackScreen& Screen);
	static bool BeginDeleteConfirmationForIds(
		UWacomBackpackScreen& Screen,
		TConstArrayView<FGuid> InstanceIds);
	static void ConfirmDelete(UWacomBackpackScreen& Screen);
	static void CancelDelete(UWacomBackpackScreen& Screen);
	static bool IsDeleteConfirmationPending(const UWacomBackpackScreen& Screen);
	static int32 DeletePreviewCardCount(const UWacomBackpackScreen& Screen);
	static int32 DeletePreviewGoldReward(const UWacomBackpackScreen& Screen);

	static bool IsDetailVisible(const UWacomBackpackScreen& Screen);
	static FText DetailNameText(const UWacomBackpackScreen& Screen);
	static bool ShowDetailForCardWidget(UWacomBackpackScreen& Screen, UWacomDeckCardWidget* SourceWidget);
	static void HideDetail(UWacomBackpackScreen& Screen);

private:
	static TArray<UWacomDeckCardWidget*> WorkspaceCards(const UWacomBackpackScreen& Screen);
};

#endif
