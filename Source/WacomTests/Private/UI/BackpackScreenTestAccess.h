// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class URunSession;
class UWacomBackpackScreen;
class UWacomBackpackWorkspaceWidget;
class UWacomDeckCardWidget;
class UWacomSpecialZoneWidget;
enum class EZoneKind : uint8;
struct FWacomBackpackScreenAutomationTestView;
struct FWacomBackpackWorkspaceAutomationTestView;

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
	static UWacomSpecialZoneWidget* SpecialZone(const UWacomBackpackScreen& Screen, int32 Index);

	static int32 RefreshApplyCount(const UWacomBackpackScreen& Screen);
	static int32 RefreshSkipCount(const UWacomBackpackScreen& Screen);
	static int32 SnapshotBuildCount(const UWacomBackpackScreen& Screen);
	static int32 SnapshotRevisionSkipCount(const UWacomBackpackScreen& Screen);
	static int32 ZoneRackEntryCount(const UWacomBackpackScreen& Screen);
	static int32 WorkspaceCardCount(const UWacomBackpackScreen& Screen);
	static bool WorkspaceChildFillsHost(const UWacomBackpackScreen& Screen);
	static bool WorkspaceOwnsPointerInput(const UWacomBackpackScreen& Screen);
	static TArray<FVector2D> WorkspaceCardPositions(const UWacomBackpackScreen& Screen);
	static TArray<float> WorkspaceCardRenderOpacities(const UWacomBackpackScreen& Screen);
	static bool ApplyStableWorkspaceGeometry(UWacomBackpackScreen& Screen, FVector2D LayoutSize);
	static void FlushDeferredWorkspaceCardFaceRender(UWacomBackpackScreen& Screen);
	static void ApplyWorkspaceLayerTransition(UWacomBackpackScreen& Screen, bool bTransitioning);
	static bool MarqueeCrossingCardPreservesMouseCapture(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static bool MarqueeCompletesWhenReleasedOverCard(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static FWacomBackpackPickupPointerSequenceProbe ProbeSelectedCardPickupPointerSequence(
		UWacomBackpackWorkspaceWidget& Workspace,
		UWacomDeckCardWidget& CardWidget);
	static EZoneKind ActiveWorkspaceZone(const UWacomBackpackScreen& Screen);
	static FGuid ActiveWorkspaceOwnerInstanceId(const UWacomBackpackScreen& Screen);
	static void ActivateZone(UWacomBackpackScreen& Screen, EZoneKind Zone, FGuid OwnerInstanceId = FGuid());
	static bool BeginWorkspaceCarry(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static bool BeginWorkspaceCarryForIds(UWacomBackpackScreen& Screen, TConstArrayView<FGuid> InstanceIds);
	static bool BeginWorkspaceMarquee(UWacomBackpackScreen& Screen);
	static bool IsWorkspaceMarqueeActive(const UWacomBackpackScreen& Screen);
	static bool PressWorkspaceEscape(UWacomBackpackScreen& Screen);
	static bool ReleaseCurrentToRackWithSynchronousRefresh(
		UWacomBackpackScreen& Screen,
		EZoneKind TargetZone,
		FGuid TargetOwnerInstanceId = FGuid());
	static void DeactivateWorkspaceScreen(UWacomBackpackScreen& Screen);
	static FWacomBackpackWorkspaceAutomationTestView WorkspaceView(const UWacomBackpackScreen& Screen);
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

	static FText ZoneTitleText(const UWacomSpecialZoneWidget& Zone);
	static bool IsBattleReadyBadgeVisible(const UWacomSpecialZoneWidget& Zone);
	static bool RequestContentCardBattleEnabledToggle(const UWacomSpecialZoneWidget& Zone, int32 Index);
	static UWacomDeckCardWidget* OwnerCard(const UWacomSpecialZoneWidget& Zone);
	static UWacomDeckCardWidget* ContentCard(const UWacomSpecialZoneWidget& Zone, int32 Index);
	static int32 ContentCardCount(const UWacomSpecialZoneWidget& Zone);
};

#endif
