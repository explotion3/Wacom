// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCardDetailController.h"

#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardDetailPanelHost.h"

#include "GameFramework/PlayerController.h"

FWacomBattleHUDCardDetailController::FWacomBattleHUDCardDetailController(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

bool FWacomBattleHUDCardDetailController::IsVisible() const
{
	return MotionController.IsVisible(Runtime.Host().GetFirstPersonCardDetailPanelSlot());
}

FText FWacomBattleHUDCardDetailController::GetNameText() const
{
	return MotionController.GetNameText(Runtime.Host().GetFirstPersonCardDetailPanelSlot());
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
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
		BuildMotionConfig());
}

UWacomCardDetailPanel* FWacomBattleHUDCardDetailController::EnsureFirstPersonPanel()
{
	return FWacomFirstPersonCardDetailPanelHost::EnsurePanel(
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
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
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::HideFirstPerson()
{
	MotionController.HideForSource(
		MotionController.GetCurrentSource(),
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
		BuildMotionConfig());
}

void FWacomBattleHUDCardDetailController::TickMotion(float DeltaTime)
{
	MotionController.TickMotion(
		DeltaTime,
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::ForceHideHost(EWacomBattleHUDCardDetailHost Host)
{
	if (Host != EWacomBattleHUDCardDetailHost::FirstPersonViewport)
	{
		return;
	}

	MotionController.ForceHideAll(Runtime.Host().GetFirstPersonCardDetailPanelSlot());
}

void FWacomBattleHUDCardDetailController::ForceHideAll()
{
	MotionController.ForceHideAll(Runtime.Host().GetFirstPersonCardDetailPanelSlot());
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
		Runtime.Host().GetCardDetailSideSwitchHysteresisPixels());
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
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
		BuildMotionConfig(),
		GetFirstPersonViewportSize());
}

void FWacomBattleHUDCardDetailController::RemoveFirstPersonPanelFromViewport()
{
	FWacomFirstPersonCardDetailPanelHost::RemovePanelFromViewport(
		Runtime.Host().GetFirstPersonCardDetailPanelSlot(),
		MotionController);
}

FWacomFirstPersonCardDetailMotionConfig
FWacomBattleHUDCardDetailController::BuildMotionConfig() const
{
	FWacomFirstPersonCardDetailMotionConfig Config;
	Config.bEnableReadabilityPolish = Runtime.Host().IsCardDetailReadabilityPolishEnabled();
	Config.PanelEstimatedSize = Runtime.Host().GetCardDetailPanelEstimatedSize();
	Config.DetailPadding = Runtime.Host().GetCardDetailPanelPadding();
	Config.AnchorBaseSize = Runtime.Host().GetFirstPersonCardDetailAnchorBaseSize();
	Config.HoverDelaySeconds = Runtime.Host().GetCardDetailHoverDelaySeconds();
	Config.FadeInSpeed = Runtime.Host().GetCardDetailFadeInSpeed();
	Config.FadeOutSpeed = Runtime.Host().GetCardDetailFadeOutSpeed();
	Config.FollowSpeed = Runtime.Host().GetCardDetailFollowSpeed();
	Config.PositionResetDistancePixels = Runtime.Host().GetCardDetailPositionResetDistancePixels();
	Config.AppearStartScale = Runtime.Host().GetCardDetailAppearStartScale();
	Config.SideSwitchHysteresisPixels = Runtime.Host().GetCardDetailSideSwitchHysteresisPixels();
	return Config;
}

FWacomFirstPersonCardDetailPanelHostContext
FWacomBattleHUDCardDetailController::BuildPanelHostContext() const
{
	FWacomFirstPersonCardDetailPanelHostContext Context;
	Context.Outer = Runtime.Host().AsObject();
	Context.OwningPlayer = Runtime.GetOwningPlayer();
	Context.World = Runtime.GetWorld();
	Context.PanelClass = Runtime.Host().GetCardDetailPanelClass();
	Context.ViewportZOrder = Runtime.Host().GetFirstPersonCardDetailViewportZOrder();
	Context.bCanAddToViewport = Context.OwningPlayer
		&& Context.OwningPlayer->IsLocalController()
		&& Context.OwningPlayer->GetLocalPlayer();
	return Context;
}
