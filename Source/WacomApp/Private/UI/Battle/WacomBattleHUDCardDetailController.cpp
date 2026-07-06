// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCardDetailController.h"

#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardDetailPanelHost.h"

FWacomBattleHUDCardDetailController::FWacomBattleHUDCardDetailController(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

bool FWacomBattleHUDCardDetailController::IsVisible() const
{
	return MotionController.IsVisible(HUD.FirstPersonCardDetailPanel);
}

FText FWacomBattleHUDCardDetailController::GetNameText() const
{
	return MotionController.GetNameText(HUD.FirstPersonCardDetailPanel);
}

void FWacomBattleHUDCardDetailController::HideAll()
{
	ForceHideAll();
}

void FWacomBattleHUDCardDetailController::HideFirstPersonForSource(
	const FGuid& CardInstanceId)
{
	MotionController.HideForSource(
		CardInstanceId,
		HUD.FirstPersonCardDetailPanel,
		BuildMotionConfig());
}

bool FWacomBattleHUDCardDetailController::IsFirstPersonInspectDetailActiveForSource(
	const FGuid& CardInstanceId) const
{
	return MotionController.IsActiveSlotInspectingForSource(CardInstanceId);
}

UWacomCardDetailPanel* FWacomBattleHUDCardDetailController::EnsureFirstPersonPanel()
{
	return FWacomFirstPersonCardDetailPanelHost::EnsurePanel(
		HUD.FirstPersonCardDetailPanel,
		BuildPanelHostContext(),
		MotionController,
		BuildMotionConfig());
}

bool FWacomBattleHUDCardDetailController::ShowFirstPersonAtSlot(
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	UWacomCardDetailPanel* Panel = EnsureFirstPersonPanel();
	if (!Panel)
	{
		return false;
	}

	if (!Panel->IsInViewport())
	{
		FWacomFirstPersonCardDetailPanelHost::AddPanelToViewportIfNeeded(
			*Panel,
			BuildPanelHostContext());
	}

	return MotionController.ShowAtSlot(
		*Panel,
		MotionController.GetCurrentSource(),
		DetailData,
		SlotView,
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::PositionFirstPersonBesideSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	MotionController.UpdateCurrentSlot(
		MotionController.GetCurrentSource(),
		SlotView,
		HUD.FirstPersonCardDetailPanel,
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::HideFirstPerson()
{
	MotionController.HideForSource(
		MotionController.GetCurrentSource(),
		HUD.FirstPersonCardDetailPanel,
		BuildMotionConfig());
}

void FWacomBattleHUDCardDetailController::TickMotion(float DeltaTime)
{
	MotionController.TickMotion(
		DeltaTime,
		HUD.FirstPersonCardDetailPanel,
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::ForceHideHost(UBattleHUD::ECardDetailHost Host)
{
	if (Host != UBattleHUD::ECardDetailHost::FirstPersonViewport)
	{
		return;
	}

	MotionController.ForceHideAll(HUD.FirstPersonCardDetailPanel);
}

void FWacomBattleHUDCardDetailController::ForceHideAll()
{
	MotionController.ForceHideAll(HUD.FirstPersonCardDetailPanel);
}

bool FWacomBattleHUDCardDetailController::ComputeFirstPersonTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	FVector2D& OutPosition)
{
	return MotionController.ComputeTarget(
		SlotView,
		BuildMotionConfig(),
		GetFirstPersonViewportSize(),
		OutPosition);
}

FVector2D FWacomBattleHUDCardDetailController::ComputeStablePosition(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding)
{
	return MotionController.ComputeStablePosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		PanelSize,
		DetailPadding,
		HUD.CardDetailSideSwitchHysteresisPixels);
}

FVector2D FWacomBattleHUDCardDetailController::GetFirstPersonViewportSize() const
{
	return FWacomFirstPersonCardDetailPanelHost::GetViewportSize(
		BuildPanelHostContext());
}

void FWacomBattleHUDCardDetailController::SetFirstPersonSource(
	const FGuid& CardInstanceId)
{
	MotionController.SetCurrentSource(CardInstanceId);
}

void FWacomBattleHUDCardDetailController::ClearFirstPersonSource()
{
	MotionController.ClearCurrentSource();
}

bool FWacomBattleHUDCardDetailController::IsCurrentFirstPersonSource(
	const FGuid& CardInstanceId) const
{
	return MotionController.IsCurrentSource(CardInstanceId);
}

void FWacomBattleHUDCardDetailController::UpdateFirstPersonSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	MotionController.UpdateCurrentSlot(
		MotionController.GetCurrentSource(),
		SlotView,
		HUD.FirstPersonCardDetailPanel,
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::RemoveFirstPersonPanelFromViewport()
{
	FWacomFirstPersonCardDetailPanelHost::RemovePanelFromViewport(
		HUD.FirstPersonCardDetailPanel,
		MotionController);
}

FWacomFirstPersonCardDetailMotionConfig
FWacomBattleHUDCardDetailController::BuildMotionConfig() const
{
	FWacomFirstPersonCardDetailMotionConfig Config;
	Config.bEnableReadabilityPolish = HUD.bEnableCardDetailReadabilityPolish;
	Config.PanelEstimatedSize = HUD.CardDetailPanelEstimatedSize;
	Config.DetailPadding = HUD.CardDetailPanelPadding;
	Config.AnchorBaseSize = HUD.FirstPersonCardDetailAnchorBaseSize;
	Config.HoverDelaySeconds = HUD.CardDetailHoverDelaySeconds;
	Config.FadeInSpeed = HUD.CardDetailFadeInSpeed;
	Config.FadeOutSpeed = HUD.CardDetailFadeOutSpeed;
	Config.FollowSpeed = HUD.CardDetailFollowSpeed;
	Config.PositionResetDistancePixels = HUD.CardDetailPositionResetDistancePixels;
	Config.AppearStartScale = HUD.CardDetailAppearStartScale;
	Config.SideSwitchHysteresisPixels = HUD.CardDetailSideSwitchHysteresisPixels;
	return Config;
}

FWacomFirstPersonCardDetailPanelHostContext
FWacomBattleHUDCardDetailController::BuildPanelHostContext() const
{
	FWacomFirstPersonCardDetailPanelHostContext Context;
	Context.Outer = &HUD;
	Context.OwningPlayer = HUD.GetOwningPlayer();
	Context.World = HUD.GetWorld();
	Context.PanelClass = HUD.CardDetailPanelClass;
	Context.ViewportZOrder = HUD.FirstPersonCardDetailViewportZOrder;
	Context.bCanAddToViewport = true;
	return Context;
}
