// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunFirstPersonCardDetailController.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardDetailPanelHost.h"

namespace
{
	FWacomFirstPersonCardDetailPanelHostContext BuildRunFirstPersonCardDetailPanelHostContext(
		AWacomPlayerController& PlayerController)
	{
		FWacomFirstPersonCardDetailPanelHostContext Context;
		Context.Outer = &PlayerController;
		Context.OwningPlayer = &PlayerController;
		Context.World = PlayerController.GetWorld();
		Context.PanelClass = PlayerController.RunFirstPersonCardDetailPanelClass;
		Context.ViewportZOrder = PlayerController.RunFirstPersonCardDetailViewportZOrder;
		Context.bCanAddToViewport = PlayerController.GetWorld() != nullptr;
		return Context;
	}
}

FWacomRunFirstPersonCardDetailController::FWacomRunFirstPersonCardDetailController(
	AWacomPlayerController& InPlayerController)
	: PlayerController(InPlayerController)
{
}

bool FWacomRunFirstPersonCardDetailController::IsVisible() const
{
	return MotionController.IsVisible(PlayerController.RunFirstPersonCardDetailPanel);
}

bool FWacomRunFirstPersonCardDetailController::IsPrewarmed() const
{
	return MotionController.IsPrewarmed(PlayerController.RunFirstPersonCardDetailPanel);
}

FText FWacomRunFirstPersonCardDetailController::GetNameText() const
{
	return MotionController.GetNameText(PlayerController.RunFirstPersonCardDetailPanel);
}

UWacomCardDetailPanel* FWacomRunFirstPersonCardDetailController::EnsurePanel()
{
	return FWacomFirstPersonCardDetailPanelHost::EnsurePanel(
		PlayerController.RunFirstPersonCardDetailPanel,
		BuildRunFirstPersonCardDetailPanelHostContext(PlayerController),
		MotionController,
		BuildMotionConfig());
}

void FWacomRunFirstPersonCardDetailController::PrewarmPanel()
{
	FWacomFirstPersonCardDetailPanelHost::PrewarmPanel(
		PlayerController.RunFirstPersonCardDetailPanel,
		BuildRunFirstPersonCardDetailPanelHostContext(PlayerController),
		MotionController,
		BuildMotionConfig());
}

void FWacomRunFirstPersonCardDetailController::RemovePanelFromViewport()
{
	FWacomFirstPersonCardDetailPanelHost::RemovePanelFromViewport(
		PlayerController.RunFirstPersonCardDetailPanel,
		MotionController);
	ClearCurrentSource();
	HoveredSourceId.Invalidate();
	HoveredSlot = FWacomFirstPersonCardLayerSlotView();
	bHasHoveredSlot = false;
}

void FWacomRunFirstPersonCardDetailController::RefreshBinding()
{
	UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	UWacomFirstPersonCardAnchorComponent* CurrentBoundAnchor = BoundAnchor.Get();
	const UWacomRunFirstPersonCardSourceComponent* Source =
		PlayerController.GetRunFirstPersonCardSourceComponent();

	const bool bShouldBind =
		Anchor
		&& Source
		&& Source->IsRunFirstPersonCardLayerActive()
		&& PlayerController.IsInExplorationFlow();

	if ((!bShouldBind || CurrentBoundAnchor != Anchor) && CurrentBoundAnchor)
	{
		UnbindBinding(CurrentBoundAnchor);
	}

	if (bShouldBind && Anchor && BoundAnchor.Get() != Anchor)
	{
		Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(&PlayerController);
		Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(&PlayerController);
		Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(&PlayerController);
		Anchor->OnFirstPersonCardLayerCardHovered.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerCardHovered);
		Anchor->OnFirstPersonCardLayerCardUnhovered.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerCardUnhovered);
		Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.AddUObject(
			&PlayerController,
			&AWacomPlayerController::HandleRunFirstPersonCardLayerHoveredCardLayoutUpdated);
		BoundAnchor = Anchor;
	}

	if (bShouldBind)
	{
		PrewarmPanel();
	}
}

void FWacomRunFirstPersonCardDetailController::UnbindBinding(
	UWacomFirstPersonCardAnchorComponent* Anchor)
{
	if (!Anchor)
	{
		return;
	}

	Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(&PlayerController);
	Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(&PlayerController);
	Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(&PlayerController);
	if (BoundAnchor.Get() == Anchor)
	{
		BoundAnchor.Reset();
	}
}

void FWacomRunFirstPersonCardDetailController::UnbindCurrentBinding()
{
	if (UWacomFirstPersonCardAnchorComponent* Anchor = BoundAnchor.Get())
	{
		UnbindBinding(Anchor);
		return;
	}
	BoundAnchor.Reset();
}

bool FWacomRunFirstPersonCardDetailController::CanReuseCurrentDetail(
	const FGuid& CardInstanceId) const
{
	return MotionController.CanReuseCurrentDetail(
		CardInstanceId,
		PlayerController.RunFirstPersonCardDetailPanel);
}

bool FWacomRunFirstPersonCardDetailController::ShowExistingAtSlot(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	EWacomRunFirstPersonCardDetailHoldReason HoldReason)
{
	if (!PlayerController.RunFirstPersonCardDetailPanel)
	{
		return false;
	}

	SetCurrentSource(CardInstanceId, HoldReason);
	return MotionController.ShowExistingAtSlot(
		*PlayerController.RunFirstPersonCardDetailPanel,
		CardInstanceId,
		SlotView,
		BuildMotionConfig(),
		GetViewportSize());
}

bool FWacomRunFirstPersonCardDetailController::ShowAtSlot(
	const FGuid& CardInstanceId,
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	EWacomRunFirstPersonCardDetailHoldReason HoldReason)
{
	UWacomCardDetailPanel* Panel = EnsurePanel();
	if (!Panel)
	{
		ForceHideAll();
		return false;
	}

	if (!Panel->IsInViewport() && PlayerController.GetWorld())
	{
		FWacomFirstPersonCardDetailPanelHost::AddPanelToViewportIfNeeded(
			*Panel,
			BuildRunFirstPersonCardDetailPanelHostContext(PlayerController));
	}

	SetCurrentSource(CardInstanceId, HoldReason);
	return MotionController.ShowAtSlot(
		*Panel,
		CardInstanceId,
		DetailData,
		SlotView,
		BuildMotionConfig(),
		GetViewportSize());
}

void FWacomRunFirstPersonCardDetailController::UpdateCurrentSlot(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	MotionController.UpdateCurrentSlot(
		CardInstanceId,
		SlotView,
		PlayerController.RunFirstPersonCardDetailPanel,
		BuildMotionConfig(),
		GetViewportSize());
}

void FWacomRunFirstPersonCardDetailController::HideForSource(
	const FGuid& CardInstanceId)
{
	if (!MotionController.IsMotionSource(CardInstanceId))
	{
		return;
	}

	ClearHoveredStateForSource(CardInstanceId);
	MotionController.HideForSource(
		CardInstanceId,
		PlayerController.RunFirstPersonCardDetailPanel,
		BuildMotionConfig());
	CurrentHoldReason = EWacomRunFirstPersonCardDetailHoldReason::None;
}

void FWacomRunFirstPersonCardDetailController::ForceHideAll()
{
	MotionController.ForceHideAll(PlayerController.RunFirstPersonCardDetailPanel);
	ClearCurrentSource();
	HoveredSourceId.Invalidate();
	HoveredSlot = FWacomFirstPersonCardLayerSlotView();
	bHasHoveredSlot = false;
}

void FWacomRunFirstPersonCardDetailController::TickMotion(float DeltaTime)
{
	MotionController.TickMotion(
		DeltaTime,
		PlayerController.RunFirstPersonCardDetailPanel,
		BuildMotionConfig(),
		GetViewportSize());
}

void FWacomRunFirstPersonCardDetailController::SetHoveredSlot(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	HoveredSourceId = CardInstanceId;
	HoveredSlot = SlotView;
	bHasHoveredSlot = CardInstanceId.IsValid() && SlotView.bProjected;
}

void FWacomRunFirstPersonCardDetailController::ClearHoveredStateForSource(
	const FGuid& CardInstanceId)
{
	if (CardInstanceId.IsValid() && HoveredSourceId != CardInstanceId)
	{
		return;
	}

	HoveredSourceId.Invalidate();
	HoveredSlot = FWacomFirstPersonCardLayerSlotView();
	bHasHoveredSlot = false;
}

bool FWacomRunFirstPersonCardDetailController::GetHoveredSlot(
	FGuid& OutCardInstanceId,
	FWacomFirstPersonCardLayerSlotView& OutSlotView) const
{
	if (!bHasHoveredSlot || !HoveredSourceId.IsValid())
	{
		return false;
	}

	OutCardInstanceId = HoveredSourceId;
	OutSlotView = HoveredSlot;
	return true;
}

bool FWacomRunFirstPersonCardDetailController::IsInspectHeldForSource(
	const FGuid& CardInstanceId) const
{
	return CurrentHoldReason == EWacomRunFirstPersonCardDetailHoldReason::Inspect
		&& (!CardInstanceId.IsValid() || MotionController.IsCurrentSource(CardInstanceId));
}

void FWacomRunFirstPersonCardDetailController::ClearInspectHold()
{
	if (CurrentHoldReason == EWacomRunFirstPersonCardDetailHoldReason::Inspect)
	{
		CurrentHoldReason = EWacomRunFirstPersonCardDetailHoldReason::None;
	}
}

void FWacomRunFirstPersonCardDetailController::HandleCardHovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	SetHoveredSlot(CardInstanceId, SlotView);

	if (IsInspectHeldForSource(FGuid()))
	{
		return;
	}

	ShowAtSlotFromRunData(
		CardInstanceId,
		SlotView,
		EWacomRunFirstPersonCardDetailHoldReason::Hover);
}

void FWacomRunFirstPersonCardDetailController::HandleCardUnhovered(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& /*SlotView*/)
{
	ClearHoveredStateForSource(CardInstanceId);
	if (IsInspectHeldForSource(CardInstanceId))
	{
		return;
	}

	HideForSource(CardInstanceId);
}

void FWacomRunFirstPersonCardDetailController::HandleHoveredCardLayoutUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	SetHoveredSlot(CardInstanceId, SlotView);

	if (!CardInstanceId.IsValid()
		|| !SlotView.bProjected
		|| !ShouldHandleCurrentSource()
		|| IsInspectHeldForSource(FGuid()))
	{
		return;
	}

	UpdateCurrentSlot(CardInstanceId, SlotView);
}

bool FWacomRunFirstPersonCardDetailController::HandleInspectDragStartedOrUpdated(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (DragView.GestureState != EWacomFirstPersonCardGestureState::Inspecting)
	{
		return false;
	}

	ShowInspectDetail(CardInstanceId, DragView);
	return true;
}

void FWacomRunFirstPersonCardDetailController::FinishInspectDetail(
	const FGuid& CardInstanceId)
{
	if (!IsInspectHeldForSource(CardInstanceId))
	{
		return;
	}

	ClearInspectHold();

	FGuid HoveredCardInstanceId;
	FWacomFirstPersonCardLayerSlotView HoveredSlotView;
	if (GetHoveredSlot(HoveredCardInstanceId, HoveredSlotView))
	{
		ShowAtSlotFromRunData(
			HoveredCardInstanceId,
			HoveredSlotView,
			EWacomRunFirstPersonCardDetailHoldReason::Hover);
		return;
	}

	HideForSource(CardInstanceId);
}

void FWacomRunFirstPersonCardDetailController::SetCurrentSource(
	const FGuid& CardInstanceId,
	EWacomRunFirstPersonCardDetailHoldReason HoldReason)
{
	MotionController.SetCurrentSource(CardInstanceId);
	CurrentHoldReason = HoldReason;
}

void FWacomRunFirstPersonCardDetailController::ClearCurrentSource()
{
	MotionController.ClearCurrentSource();
	CurrentHoldReason = EWacomRunFirstPersonCardDetailHoldReason::None;
}

bool FWacomRunFirstPersonCardDetailController::ShouldHandleCurrentSource() const
{
	const UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	const UWacomRunFirstPersonCardSourceComponent* Source =
		PlayerController.GetRunFirstPersonCardSourceComponent();
	if (!Anchor
		|| !PlayerController.IsInExplorationFlow()
		|| !Source
		|| !Source->IsRunFirstPersonCardLayerActive())
	{
		return false;
	}

	return Source->CanHandleRunFirstPersonCardLayerSource(
		Anchor->GetRuntimeCardLayerSourceId());
}

bool FWacomRunFirstPersonCardDetailController::ShowAtSlotFromRunData(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	EWacomRunFirstPersonCardDetailHoldReason HoldReason)
{
	if (!ShouldHandleCurrentSource()
		|| !CardInstanceId.IsValid()
		|| !SlotView.bProjected)
	{
		ForceHideAll();
		return false;
	}

	if (CanReuseCurrentDetail(CardInstanceId)
		&& ShowExistingAtSlot(CardInstanceId, SlotView, HoldReason))
	{
		return true;
	}

	FWacomCardDetailViewData DetailData;
	if (!PlayerController.BuildRunFirstPersonCardDetailViewData(CardInstanceId, DetailData))
	{
		ForceHideAll();
		return false;
	}

	return ShowAtSlot(CardInstanceId, DetailData, SlotView, HoldReason);
}

bool FWacomRunFirstPersonCardDetailController::ShouldShowInspectDetail(
	const FWacomFirstPersonCardDragView& DragView) const
{
	const UWacomFirstPersonCardAnchorComponent* Anchor =
		PlayerController.ResolveFirstPersonCardAnchorForRunMenuProbe();
	return DragView.GestureState == EWacomFirstPersonCardGestureState::Inspecting
		&& Anchor
		&& Anchor->bShowDetailDuringCardInspect
		&& ShouldHandleCurrentSource()
		&& DragView.CardInstanceId.IsValid()
		&& DragView.SourceSlotView.bProjected;
}

bool FWacomRunFirstPersonCardDetailController::ShowInspectDetail(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardDragView& DragView)
{
	if (!ShouldShowInspectDetail(DragView))
	{
		HideForSource(CardInstanceId);
		return false;
	}

	return ShowAtSlotFromRunData(
		CardInstanceId,
		DragView.SourceSlotView,
		EWacomRunFirstPersonCardDetailHoldReason::Inspect);
}

FWacomFirstPersonCardDetailMotionConfig
FWacomRunFirstPersonCardDetailController::BuildMotionConfig() const
{
	FWacomFirstPersonCardDetailMotionConfig Config;
	Config.bEnableReadabilityPolish =
		PlayerController.bEnableRunFirstPersonCardDetailReadabilityPolish;
	Config.PanelEstimatedSize = PlayerController.RunFirstPersonCardDetailPanelEstimatedSize;
	Config.DetailPadding = PlayerController.RunFirstPersonCardDetailPanelPadding;
	Config.AnchorBaseSize = PlayerController.RunFirstPersonCardDetailAnchorBaseSize;
	Config.HoverDelaySeconds = PlayerController.RunFirstPersonCardDetailHoverDelaySeconds;
	Config.FadeInSpeed = PlayerController.RunFirstPersonCardDetailFadeInSpeed;
	Config.FadeOutSpeed = PlayerController.RunFirstPersonCardDetailFadeOutSpeed;
	Config.FollowSpeed = PlayerController.RunFirstPersonCardDetailFollowSpeed;
	Config.PositionResetDistancePixels = 420.0f;
	Config.AppearStartScale = PlayerController.RunFirstPersonCardDetailAppearStartScale;
	Config.SideSwitchHysteresisPixels =
		PlayerController.RunFirstPersonCardDetailSideSwitchHysteresisPixels;
	return Config;
}

FVector2D FWacomRunFirstPersonCardDetailController::GetViewportSize() const
{
	return FWacomFirstPersonCardDetailPanelHost::GetViewportSize(
		BuildRunFirstPersonCardDetailPanelHostContext(PlayerController));
}
