// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UDragDropOperation;
class URunSession;
class UWacomCardDragOperation;
class UWacomBackpackScreen;
class UWacomDeckCardWidget;
class UWacomSpecialZoneWidget;
enum class EZoneKind : uint8;
struct FWacomBackpackScreenAutomationTestView;
struct FWacomBackpackWorkspaceAutomationTestView;

struct FWacomBackpackScreenTestAccess
{
	static UWacomBackpackScreen* Create(UObject* Outer, URunSession* RunSession);
	static void Refresh(UWacomBackpackScreen& Screen);
	static void SetRunSession(UWacomBackpackScreen& Screen, URunSession* RunSession);
	static FWacomBackpackScreenAutomationTestView View(const UWacomBackpackScreen& Screen);
	static FGuid ResolveDeleteRequestInstanceId(const UWacomCardDragOperation& CardOp);
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
	static EZoneKind ActiveWorkspaceZone(const UWacomBackpackScreen& Screen);
	static FGuid ActiveWorkspaceOwnerInstanceId(const UWacomBackpackScreen& Screen);
	static void ActivateZone(UWacomBackpackScreen& Screen, EZoneKind Zone, FGuid OwnerInstanceId = FGuid());
	static bool BeginWorkspaceCarry(UWacomBackpackScreen& Screen, int32 CardIndex = 0);
	static bool BeginWorkspaceCarryForIds(UWacomBackpackScreen& Screen, TConstArrayView<FGuid> InstanceIds);
	static void DeactivateWorkspaceScreen(UWacomBackpackScreen& Screen);
	static FWacomBackpackWorkspaceAutomationTestView WorkspaceView(const UWacomBackpackScreen& Screen);
	static bool BeginDeleteConfirmation(UWacomBackpackScreen& Screen);
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
	static UDragDropOperation* BuildOwnerCardDragOperation(const UWacomSpecialZoneWidget& Zone);
	static UDragDropOperation* BuildContentCardDragOperation(const UWacomSpecialZoneWidget& Zone, int32 Index);
	static bool RequestContentCardBattleEnabledToggle(const UWacomSpecialZoneWidget& Zone, int32 Index);
	static UWacomDeckCardWidget* OwnerCard(const UWacomSpecialZoneWidget& Zone);
	static UWacomDeckCardWidget* ContentCard(const UWacomSpecialZoneWidget& Zone, int32 Index);
	static int32 ContentCardCount(const UWacomSpecialZoneWidget& Zone);
};

#endif
