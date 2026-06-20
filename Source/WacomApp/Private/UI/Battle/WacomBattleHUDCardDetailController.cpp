// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCardDetailController.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Card/WacomCardDetailPanel.h"

namespace
{
	bool AreDetailTextsEquivalent(const FText& Left, const FText& Right)
	{
		return Left.ToString() == Right.ToString();
	}

	bool AreDetailTextArraysEquivalent(const TArray<FText>& Left, const TArray<FText>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!AreDetailTextsEquivalent(Left[Index], Right[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool AreDetailTokensEquivalent(
		const FWacomCardDetailToken& Left,
		const FWacomCardDetailToken& Right)
	{
		return Left.StableId == Right.StableId
			&& Left.Kind == Right.Kind
			&& AreDetailTextsEquivalent(Left.Text, Right.Text)
			&& Left.Icon == Right.Icon
			&& Left.Value == Right.Value
			&& Left.bHasValue == Right.bHasValue
			&& Left.PreviewValue == Right.PreviewValue
			&& Left.bHasPreviewValue == Right.bHasPreviewValue
			&& Left.bSkipped == Right.bSkipped
			&& Left.bEmphasized == Right.bEmphasized;
	}

	bool AreDetailTokenArraysEquivalent(
		const TArray<FWacomCardDetailToken>& Left,
		const TArray<FWacomCardDetailToken>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!AreDetailTokensEquivalent(Left[Index], Right[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool AreDetailTokenLinesEquivalent(
		const TArray<FWacomCardDetailTokenLine>& Left,
		const TArray<FWacomCardDetailTokenLine>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].LineId != Right[Index].LineId
				|| Left[Index].Kind != Right[Index].Kind
				|| !AreDetailTokenArraysEquivalent(Left[Index].Tokens, Right[Index].Tokens))
			{
				return false;
			}
		}
		return true;
	}

	bool AreCardDetailViewDataEquivalent(
		const FWacomCardDetailViewData& Left,
		const FWacomCardDetailViewData& Right)
	{
		return AreDetailTextsEquivalent(Left.Name, Right.Name)
			&& AreDetailTextsEquivalent(Left.Description, Right.Description)
			&& AreDetailTextArraysEquivalent(Left.TaskLines, Right.TaskLines)
			&& AreDetailTextArraysEquivalent(Left.PassiveLines, Right.PassiveLines)
			&& AreDetailTokenLinesEquivalent(Left.TokenLines, Right.TokenLines);
	}
}

FWacomBattleHUDCardDetailController::FWacomBattleHUDCardDetailController(UBattleHUD& InHUD)
	: HUD(InHUD)
{
}

bool FWacomBattleHUDCardDetailController::IsVisible() const
{
	return HUD.FirstPersonCardDetailPanel
		&& HUD.FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

FText FWacomBattleHUDCardDetailController::GetNameText() const
{
	if (HUD.FirstPersonCardDetailPanel
		&& HUD.FirstPersonCardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return HUD.FirstPersonCardDetailPanel->GetNameText();
	}
	return FText::GetEmpty();
}

void FWacomBattleHUDCardDetailController::HideAll()
{
	ForceHideAll();
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

	const bool bCanReuseCurrentDetail = HUD.bEnableCardDetailReadabilityPolish
		&& MotionState.ActiveHost == UBattleHUD::ECardDetailHost::FirstPersonViewport
		&& MotionState.bHasActiveFirstPersonSlot
		&& AreCardDetailViewDataEquivalent(Panel->GetCardDetailData(), DetailData);
	if (bCanReuseCurrentDetail)
	{
		PositionFirstPersonBesideSlot(SlotView);
		Panel->SetDesiredSizeInViewport(HUD.CardDetailPanelEstimatedSize);
		Panel->SetIsEnabled(true);
		MotionState.ActiveFirstPersonSlot = SlotView;
		MotionState.bHasActiveFirstPersonSlot = true;
		RequestMotionShow(UBattleHUD::ECardDetailHost::FirstPersonViewport);
		return true;
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
	case UBattleHUD::ECardDetailHost::FirstPersonViewport:
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
	if (Host == UBattleHUD::ECardDetailHost::FirstPersonViewport)
	{
		CurrentFirstPersonSourceId.Invalidate();
		LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
	}
}

void FWacomBattleHUDCardDetailController::ForceHideHost(UBattleHUD::ECardDetailHost Host)
{
	CollapseHost(Host);
	if (MotionState.ActiveHost == Host)
	{
		MotionState = FCardDetailMotionState();
	}
	if (Host == UBattleHUD::ECardDetailHost::FirstPersonViewport)
	{
		CurrentFirstPersonSourceId.Invalidate();
		LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
	}
}

void FWacomBattleHUDCardDetailController::ForceHideAll()
{
	CollapseHost(UBattleHUD::ECardDetailHost::FirstPersonViewport);
	MotionState = FCardDetailMotionState();
	CurrentFirstPersonSourceId.Invalidate();
	LastFirstPersonCardDetailPanelPosition = FVector2D::ZeroVector;
}

UWacomCardDetailPanel* FWacomBattleHUDCardDetailController::GetPanelForHost(
	UBattleHUD::ECardDetailHost Host) const
{
	switch (Host)
	{
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

	if (Host == UBattleHUD::ECardDetailHost::FirstPersonViewport)
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
