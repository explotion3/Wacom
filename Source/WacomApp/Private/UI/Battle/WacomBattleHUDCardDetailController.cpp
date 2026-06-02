// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCardDetailController.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

FWacomBattleHUDCardDetailController::FWacomBattleHUDCardDetailController(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

bool FWacomBattleHUDCardDetailController::IsVisible() const
{
	const bool bLegacyVisible =
		HUD.CardDetailPanel && HUD.CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bFirstPersonVisible =
		HUD.FirstPersonCardDetailPanel
		&& HUD.FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
	return bLegacyVisible || bFirstPersonVisible;
}

FText FWacomBattleHUDCardDetailController::GetNameText() const
{
	if (HUD.FirstPersonCardDetailPanel
		&& HUD.FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return HUD.FirstPersonCardDetailPanel->GetNameText();
	}
	return HUD.CardDetailPanel ? HUD.CardDetailPanel->GetNameText() : FText::GetEmpty();
}

void FWacomBattleHUDCardDetailController::HandleHandCardHovered(UCardWidget* SourceWidget)
{
	ShowForCardWidget(SourceWidget);
}

void FWacomBattleHUDCardDetailController::HandleHandCardUnhovered(UCardWidget* SourceWidget)
{
	HideLegacyForSource(SourceWidget);
}

bool FWacomBattleHUDCardDetailController::ShowForCardWidget(UCardWidget* SourceWidget)
{
	if (!SourceWidget || !SourceWidget->GetCardSnapshot().Definition || HUD.UIState != EBattleUIState::Idle)
	{
		HideAll();
		return false;
	}

	EnsureLayer();
	if (!HUD.CardDetailLayer)
	{
		return false;
	}

	const FGeometry& LayerGeometry = HUD.CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	CurrentLegacySource = SourceWidget;
	if (ShowAtAnchor(
		UWacomCardPresentationBuilder::BuildCardDetailViewData(SourceWidget->GetCardSnapshot().Definition),
		AnchorPosition,
		AnchorSize))
	{
		CurrentFirstPersonSourceId.Invalidate();
		ForceHideHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		return true;
	}
	CurrentLegacySource.Reset();
	return false;
}

bool FWacomBattleHUDCardDetailController::ShowAtAnchor(
	const FWacomCardDetailViewData& DetailData,
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize)
{
	UWacomCardDetailPanel* Panel = EnsureLegacyPanel();
	if (!Panel)
	{
		return false;
	}

	Panel->SetCardDetailData(DetailData);
	PositionLegacyBesideAnchor(AnchorPosition, AnchorSize);
	if (!HUD.bEnableCardDetailReadabilityPolish)
	{
		Panel->SetRenderOpacity(1.0f);
		Panel->SetRenderTransform(FWidgetTransform());
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		return true;
	}

	Panel->SetIsEnabled(true);
	if (!BeginMotionShow(UBattleHUD::ECardDetailHost::LegacyHandPanel))
	{
		ForceHideHost(UBattleHUD::ECardDetailHost::LegacyHandPanel);
		return false;
	}
	return true;
}

void FWacomBattleHUDCardDetailController::HideAll()
{
	ForceHideAll();
}

void FWacomBattleHUDCardDetailController::HideLegacyForSource(UCardWidget* SourceWidget)
{
	if (!IsMotionSource(UBattleHUD::ECardDetailHost::LegacyHandPanel, SourceWidget))
	{
		return;
	}
	RequestMotionHide(
		UBattleHUD::ECardDetailHost::LegacyHandPanel,
		!HUD.bEnableCardDetailReadabilityPolish);
}

void FWacomBattleHUDCardDetailController::HideFirstPersonForSource(const FGuid& CardInstanceId)
{
	if (!IsMotionSource(UBattleHUD::ECardDetailHost::FirstPersonViewport, CardInstanceId))
	{
		return;
	}
	RequestMotionHide(
		UBattleHUD::ECardDetailHost::FirstPersonViewport,
		!HUD.bEnableCardDetailReadabilityPolish);
}

bool FWacomBattleHUDCardDetailController::IsFirstPersonInspectDetailActiveForSource(
	const FGuid& CardInstanceId) const
{
	if (!CardInstanceId.IsValid()
		|| CurrentFirstPersonSourceId != CardInstanceId
		|| MotionState.ActiveHost != UBattleHUD::ECardDetailHost::FirstPersonViewport
		|| MotionState.ActiveFirstPersonSourceId != CardInstanceId
		|| !MotionState.bHasActiveFirstPersonSlot)
	{
		return false;
	}

	return MotionState.ActiveFirstPersonSlot.GestureState == EWacomFirstPersonCardGestureState::Inspecting;
}

UWacomCardDetailPanel* FWacomBattleHUDCardDetailController::EnsureLegacyPanel()
{
	EnsureLayer();
	if (!HUD.CardDetailLayer)
	{
		return nullptr;
	}

	if (HUD.CardDetailPanel)
	{
		return HUD.CardDetailPanel;
	}

	UClass* PanelClass = HUD.CardDetailPanelClass
		? HUD.CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	if (APlayerController* OwningPlayer = HUD.GetOwningPlayer();
		OwningPlayer && OwningPlayer->IsLocalController() && OwningPlayer->GetLocalPlayer())
	{
		HUD.CardDetailPanel = CreateWidget<UWacomCardDetailPanel>(OwningPlayer, PanelClass);
	}
	if (!HUD.CardDetailPanel)
	{
		if (UWorld* World = HUD.GetWorld())
		{
			HUD.CardDetailPanel = CreateWidget<UWacomCardDetailPanel>(World, PanelClass);
		}
	}
	if (!HUD.CardDetailPanel)
	{
		HUD.CardDetailPanel = NewObject<UWacomCardDetailPanel>(&HUD, PanelClass);
	}
	if (!HUD.CardDetailPanel)
	{
		return nullptr;
	}

	HUD.CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	HUD.CardDetailPanel->SetIsEnabled(true);
	HUD.CardDetailPanel->SetRenderOpacity(1.0f);
	HUD.CardDetailPanel->SetRenderTransform(FWidgetTransform());
	if (UCanvasPanelSlot* DetailSlot = HUD.CardDetailLayer->AddChildToCanvas(HUD.CardDetailPanel))
	{
		DetailSlot->SetAutoSize(false);
		DetailSlot->SetSize(HUD.CardDetailPanelEstimatedSize);
		DetailSlot->SetZOrder(1);
	}
	return HUD.CardDetailPanel;
}

UWacomCardDetailPanel* FWacomBattleHUDCardDetailController::EnsureFirstPersonPanel()
{
	if (HUD.FirstPersonCardDetailPanel)
	{
		return HUD.FirstPersonCardDetailPanel;
	}

	UClass* PanelClass = HUD.CardDetailPanelClass
		? HUD.CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	if (APlayerController* OwningPlayer = HUD.GetOwningPlayer();
		OwningPlayer && OwningPlayer->IsLocalController() && OwningPlayer->GetLocalPlayer())
	{
		HUD.FirstPersonCardDetailPanel = CreateWidget<UWacomCardDetailPanel>(OwningPlayer, PanelClass);
	}
	if (!HUD.FirstPersonCardDetailPanel)
	{
		if (UWorld* World = HUD.GetWorld())
		{
			HUD.FirstPersonCardDetailPanel = CreateWidget<UWacomCardDetailPanel>(World, PanelClass);
		}
	}
	if (!HUD.FirstPersonCardDetailPanel)
	{
		HUD.FirstPersonCardDetailPanel = NewObject<UWacomCardDetailPanel>(&HUD, PanelClass);
	}
	if (!HUD.FirstPersonCardDetailPanel)
	{
		return nullptr;
	}

	HUD.FirstPersonCardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	HUD.FirstPersonCardDetailPanel->SetIsEnabled(true);
	HUD.FirstPersonCardDetailPanel->SetRenderOpacity(1.0f);
	HUD.FirstPersonCardDetailPanel->SetRenderTransform(FWidgetTransform());
	HUD.FirstPersonCardDetailPanel->AddToViewport(HUD.FirstPersonCardDetailViewportZOrder);
	return HUD.FirstPersonCardDetailPanel;
}

void FWacomBattleHUDCardDetailController::EnsureLayer()
{
	if (HUD.CardDetailLayer)
	{
		return;
	}

	if (UCanvasPanel* RootCanvas = HUD.WidgetTree ? Cast<UCanvasPanel>(HUD.WidgetTree->RootWidget) : nullptr)
	{
		HUD.CardDetailLayer = HUD.WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("CardDetailLayer_Runtime"));
		HUD.CardDetailLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* DetailLayerSlot = RootCanvas->AddChildToCanvas(HUD.CardDetailLayer))
		{
			DetailLayerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			DetailLayerSlot->SetOffsets(FMargin(0.0f));
			DetailLayerSlot->SetAutoSize(false);
			DetailLayerSlot->SetZOrder(10);
		}
	}
	else
	{
		if (!bLoggedMissingCardDetailLayer)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BattleHUD] CardDetailLayer 未绑定，且 RootWidget 不是 CanvasPanel，战斗手牌悬浮详情不会显示"));
			bLoggedMissingCardDetailLayer = true;
		}
	}
}

void FWacomBattleHUDCardDetailController::PositionLegacyNear(UCardWidget* SourceWidget)
{
	if (!SourceWidget || !HUD.CardDetailLayer || !HUD.CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = HUD.CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	PositionLegacyBesideAnchor(AnchorPosition, AnchorSize);
}

void FWacomBattleHUDCardDetailController::PositionLegacyBesideAnchor(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize)
{
	if (!HUD.CardDetailLayer || !HUD.CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = HUD.CardDetailLayer->GetCachedGeometry();
	const FVector2D LayerSize = LayerGeometry.GetLocalSize();
	const FVector2D Position = HUD.bEnableCardDetailReadabilityPolish
		? ComputeStablePosition(
			AnchorPosition,
			AnchorSize,
			LayerSize,
			HUD.CardDetailPanelEstimatedSize,
			HUD.CardDetailPanelPadding)
		: UBattleHUD::ComputeCardDetailPanelPositionBeside(
			AnchorPosition,
			AnchorSize,
			LayerSize,
			HUD.CardDetailPanelEstimatedSize,
			HUD.CardDetailPanelPadding);

	if (HUD.bEnableCardDetailReadabilityPolish)
	{
		MotionState.TargetPosition = Position;
		MotionState.bHasTargetPosition = true;
		return;
	}

	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(HUD.CardDetailPanel->Slot))
	{
		DetailSlot->SetPosition(Position);
		DetailSlot->SetSize(HUD.CardDetailPanelEstimatedSize);
	}
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
		Panel->AddToViewport(HUD.FirstPersonCardDetailViewportZOrder);
	}

	Panel->SetCardDetailData(DetailData);
	PositionFirstPersonBesideSlot(SlotView);
	Panel->SetDesiredSizeInViewport(HUD.CardDetailPanelEstimatedSize);
	Panel->SetIsEnabled(true);
	MotionState.ActiveFirstPersonSlot = SlotView;
	MotionState.bHasActiveFirstPersonSlot = true;
	if (!HUD.bEnableCardDetailReadabilityPolish)
	{
		Panel->SetRenderOpacity(1.0f);
		Panel->SetRenderTransform(FWidgetTransform());
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		return true;
	}

	if (!BeginMotionShow(UBattleHUD::ECardDetailHost::FirstPersonViewport))
	{
		ForceHideHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		return false;
	}
	return true;
}

void FWacomBattleHUDCardDetailController::PositionFirstPersonBesideSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	if (!HUD.FirstPersonCardDetailPanel)
	{
		return;
	}

	const FVector2D AnchorSize =
		HUD.FirstPersonCardDetailAnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	const FVector2D Position = HUD.bEnableCardDetailReadabilityPolish
		? ComputeStablePosition(
			AnchorPosition,
			AnchorSize,
			GetFirstPersonViewportSize(),
			HUD.CardDetailPanelEstimatedSize,
			HUD.CardDetailPanelPadding)
		: UBattleHUD::ComputeCardDetailPanelPositionBeside(
			AnchorPosition,
			AnchorSize,
			GetFirstPersonViewportSize(),
			HUD.CardDetailPanelEstimatedSize,
			HUD.CardDetailPanelPadding);

	HUD.FirstPersonCardDetailPanel->SetDesiredSizeInViewport(HUD.CardDetailPanelEstimatedSize);
	HUD.FirstPersonCardDetailPanel->SetAlignmentInViewport(FVector2D::ZeroVector);
	if (HUD.bEnableCardDetailReadabilityPolish)
	{
		MotionState.TargetPosition = Position;
		MotionState.bHasTargetPosition = true;
		return;
	}

	HUD.FirstPersonCardDetailPanel->SetPositionInViewport(Position, false);
	LastFirstPersonCardDetailPanelPosition = Position;
}

void FWacomBattleHUDCardDetailController::HideFirstPerson()
{
	RequestMotionHide(
		UBattleHUD::ECardDetailHost::FirstPersonViewport,
		!HUD.bEnableCardDetailReadabilityPolish);
}

void FWacomBattleHUDCardDetailController::TickMotion(float DeltaTime)
{
	if (!HUD.bEnableCardDetailReadabilityPolish)
	{
		return;
	}

	FCardDetailMotionState& State = MotionState;
	if (State.ActiveHost == UBattleHUD::ECardDetailHost::None && !State.bPendingShow)
	{
		return;
	}

	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	if (State.bPendingShow)
	{
		State.PendingElapsedSeconds += SafeDeltaTime;
		UpdateMotionTarget(State.ActiveHost);
		if (State.PendingElapsedSeconds < FMath::Max(0.0f, HUD.CardDetailHoverDelaySeconds))
		{
			return;
		}

		State.bPendingShow = false;
		State.bWantsVisible = true;
		State.VisualOpacity = 0.0f;
		State.bResetPosition = true;
		if (UWacomCardDetailPanel* Panel = GetPanelForHost(State.ActiveHost))
		{
			Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
			Panel->SetRenderOpacity(0.0f);
		}
	}

	if (!UpdateMotionTarget(State.ActiveHost))
	{
		ForceHideHost(State.ActiveHost);
		return;
	}

	UWacomCardDetailPanel* Panel = GetPanelForHost(State.ActiveHost);
	if (!Panel)
	{
		ForceHideHost(State.ActiveHost);
		return;
	}

	if (State.bWantsVisible && Panel->GetVisibility() == ESlateVisibility::Collapsed)
	{
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const FVector2D TargetPosition = State.bHasTargetPosition ? State.TargetPosition : FVector2D::ZeroVector;
	if (!State.bHasVisualPosition
		|| State.bResetPosition
		|| FVector2D::Distance(State.VisualPosition, TargetPosition)
			> FMath::Max(0.0f, HUD.CardDetailPositionResetDistancePixels))
	{
		State.VisualPosition = TargetPosition;
		State.bHasVisualPosition = true;
		State.bResetPosition = false;
	}
	else if (HUD.CardDetailFollowSpeed <= 0.0f)
	{
		State.VisualPosition = TargetPosition;
	}
	else
	{
		State.VisualPosition = FMath::Vector2DInterpTo(
			State.VisualPosition,
			TargetPosition,
			SafeDeltaTime,
			HUD.CardDetailFollowSpeed);
	}

	const float TargetOpacity = State.bWantsVisible ? 1.0f : 0.0f;
	const float OpacitySpeed = State.bWantsVisible ? HUD.CardDetailFadeInSpeed : HUD.CardDetailFadeOutSpeed;
	State.VisualOpacity = OpacitySpeed <= 0.0f
		? TargetOpacity
		: FMath::FInterpTo(State.VisualOpacity, TargetOpacity, SafeDeltaTime, OpacitySpeed);

	if (!State.bWantsVisible && State.VisualOpacity <= 0.01f)
	{
		CollapseHost(State.ActiveHost);
		State = FCardDetailMotionState();
		return;
	}

	ApplyMotionVisual(State.ActiveHost, State.VisualPosition, State.VisualOpacity);
}

bool FWacomBattleHUDCardDetailController::BeginMotionShow(UBattleHUD::ECardDetailHost Host)
{
	if (!HUD.bEnableCardDetailReadabilityPolish)
	{
		return UpdateMotionTarget(Host);
	}

	FCardDetailMotionState& State = MotionState;
	const UBattleHUD::ECardDetailHost PreviousHost = State.ActiveHost;
	if (PreviousHost != UBattleHUD::ECardDetailHost::None && PreviousHost != Host)
	{
		CollapseHost(PreviousHost);
		State.bResetPosition = true;
		State.StableSide = 0;
	}

	State.ActiveHost = Host;
	State.bHasTargetPosition = false;
	switch (Host)
	{
	case UBattleHUD::ECardDetailHost::LegacyHandPanel:
		State.ActiveLegacySource = CurrentLegacySource;
		State.ActiveFirstPersonSourceId.Invalidate();
		State.bHasActiveFirstPersonSlot = false;
		break;
	case UBattleHUD::ECardDetailHost::FirstPersonViewport:
		State.ActiveLegacySource.Reset();
		State.ActiveFirstPersonSourceId = CurrentFirstPersonSourceId;
		break;
	default:
		break;
	}

	if (!UpdateMotionTarget(Host))
	{
		return false;
	}

	RequestMotionShow(Host);
	return true;
}

void FWacomBattleHUDCardDetailController::RequestMotionShow(UBattleHUD::ECardDetailHost Host)
{
	if (!HUD.bEnableCardDetailReadabilityPolish)
	{
		if (UWacomCardDetailPanel* Panel = GetPanelForHost(Host))
		{
			Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
			Panel->SetRenderOpacity(1.0f);
		}
		return;
	}

	FCardDetailMotionState& State = MotionState;
	const bool bWasShowing = State.VisualOpacity > 0.01f || State.bWantsVisible;
	State.ActiveHost = Host;
	State.bPendingShow = !bWasShowing && HUD.CardDetailHoverDelaySeconds > 0.0f;
	State.PendingElapsedSeconds = 0.0f;
	State.bWantsVisible = !State.bPendingShow;
	if (State.bPendingShow)
	{
		if (UWacomCardDetailPanel* Panel = GetPanelForHost(Host))
		{
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			Panel->SetRenderOpacity(0.0f);
		}
		State.VisualOpacity = 0.0f;
		State.bResetPosition = true;
		return;
	}

	if (!State.bHasVisualPosition)
	{
		State.VisualOpacity = 0.0f;
		State.bResetPosition = true;
	}
	if (UWacomCardDetailPanel* Panel = GetPanelForHost(Host))
	{
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void FWacomBattleHUDCardDetailController::RequestMotionHide(
	UBattleHUD::ECardDetailHost Host,
	bool bImmediate)
{
	if (bImmediate || !HUD.bEnableCardDetailReadabilityPolish)
	{
		ForceHideHost(Host);
		return;
	}

	FCardDetailMotionState& State = MotionState;
	if (State.ActiveHost != Host)
	{
		return;
	}

	State.bPendingShow = false;
	State.PendingElapsedSeconds = 0.0f;
	State.bWantsVisible = false;
	switch (Host)
	{
	case UBattleHUD::ECardDetailHost::LegacyHandPanel:
		CurrentLegacySource.Reset();
		break;
	case UBattleHUD::ECardDetailHost::FirstPersonViewport:
		CurrentFirstPersonSourceId.Invalidate();
		LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
		break;
	default:
		break;
	}
}

void FWacomBattleHUDCardDetailController::ForceHideHost(UBattleHUD::ECardDetailHost Host)
{
	CollapseHost(Host);
	if (MotionState.ActiveHost == Host)
	{
		MotionState = FCardDetailMotionState();
	}
	if (Host == UBattleHUD::ECardDetailHost::LegacyHandPanel)
	{
		CurrentLegacySource.Reset();
	}
	else if (Host == UBattleHUD::ECardDetailHost::FirstPersonViewport)
	{
		CurrentFirstPersonSourceId.Invalidate();
		LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
	}
}

void FWacomBattleHUDCardDetailController::ForceHideAll()
{
	CollapseHost(UBattleHUD::ECardDetailHost::LegacyHandPanel);
	CollapseHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	MotionState = FCardDetailMotionState();
	CurrentLegacySource.Reset();
	CurrentFirstPersonSourceId.Invalidate();
	LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
}

UWacomCardDetailPanel* FWacomBattleHUDCardDetailController::GetPanelForHost(
	UBattleHUD::ECardDetailHost Host) const
{
	switch (Host)
	{
	case UBattleHUD::ECardDetailHost::LegacyHandPanel:
		return HUD.CardDetailPanel;
	case UBattleHUD::ECardDetailHost::FirstPersonViewport:
		return HUD.FirstPersonCardDetailPanel;
	default:
		return nullptr;
	}
}

bool FWacomBattleHUDCardDetailController::UpdateMotionTarget(UBattleHUD::ECardDetailHost Host)
{
	switch (Host)
	{
	case UBattleHUD::ECardDetailHost::LegacyHandPanel:
	{
		FVector2D Position = FVector2D::ZeroVector;
		if (!ComputeLegacyTarget(MotionState.ActiveLegacySource.Get(), Position))
		{
			return false;
		}
		MotionState.TargetPosition = Position;
		MotionState.bHasTargetPosition = true;
		return true;
	}
	case UBattleHUD::ECardDetailHost::FirstPersonViewport:
	{
		if (!MotionState.bHasActiveFirstPersonSlot)
		{
			return false;
		}
		FVector2D Position = FVector2D::ZeroVector;
		if (!ComputeFirstPersonTarget(MotionState.ActiveFirstPersonSlot, Position))
		{
			return false;
		}
		MotionState.TargetPosition = Position;
		MotionState.bHasTargetPosition = true;
		return true;
	}
	default:
		return false;
	}
}

bool FWacomBattleHUDCardDetailController::ComputeLegacyTarget(UCardWidget* SourceWidget, FVector2D& OutPosition)
{
	if (!SourceWidget || !HUD.CardDetailLayer || !HUD.CardDetailPanel)
	{
		return false;
	}

	const FGeometry& LayerGeometry = HUD.CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	FVector2D LayerSize = LayerGeometry.GetLocalSize();
	if (LayerSize.X <= 0.0f || LayerSize.Y <= 0.0f)
	{
		LayerSize = GetFirstPersonViewportSize();
	}

	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	OutPosition = ComputeStablePosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		HUD.CardDetailPanelEstimatedSize,
		HUD.CardDetailPanelPadding);
	return true;
}

bool FWacomBattleHUDCardDetailController::ComputeFirstPersonTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	FVector2D& OutPosition)
{
	if (!HUD.FirstPersonCardDetailPanel || !SlotView.bProjected)
	{
		return false;
	}

	const FVector2D ViewportSize = GetFirstPersonViewportSize();
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D AnchorSize =
		HUD.FirstPersonCardDetailAnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	OutPosition = ComputeStablePosition(
		AnchorPosition,
		AnchorSize,
		ViewportSize,
		HUD.CardDetailPanelEstimatedSize,
		HUD.CardDetailPanelPadding);
	return true;
}

FVector2D FWacomBattleHUDCardDetailController::ComputeStablePosition(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding)
{
	const float SafePadding = FMath::Max(0.0f, DetailPadding);
	const float Hysteresis = FMath::Max(0.0f, HUD.CardDetailSideSwitchHysteresisPixels);
	const float MaxX = FMath::Max(0.0f, LayerSize.X - PanelSize.X);
	const float MaxY = FMath::Max(0.0f, LayerSize.Y - PanelSize.Y);
	const float LeftX = AnchorPosition.X - PanelSize.X - SafePadding;
	const float RightX = AnchorPosition.X + AnchorSize.X + SafePadding;

	int32 DesiredSide = MotionState.StableSide;
	if (DesiredSide < 0 && LeftX < -Hysteresis)
	{
		DesiredSide = 0;
	}
	else if (DesiredSide > 0 && RightX > MaxX + Hysteresis)
	{
		DesiredSide = 0;
	}

	if (DesiredSide == 0)
	{
		DesiredSide = LeftX >= 0.0f ? -1 : 1;
	}
	MotionState.StableSide = DesiredSide;

	const float DesiredX = DesiredSide < 0 ? LeftX : RightX;
	const float DesiredY = AnchorPosition.Y + (AnchorSize.Y - PanelSize.Y) * 0.5f;
	return FVector2D(
		FMath::Clamp(DesiredX, 0.0f, MaxX),
		FMath::Clamp(DesiredY, 0.0f, MaxY));
}

void FWacomBattleHUDCardDetailController::ApplyMotionVisual(
	UBattleHUD::ECardDetailHost Host,
	const FVector2D& Position,
	float Opacity)
{
	UWacomCardDetailPanel* Panel = GetPanelForHost(Host);
	if (!Panel)
	{
		return;
	}

	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	const float StartScale = FMath::Clamp(HUD.CardDetailAppearStartScale, 0.5f, 1.0f);
	const float Scale = FMath::Lerp(StartScale, 1.0f, FMath::Clamp(Opacity, 0.0f, 1.0f));
	FWidgetTransform Transform = Panel->GetRenderTransform();
	Transform.Scale = FVector2D(Scale, Scale);
	Panel->SetRenderTransform(Transform);
	Panel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	if (Host == UBattleHUD::ECardDetailHost::LegacyHandPanel)
	{
		if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(Panel->Slot))
		{
			DetailSlot->SetPosition(Position);
			DetailSlot->SetSize(HUD.CardDetailPanelEstimatedSize);
		}
	}
	else if (Host == UBattleHUD::ECardDetailHost::FirstPersonViewport)
	{
		Panel->SetDesiredSizeInViewport(HUD.CardDetailPanelEstimatedSize);
		Panel->SetAlignmentInViewport(FVector2D::ZeroVector);
		Panel->SetPositionInViewport(Position, false);
		LastFirstPersonCardDetailPanelPosition = Position;
	}
}

void FWacomBattleHUDCardDetailController::CollapseHost(UBattleHUD::ECardDetailHost Host)
{
	if (UWacomCardDetailPanel* Panel = GetPanelForHost(Host))
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
		Panel->SetRenderOpacity(0.0f);
		Panel->SetRenderTransform(FWidgetTransform());
	}
}

bool FWacomBattleHUDCardDetailController::IsMotionSource(
	UBattleHUD::ECardDetailHost Host,
	UCardWidget* SourceWidget) const
{
	if (!SourceWidget || Host != UBattleHUD::ECardDetailHost::LegacyHandPanel)
	{
		return false;
	}
	return CurrentLegacySource.Get() == SourceWidget
		|| MotionState.ActiveLegacySource.Get() == SourceWidget;
}

bool FWacomBattleHUDCardDetailController::IsMotionSource(
	UBattleHUD::ECardDetailHost Host,
	const FGuid& CardInstanceId) const
{
	if (!CardInstanceId.IsValid() || Host != UBattleHUD::ECardDetailHost::FirstPersonViewport)
	{
		return false;
	}
	return CurrentFirstPersonSourceId == CardInstanceId
		|| MotionState.ActiveFirstPersonSourceId == CardInstanceId;
}

FVector2D FWacomBattleHUDCardDetailController::GetFirstPersonViewportSize() const
{
	FVector2D ViewportPixelSize = FVector2D::ZeroVector;
	if (const UWorld* World = HUD.GetWorld())
	{
		if (const UGameViewportClient* GameViewport = World->GetGameViewport())
		{
			GameViewport->GetViewportSize(ViewportPixelSize);
		}
	}

	if (ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
	{
		if (const APlayerController* PC = HUD.GetOwningPlayer())
		{
			int32 ViewportX = 0;
			int32 ViewportY = 0;
			PC->GetViewportSize(ViewportX, ViewportY);
			ViewportPixelSize = FVector2D(ViewportX, ViewportY);
		}
	}

	if (ViewportPixelSize.X <= 0.0f || ViewportPixelSize.Y <= 0.0f)
	{
		ViewportPixelSize = FVector2D(1920.0f, 1080.0f);
	}

	float ViewportScale = 1.0f;
	if (const APlayerController* PC = HUD.GetOwningPlayer())
	{
		ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
	}
	ViewportScale = FMath::Max(0.01f, ViewportScale);
	return ViewportPixelSize / ViewportScale;
}

void FWacomBattleHUDCardDetailController::SetFirstPersonSource(const FGuid& CardInstanceId)
{
	CurrentFirstPersonSourceId = CardInstanceId;
}

void FWacomBattleHUDCardDetailController::ClearFirstPersonSource()
{
	CurrentFirstPersonSourceId.Invalidate();
}

bool FWacomBattleHUDCardDetailController::IsCurrentFirstPersonSource(const FGuid& CardInstanceId) const
{
	return CardInstanceId.IsValid() && CurrentFirstPersonSourceId == CardInstanceId;
}

void FWacomBattleHUDCardDetailController::ClearLegacySource()
{
	CurrentLegacySource.Reset();
}

void FWacomBattleHUDCardDetailController::UpdateFirstPersonSlot(
	const FWacomFirstPersonCardLayerSlotView& SlotView)
{
	MotionState.ActiveFirstPersonSlot = SlotView;
	MotionState.bHasActiveFirstPersonSlot = true;
}

void FWacomBattleHUDCardDetailController::RemoveFirstPersonPanelFromViewport()
{
	ForceHideHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	if (HUD.FirstPersonCardDetailPanel)
	{
		HUD.FirstPersonCardDetailPanel->RemoveFromParent();
		HUD.FirstPersonCardDetailPanel = nullptr;
	}
}
