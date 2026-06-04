// Copyright Wacom. All Rights Reserved.

#include "BackpackScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"

UWacomBackpackScreen* FWacomBackpackScreenTestAccess::Create(UObject* Outer, URunSession* RunSession)
{
	UWacomBackpackScreen* Screen = NewObject<UWacomBackpackScreen>(Outer);
	SetRunSession(*Screen, RunSession);
	Screen->TakeWidget();
	Refresh(*Screen);
	return Screen;
}

void FWacomBackpackScreenTestAccess::Refresh(UWacomBackpackScreen& Screen)
{
	Screen.RebuildAllForTest();
}

void FWacomBackpackScreenTestAccess::SetRunSession(UWacomBackpackScreen& Screen, URunSession* RunSession)
{
	Screen.SetRunSessionForTest(RunSession);
}

FWacomBackpackScreenAutomationTestView FWacomBackpackScreenTestAccess::View(const UWacomBackpackScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::BattleDeckCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetBattleDeckCardWidgetForTest(Index);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::FluxContentCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetFluxContentCardWidgetForTest(Index);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::BurdenCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetBurdenCardWidgetForTest(Index);
}

UWacomSpecialZoneWidget* FWacomBackpackScreenTestAccess::SpecialZone(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetSpecialZoneWidgetForTest(Index);
}

int32 FWacomBackpackScreenTestAccess::RefreshApplyCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ListRefreshApplyCount;
}

int32 FWacomBackpackScreenTestAccess::RefreshSkipCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ListRefreshSkipCount;
}

int32 FWacomBackpackScreenTestAccess::SnapshotBuildCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).SnapshotBuildCount;
}

int32 FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).SnapshotRevisionSkipCount;
}

bool FWacomBackpackScreenTestAccess::IsDetailVisible(const UWacomBackpackScreen& Screen)
{
	return Screen.IsCardDetailPanelVisible();
}

FText FWacomBackpackScreenTestAccess::DetailNameText(const UWacomBackpackScreen& Screen)
{
	return Screen.GetCardDetailPanelNameText();
}

bool FWacomBackpackScreenTestAccess::ShowDetailForCardWidget(
	UWacomBackpackScreen& Screen,
	UWacomDeckCardWidget* SourceWidget)
{
	return Screen.ShowCardDetailForCardWidget(SourceWidget);
}

void FWacomBackpackScreenTestAccess::HideDetail(UWacomBackpackScreen& Screen)
{
	Screen.HideCardDetailPanel();
}

FText FWacomBackpackScreenTestAccess::ZoneTitleText(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.GetZoneTitleTextForTest();
}

bool FWacomBackpackScreenTestAccess::IsBattleReadyBadgeVisible(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.IsBattleReadyBadgeVisibleForTest();
}

UDragDropOperation* FWacomBackpackScreenTestAccess::BuildOwnerCardDragOperation(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.BuildOwnerCardDragOperationForTest();
}

UDragDropOperation* FWacomBackpackScreenTestAccess::BuildContentCardDragOperation(
	const UWacomSpecialZoneWidget& Zone,
	int32 Index)
{
	return Zone.BuildContentCardDragOperationForTest(Index);
}

bool FWacomBackpackScreenTestAccess::RequestContentCardBattleEnabledToggle(
	const UWacomSpecialZoneWidget& Zone,
	int32 Index)
{
	return Zone.RequestContentCardBattleEnabledToggleForTest(Index);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::OwnerCard(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.GetOwnerCardWidgetForTest();
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::ContentCard(const UWacomSpecialZoneWidget& Zone, int32 Index)
{
	return Zone.GetContentCardWidgetForTest(Index);
}

int32 FWacomBackpackScreenTestAccess::ContentCardCount(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.GetContentCardWidgetCountForTest();
}

#endif
