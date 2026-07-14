// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardSlotLayoutBuilder.h"

TArray<FWacomFirstPersonCardLayerSlotView> FWacomFirstPersonCardSlotLayoutBuilder::BuildSlots(
	const FWacomFirstPersonCardSlotLayoutBuildInput& Input)
{
	TArray<FWacomFirstPersonCardLayerSlotView> Slots;
	if (!Input.bHasValidAnchor || !Input.CardEntries || !Input.Config)
	{
		return Slots;
	}

	const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries = *Input.CardEntries;
	const FWacomFirstPersonCardResolvedLayoutConfig& Config = *Input.Config;
	const int32 ClampedCount = FMath::Clamp(CardEntries.Num(), 0, 32);
	Slots.Reserve(ClampedCount);
	const float EffectiveEdgeDropPixels = ResolveEdgeDropPixelsForHandCount(Config, ClampedCount);

	bool bHasPendingTargetingCard = false;
	for (int32 EntryIndex = 0; EntryIndex < ClampedCount; ++EntryIndex)
	{
		if (CardEntries[EntryIndex].bIsPendingTargeting)
		{
			bHasPendingTargetingCard = true;
			break;
		}
	}

	for (int32 Index = 0; Index < ClampedCount; ++Index)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = Index;
		Slot.Entry = CardEntries[Index];
		Slot.Entry.bIsPlayable = Slot.Entry.bIsPlayable && !Slot.Entry.CardViewData.bDisabled;
		Slot.Entry.CardViewData.bDisabled = !Slot.Entry.bIsPlayable;
		Slot.RenderScale = FMath::Max(0.01f, Config.HandCardRenderScale);
		Slot.PresentationScale = FMath::Clamp(Config.PresentationScale, 0.5f, 1.0f);
		Slot.RenderOpacity = Slot.Entry.bIsPlayable
			? 1.0f
			: FMath::Clamp(Config.DisabledRenderOpacity, 0.0f, 1.0f);
		Slot.ZOrder = Index;
		Slot.ProjectionMode = Config.ProjectionMode;
		Slot.ViewportClampMode = Config.ViewportClampMode;
		Slot.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
		Slot.bCurrentCameraProjection = true;
		Slot.bLookOffsetAppliedToLayout = Input.bCurrentLookOffsetAppliedToLayout;

		const float CenterOffset =
			static_cast<float>(Index) - (static_cast<float>(ClampedCount - 1) * 0.5f);
		const float MaxAbsCenterOffset = FMath::Max(1.0f, static_cast<float>(ClampedCount - 1) * 0.5f);
		const float NormalizedHandOffset = CenterOffset / MaxAbsCenterOffset;
		const float NormalizedEdgeDistance = FMath::Abs(NormalizedHandOffset);
		const float FanCurveExponent = FMath::Max(0.01f, Config.AuthoredFanCurveExponent);
		const float FanDirection = CenterOffset < 0.0f ? -1.0f : 1.0f;
		const float AuthoredFanMagnitude =
			FMath::Pow(NormalizedEdgeDistance, FanCurveExponent) * MaxAbsCenterOffset;
		const float FanOffset = FanDirection * AuthoredFanMagnitude;
		const bool bIsHovered =
			Slot.Entry.CardInstanceId.IsValid()
			&& Slot.Entry.CardInstanceId == Input.HoveredCardInstanceId;
		Slot.NormalizedHandOffset = NormalizedHandOffset;
		Slot.RenderAngleDegrees = ClampRenderAngle(FanOffset * Config.FanYawDegrees, Config);
		if (Config.bAuthoredCenterCardsDrawOnTop)
		{
			Slot.ZOrder = FMath::RoundToInt((1.0f - NormalizedEdgeDistance) * 100.0f);
		}
		Slot.InputHitScale = Slot.RenderScale;
		Slot.InputHitAngleDegrees = Slot.RenderAngleDegrees;
		Slot.InputHitOrder = Index;
		Slot.bIsHovered = bIsHovered;
		Slot.bHasPendingTargetingCardInHand = bHasPendingTargetingCard;

		Slot.RawScreenPosition = Input.AnchorPoint.RawScreenPosition;
		Slot.UnclampedWidgetPosition = Input.AnchorPoint.UnclampedWidgetPosition;
		Slot.ViewportScale = Input.AnchorPoint.ViewportScale;
		Slot.ProjectionMode = Input.AnchorPoint.ProjectionMode;
		Slot.ViewportClampMode = Input.AnchorPoint.ViewportClampMode;
		Slot.AnchorWidgetPosition = Input.AnchorPoint.WidgetPosition;
		Slot.UnsmoothedAnchorWidgetPosition = Input.AnchorPoint.UnsmoothedAnchorWidgetPosition;
		Slot.SmoothedAnchorWidgetPosition = Input.AnchorPoint.SmoothedAnchorWidgetPosition;
		Slot.OffscreenDistancePixels = Input.AnchorPoint.OffscreenDistancePixels;
		Slot.AnchorScreenSmoothingDistancePixels = Input.AnchorPoint.AnchorScreenSmoothingDistancePixels;
		Slot.bBodyLockedLayout = Config.ProjectionMode == EWacomFirstPersonCardProjectionMode::BodyLocked;
		Slot.bCurrentCameraProjection = true;
		Slot.bLookOffsetAppliedToLayout = Input.bCurrentLookOffsetAppliedToLayout;
		Slot.bClamped = Input.AnchorPoint.bClamped;
		Slot.bOutsideViewport = Input.AnchorPoint.bOutsideViewport;
		Slot.bAnchorScreenSmoothed = Input.AnchorPoint.bAnchorScreenSmoothed;
		Slot.bProjected = false;

		if (Input.bAnchorProjected)
		{
			const float NaturalHandWidth =
				FMath::Max(0, ClampedCount - 1) * FMath::Max(0.0f, Config.AuthoredCardSpacingPixels);
			const float MaxHandWidth = FMath::Max(0.0f, Config.AuthoredMaxHandWidthPixels);
			const float WidthScale = (MaxHandWidth > 0.0f && NaturalHandWidth > MaxHandWidth)
				? MaxHandWidth / NaturalHandWidth
				: 1.0f;
			const float XOffset = CenterOffset * FMath::Max(0.0f, Config.AuthoredCardSpacingPixels) * WidthScale;
			const float DropMagnitude =
				FMath::Pow(NormalizedEdgeDistance, FMath::Max(0.01f, Config.AuthoredDropCurveExponent))
				* EffectiveEdgeDropPixels;
			const float CenterLiftMagnitude =
				(1.0f - NormalizedEdgeDistance) * Config.AuthoredCenterLiftPixels;

			FVector2D FinalPosition =
				Input.AnchorPoint.WidgetPosition
				+ Config.AuthoredHandScreenOffset
				+ FVector2D(XOffset, DropMagnitude - CenterLiftMagnitude);
			Slot.AuthoredLayoutOffset = FinalPosition - Input.AnchorPoint.WidgetPosition;
			Slot.bBodyLockedLayout = Input.AnchorPoint.bBodyLockedLayout;
			Slot.bCurrentCameraProjection = Input.AnchorPoint.bCurrentCameraProjection;
			Slot.bLookOffsetAppliedToLayout = Input.AnchorPoint.bLookOffsetAppliedToLayout;
			bool bPixelSnapped = false;
			Slot.WidgetPosition = FinalPosition;
			Slot.SnappedWidgetPosition = SnapPosition(FinalPosition, Config, bPixelSnapped);
			Slot.ScreenPosition = Slot.SnappedWidgetPosition;
			Slot.InputHitCenter = Slot.SnappedWidgetPosition;
			Slot.bPixelSnapped = bPixelSnapped;
			Slot.bProjected = Input.AnchorPoint.bProjected;
		}

		Slots.Add(Slot);
	}
	return Slots;
}

FVector2D FWacomFirstPersonCardSlotLayoutBuilder::ApplyViewportClampToWidgetPosition(
	FVector2D UnclampedPosition,
	FVector2D WidgetViewportSize,
	const FWacomFirstPersonCardResolvedLayoutConfig& Config,
	bool& bOutClamped,
	bool& bOutOutsideViewport,
	float& OutOffscreenDistancePixels)
{
	bOutClamped = false;
	bOutOutsideViewport = false;
	OutOffscreenDistancePixels = 0.0f;

	const float Padding = FMath::Max(0.0f, Config.ProjectionPadding);
	const FVector2D SafeMin(Padding, Padding);
	const FVector2D SafeMax(
		FMath::Max(Padding, WidgetViewportSize.X - Padding),
		FMath::Max(Padding, WidgetViewportSize.Y - Padding));
	const FVector2D NearestSafePoint(
		FMath::Clamp(UnclampedPosition.X, SafeMin.X, SafeMax.X),
		FMath::Clamp(UnclampedPosition.Y, SafeMin.Y, SafeMax.Y));
	OutOffscreenDistancePixels = FVector2D::Distance(UnclampedPosition, NearestSafePoint);
	bOutOutsideViewport = OutOffscreenDistancePixels > KINDA_SMALL_NUMBER;

	if (Config.ViewportClampMode == EWacomFirstPersonCardViewportClampMode::AllowOffscreen)
	{
		return UnclampedPosition;
	}

	if (Config.ViewportClampMode == EWacomFirstPersonCardViewportClampMode::HardClampToViewport)
	{
		bOutClamped = bOutOutsideViewport;
		return NearestSafePoint;
	}

	const float Allowance = FMath::Max(0.0f, Config.SoftClampOffscreenAllowancePixels);
	const FVector2D SoftMin = SafeMin - FVector2D(Allowance, Allowance);
	const FVector2D SoftMax = SafeMax + FVector2D(Allowance, Allowance);
	const FVector2D NearestSoftPoint(
		FMath::Clamp(UnclampedPosition.X, SoftMin.X, SoftMax.X),
		FMath::Clamp(UnclampedPosition.Y, SoftMin.Y, SoftMax.Y));
	const float SoftOvershootDistance = FVector2D::Distance(UnclampedPosition, NearestSoftPoint);
	if (SoftOvershootDistance <= KINDA_SMALL_NUMBER)
	{
		return UnclampedPosition;
	}

	const float BlendRange = FMath::Max(0.0f, Config.SoftClampBlendRangePixels);
	const float Alpha = BlendRange <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(SoftOvershootDistance / BlendRange, 0.0f, 1.0f));
	const FVector2D ClampedPosition = FMath::Lerp(UnclampedPosition, NearestSoftPoint, Alpha);
	bOutClamped = !UnclampedPosition.Equals(ClampedPosition, KINDA_SMALL_NUMBER);
	return ClampedPosition;
}

FVector2D FWacomFirstPersonCardSlotLayoutBuilder::SnapPosition(
	FVector2D Position,
	const FWacomFirstPersonCardResolvedLayoutConfig& Config,
	bool& bOutPixelSnapped)
{
	bOutPixelSnapped = false;
	if (!Config.bEnableCardLayerPixelSnapping)
	{
		return Position;
	}

	const float Grid = FMath::Max(0.01f, Config.CardLayerPixelSnapGrid);
	const FVector2D SnappedPosition(
		FMath::RoundToFloat(Position.X / Grid) * Grid,
		FMath::RoundToFloat(Position.Y / Grid) * Grid);
	bOutPixelSnapped = !SnappedPosition.Equals(Position, KINDA_SMALL_NUMBER);
	return SnappedPosition;
}

float FWacomFirstPersonCardSlotLayoutBuilder::ResolveEdgeDropPixelsForHandCount(
	const FWacomFirstPersonCardResolvedLayoutConfig& Config,
	int32 CardCount)
{
	const float MaxEdgeDrop = FMath::Max(0.0f, Config.HandMaxEdgeDropPixels);
	if (!Config.bScaleEdgeDropByHandCount)
	{
		return MaxEdgeDrop;
	}

	const float ShortHandEdgeDrop = FMath::Clamp(Config.ShortHandEdgeDropPixels, 0.0f, MaxEdgeDrop);
	const int32 MinCardCount = FMath::Max(1, Config.EdgeDropScaleMinCardCount);
	const int32 MaxCardCount = FMath::Max(MinCardCount + 1, Config.EdgeDropScaleMaxCardCount);
	const float Alpha = FMath::SmoothStep(
		0.0f,
		1.0f,
		FMath::Clamp(
			static_cast<float>(CardCount - MinCardCount) / static_cast<float>(MaxCardCount - MinCardCount),
			0.0f,
			1.0f));
	return FMath::Lerp(ShortHandEdgeDrop, MaxEdgeDrop, Alpha);
}

float FWacomFirstPersonCardSlotLayoutBuilder::ClampRenderAngle(
	float AngleDegrees,
	const FWacomFirstPersonCardResolvedLayoutConfig& Config)
{
	if (!Config.bClampCardLayerRenderAngle)
	{
		return AngleDegrees;
	}

	const float MaxAbsAngle = FMath::Max(0.0f, Config.MaxCardLayerRenderAngleDegrees);
	return FMath::Clamp(AngleDegrees, -MaxAbsAngle, MaxAbsAngle);
}
