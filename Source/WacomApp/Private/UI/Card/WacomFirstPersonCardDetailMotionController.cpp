// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDetailMotionController.h"

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

	bool AreDetailSectionsEquivalent(
		const TArray<FWacomCardDetailSection>& Left,
		const TArray<FWacomCardDetailSection>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].SectionId != Right[Index].SectionId
				|| Left[Index].Kind != Right[Index].Kind
				|| !AreDetailTextsEquivalent(Left[Index].Title, Right[Index].Title)
				|| !AreDetailTokenLinesEquivalent(Left[Index].TokenLines, Right[Index].TokenLines))
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
			&& AreDetailSectionsEquivalent(Left.Sections, Right.Sections)
			&& AreDetailTextArraysEquivalent(Left.TaskLines, Right.TaskLines)
			&& AreDetailTokenLinesEquivalent(Left.TokenLines, Right.TokenLines);
	}
}

bool FWacomFirstPersonCardDetailMotionController::IsVisible(
	const UWacomCardDetailPanel* Panel) const
{
	return Panel && Panel->GetVisibility() != ESlateVisibility::Collapsed;
}

bool FWacomFirstPersonCardDetailMotionController::IsPrewarmed(
	const UWacomCardDetailPanel* Panel) const
{
	return Panel != nullptr;
}

FText FWacomFirstPersonCardDetailMotionController::GetNameText(
	const UWacomCardDetailPanel* Panel) const
{
	if (Panel && Panel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return Panel->GetNameText();
	}
	return FText::GetEmpty();
}

void FWacomFirstPersonCardDetailMotionController::PrewarmPanel(
	UWacomCardDetailPanel& Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config)
{
	Panel.SetVisibility(ESlateVisibility::Collapsed);
	Panel.SetIsEnabled(true);
	Panel.SetRenderOpacity(0.0f);
	Panel.SetRenderTransform(FWidgetTransform());
	Panel.SetDesiredSizeInViewport(Config.PanelEstimatedSize);
}

bool FWacomFirstPersonCardDetailMotionController::CanReuseCurrentDetail(
	const FGuid& CardInstanceId,
	const UWacomCardDetailPanel* Panel) const
{
	return CardInstanceId.IsValid()
		&& CurrentSourceId == CardInstanceId
		&& Panel != nullptr;
}

bool FWacomFirstPersonCardDetailMotionController::ShowExistingAtSlot(
	UWacomCardDetailPanel& Panel,
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	if (!CanReuseCurrentDetail(CardInstanceId, &Panel) || !SlotView.bProjected)
	{
		return false;
	}

	SetCurrentSource(CardInstanceId);
	PositionBesideSlot(Panel, SlotView, Config, ViewportSize);
	Panel.SetDesiredSizeInViewport(Config.PanelEstimatedSize);
	Panel.SetIsEnabled(true);
	MotionState.ActiveSlot = SlotView;
	MotionState.bHasActiveSlot = true;
	RequestMotionShow(Panel, Config);
	return true;
}

bool FWacomFirstPersonCardDetailMotionController::ShowAtSlot(
	UWacomCardDetailPanel& Panel,
	const FGuid& CardInstanceId,
	const FWacomCardDetailViewData& DetailData,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	if (!CardInstanceId.IsValid() || !SlotView.bProjected)
	{
		ForceHideAll(&Panel);
		return false;
	}

	const bool bCanReuseCurrentData = Config.bEnableReadabilityPolish
		&& CurrentSourceId == CardInstanceId
		&& AreCardDetailViewDataEquivalent(Panel.GetCardDetailData(), DetailData);
	if (!bCanReuseCurrentData)
	{
		Panel.SetCardDetailData(DetailData);
#if WITH_AUTOMATION_TESTS
		++DetailDataApplyCountForTest;
#endif
	}

	SetCurrentSource(CardInstanceId);
	PositionBesideSlot(Panel, SlotView, Config, ViewportSize);
	Panel.SetDesiredSizeInViewport(Config.PanelEstimatedSize);
	Panel.SetIsEnabled(true);
	MotionState.ActiveSlot = SlotView;
	MotionState.bHasActiveSlot = true;

	if (!Config.bEnableReadabilityPolish)
	{
		Panel.SetRenderOpacity(1.0f);
		Panel.SetRenderTransform(FWidgetTransform());
		Panel.SetVisibility(ESlateVisibility::HitTestInvisible);
		return true;
	}

	if (!BeginMotionShow(Panel, Config, ViewportSize))
	{
		ForceHideAll(&Panel);
		return false;
	}
	return true;
}

void FWacomFirstPersonCardDetailMotionController::UpdateCurrentSlot(
	const FGuid& CardInstanceId,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	UWacomCardDetailPanel* Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	if (!Panel
		|| !CardInstanceId.IsValid()
		|| CurrentSourceId != CardInstanceId
		|| !MotionState.bHasActiveSlot
		|| !SlotView.bProjected)
	{
		return;
	}

	MotionState.ActiveSlot = SlotView;
	PositionBesideSlot(*Panel, SlotView, Config, ViewportSize);
}

void FWacomFirstPersonCardDetailMotionController::HideForSource(
	const FGuid& CardInstanceId,
	UWacomCardDetailPanel* Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config)
{
	if (!IsMotionSource(CardInstanceId))
	{
		return;
	}

	RequestMotionHide(Panel, Config, !Config.bEnableReadabilityPolish);
}

void FWacomFirstPersonCardDetailMotionController::ForceHideAll(
	UWacomCardDetailPanel* Panel)
{
	CollapsePanel(Panel);
	MotionState = FMotionState();
	ClearCurrentSource();
	LastPanelPosition = FVector2D::ZeroVector;
}

void FWacomFirstPersonCardDetailMotionController::TickMotion(
	float DeltaTime,
	UWacomCardDetailPanel* Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	if (!Config.bEnableReadabilityPolish)
	{
		return;
	}

	if (!MotionState.ActiveSourceId.IsValid() && !MotionState.bPendingShow)
	{
		return;
	}

	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	if (MotionState.bPendingShow)
	{
		MotionState.PendingElapsedSeconds += SafeDeltaTime;
		UpdateMotionTarget(Config, ViewportSize);
		if (MotionState.PendingElapsedSeconds < FMath::Max(0.0f, Config.HoverDelaySeconds))
		{
			return;
		}

		MotionState.bPendingShow = false;
		MotionState.bWantsVisible = true;
		MotionState.VisualOpacity = 0.0f;
		MotionState.bResetPosition = true;
		if (Panel)
		{
			Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
			Panel->SetRenderOpacity(0.0f);
		}
	}

	if (!UpdateMotionTarget(Config, ViewportSize))
	{
		ForceHideAll(Panel);
		return;
	}

	if (!Panel)
	{
		ForceHideAll(nullptr);
		return;
	}

	if (MotionState.bWantsVisible && Panel->GetVisibility() == ESlateVisibility::Collapsed)
	{
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const FVector2D TargetPosition =
		MotionState.bHasTargetPosition ? MotionState.TargetPosition : FVector2D::ZeroVector;
	const float ResetDistance = FMath::Max(0.0f, Config.PositionResetDistancePixels);
	if (!MotionState.bHasVisualPosition
		|| MotionState.bResetPosition
		|| (ResetDistance > 0.0f
			&& FVector2D::Distance(MotionState.VisualPosition, TargetPosition) > ResetDistance))
	{
		MotionState.VisualPosition = TargetPosition;
		MotionState.bHasVisualPosition = true;
		MotionState.bResetPosition = false;
	}
	else if (Config.FollowSpeed <= 0.0f)
	{
		MotionState.VisualPosition = TargetPosition;
	}
	else
	{
		MotionState.VisualPosition = FMath::Vector2DInterpTo(
			MotionState.VisualPosition,
			TargetPosition,
			SafeDeltaTime,
			Config.FollowSpeed);
	}

	const float TargetOpacity = MotionState.bWantsVisible ? 1.0f : 0.0f;
	const float OpacitySpeed =
		MotionState.bWantsVisible ? Config.FadeInSpeed : Config.FadeOutSpeed;
	MotionState.VisualOpacity = OpacitySpeed <= 0.0f
		? TargetOpacity
		: FMath::FInterpTo(MotionState.VisualOpacity, TargetOpacity, SafeDeltaTime, OpacitySpeed);

	if (!MotionState.bWantsVisible && MotionState.VisualOpacity <= 0.01f)
	{
		CollapsePanel(Panel);
		MotionState = FMotionState();
		ClearCurrentSource();
		LastPanelPosition = FVector2D::ZeroVector;
		return;
	}

	ApplyMotionVisual(*Panel, MotionState.VisualPosition, MotionState.VisualOpacity, Config);
}

void FWacomFirstPersonCardDetailMotionController::SetCurrentSource(
	const FGuid& CardInstanceId)
{
	CurrentSourceId = CardInstanceId;
	MotionState.ActiveSourceId = CardInstanceId;
}

void FWacomFirstPersonCardDetailMotionController::ClearCurrentSource()
{
	CurrentSourceId.Invalidate();
}

bool FWacomFirstPersonCardDetailMotionController::IsCurrentSource(
	const FGuid& CardInstanceId) const
{
	return CardInstanceId.IsValid() && CurrentSourceId == CardInstanceId;
}

bool FWacomFirstPersonCardDetailMotionController::IsMotionSource(
	const FGuid& CardInstanceId) const
{
	if (!CardInstanceId.IsValid())
	{
		return false;
	}
	return CurrentSourceId == CardInstanceId || MotionState.ActiveSourceId == CardInstanceId;
}

bool FWacomFirstPersonCardDetailMotionController::GetActiveSlotForSource(
	const FGuid& CardInstanceId,
	FWacomFirstPersonCardLayerSlotView& OutSlotView) const
{
	if (!CardInstanceId.IsValid()
		|| MotionState.ActiveSourceId != CardInstanceId
		|| !MotionState.bHasActiveSlot)
	{
		return false;
	}

	OutSlotView = MotionState.ActiveSlot;
	return true;
}

bool FWacomFirstPersonCardDetailMotionController::IsActiveSlotInspectingForSource(
	const FGuid& CardInstanceId) const
{
	FWacomFirstPersonCardLayerSlotView SlotView;
	return GetActiveSlotForSource(CardInstanceId, SlotView)
		&& SlotView.GestureState == EWacomFirstPersonCardGestureState::Inspecting;
}

void FWacomFirstPersonCardDetailMotionController::PositionBesideSlot(
	UWacomCardDetailPanel& Panel,
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	const FVector2D AnchorSize =
		Config.AnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	const FVector2D Position = Config.bEnableReadabilityPolish
		? ComputeStablePosition(
			AnchorPosition,
			AnchorSize,
			ViewportSize,
			Config.PanelEstimatedSize,
			Config.DetailPadding,
			Config.SideSwitchHysteresisPixels)
		: ComputeImmediatePosition(
			AnchorPosition,
			AnchorSize,
			ViewportSize,
			Config.PanelEstimatedSize,
			Config.DetailPadding);

	Panel.SetDesiredSizeInViewport(Config.PanelEstimatedSize);
	Panel.SetAlignmentInViewport(FVector2D::ZeroVector);
	if (Config.bEnableReadabilityPolish)
	{
		MotionState.TargetPosition = Position;
		MotionState.bHasTargetPosition = true;
		return;
	}

	Panel.SetPositionInViewport(Position, false);
	LastPanelPosition = Position;
}

bool FWacomFirstPersonCardDetailMotionController::BeginMotionShow(
	UWacomCardDetailPanel& Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	if (!Config.bEnableReadabilityPolish)
	{
		return UpdateMotionTarget(Config, ViewportSize);
	}

	MotionState.bHasTargetPosition = false;
	if (!UpdateMotionTarget(Config, ViewportSize))
	{
		return false;
	}

	RequestMotionShow(Panel, Config);
	return true;
}

void FWacomFirstPersonCardDetailMotionController::RequestMotionShow(
	UWacomCardDetailPanel& Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config)
{
	if (!Config.bEnableReadabilityPolish)
	{
		Panel.SetVisibility(ESlateVisibility::HitTestInvisible);
		Panel.SetRenderOpacity(1.0f);
		return;
	}

	const bool bWasShowing = MotionState.VisualOpacity > 0.01f || MotionState.bWantsVisible;
	MotionState.bPendingShow = !bWasShowing && Config.HoverDelaySeconds > 0.0f;
	MotionState.PendingElapsedSeconds = 0.0f;
	MotionState.bWantsVisible = !MotionState.bPendingShow;
	if (MotionState.bPendingShow)
	{
		Panel.SetVisibility(ESlateVisibility::Collapsed);
		Panel.SetRenderOpacity(0.0f);
		MotionState.VisualOpacity = 0.0f;
		MotionState.bResetPosition = true;
		return;
	}

	if (!MotionState.bHasVisualPosition)
	{
		MotionState.VisualOpacity = 0.0f;
		MotionState.bResetPosition = true;
	}
	Panel.SetVisibility(ESlateVisibility::HitTestInvisible);
}

void FWacomFirstPersonCardDetailMotionController::RequestMotionHide(
	UWacomCardDetailPanel* Panel,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	bool bImmediate)
{
	if (bImmediate || !Config.bEnableReadabilityPolish)
	{
		ForceHideAll(Panel);
		return;
	}

	MotionState.bPendingShow = false;
	MotionState.PendingElapsedSeconds = 0.0f;
	MotionState.bWantsVisible = false;
	ClearCurrentSource();
}

bool FWacomFirstPersonCardDetailMotionController::UpdateMotionTarget(
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize)
{
	if (!MotionState.bHasActiveSlot)
	{
		return false;
	}

	FVector2D Position = FVector2D::ZeroVector;
	if (!ComputeTarget(MotionState.ActiveSlot, Config, ViewportSize, Position))
	{
		return false;
	}

	MotionState.TargetPosition = Position;
	MotionState.bHasTargetPosition = true;
	return true;
}

bool FWacomFirstPersonCardDetailMotionController::ComputeTarget(
	const FWacomFirstPersonCardLayerSlotView& SlotView,
	const FWacomFirstPersonCardDetailMotionConfig& Config,
	const FVector2D& ViewportSize,
	FVector2D& OutPosition)
{
	if (!SlotView.bProjected || ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D AnchorSize =
		Config.AnchorBaseSize * FMath::Max(0.01f, SlotView.RenderScale);
	const FVector2D AnchorPosition = SlotView.ScreenPosition - AnchorSize * 0.5f;
	OutPosition = ComputeStablePosition(
		AnchorPosition,
		AnchorSize,
		ViewportSize,
		Config.PanelEstimatedSize,
		Config.DetailPadding,
		Config.SideSwitchHysteresisPixels);
	return true;
}

FVector2D FWacomFirstPersonCardDetailMotionController::ComputeStablePosition(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding,
	float SideSwitchHysteresisPixels)
{
	const float SafePadding = FMath::Max(0.0f, DetailPadding);
	const float Hysteresis = FMath::Max(0.0f, SideSwitchHysteresisPixels);
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

FVector2D FWacomFirstPersonCardDetailMotionController::ComputeImmediatePosition(
	const FVector2D& AnchorPosition,
	const FVector2D& AnchorSize,
	const FVector2D& LayerSize,
	const FVector2D& PanelSize,
	float DetailPadding) const
{
	const float SafePadding = FMath::Max(0.0f, DetailPadding);
	const float MaxX = FMath::Max(0.0f, LayerSize.X - PanelSize.X);
	const float MaxY = FMath::Max(0.0f, LayerSize.Y - PanelSize.Y);
	const float LeftX = AnchorPosition.X - PanelSize.X - SafePadding;
	const float RightX = AnchorPosition.X + AnchorSize.X + SafePadding;
	const float DesiredX = LeftX >= 0.0f ? LeftX : RightX;
	const float DesiredY = AnchorPosition.Y + (AnchorSize.Y - PanelSize.Y) * 0.5f;
	return FVector2D(
		FMath::Clamp(DesiredX, 0.0f, MaxX),
		FMath::Clamp(DesiredY, 0.0f, MaxY));
}

void FWacomFirstPersonCardDetailMotionController::ApplyMotionVisual(
	UWacomCardDetailPanel& Panel,
	const FVector2D& Position,
	float Opacity,
	const FWacomFirstPersonCardDetailMotionConfig& Config)
{
	const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	Panel.SetVisibility(ESlateVisibility::HitTestInvisible);
	Panel.SetRenderOpacity(ClampedOpacity);
	const float StartScale = FMath::Clamp(Config.AppearStartScale, 0.5f, 1.0f);
	const float Scale = FMath::Lerp(StartScale, 1.0f, ClampedOpacity);
	FWidgetTransform Transform = Panel.GetRenderTransform();
	Transform.Scale = FVector2D(Scale, Scale);
	Panel.SetRenderTransform(Transform);
	Panel.SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Panel.SetDesiredSizeInViewport(Config.PanelEstimatedSize);
	Panel.SetAlignmentInViewport(FVector2D::ZeroVector);
	Panel.SetPositionInViewport(Position, false);
	LastPanelPosition = Position;
}

void FWacomFirstPersonCardDetailMotionController::CollapsePanel(
	UWacomCardDetailPanel* Panel)
{
	if (!Panel)
	{
		return;
	}

	Panel->SetVisibility(ESlateVisibility::Collapsed);
	Panel->SetRenderOpacity(0.0f);
	Panel->SetRenderTransform(FWidgetTransform());
}
