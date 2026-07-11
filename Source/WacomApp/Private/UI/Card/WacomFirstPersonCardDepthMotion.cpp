// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDepthMotion.h"

namespace
{
	constexpr float DepthTiltToleranceDegrees = 0.01f;
	constexpr float DepthShadowOffsetTolerancePixels = 0.02f;
	constexpr float DepthScalarTolerance = 0.001f;
	constexpr float PointerVelocityTolerance = 0.5f;
}

const FWacomFirstPersonCardDepthView& FWacomFirstPersonCardDepthMotion::Update(
	const FWacomFirstPersonCardDepthConfig& Config,
	const FWacomFirstPersonCardDepthMotionInput& Input,
	float DeltaTime)
{
	const float SafeDeltaTime = FMath::Clamp(DeltaTime, 0.0f, 0.1f);
	UpdateFilteredPointerVelocity(Config, Input, SafeDeltaTime);
	TargetView = BuildTargetView(Config, Input);
	bHasTarget = true;

	if (!bInitialized)
	{
		CurrentView = MakeNeutralView(Config, Input.bProjected);
		bInitialized = true;
	}

	const bool bReturningToRest = TargetView.TiltDegrees.IsNearlyZero(DepthTiltToleranceDegrees)
		&& TargetView.ShadowOffsetPixels.Equals(Config.BaseShadowOffsetPixels, DepthShadowOffsetTolerancePixels)
		&& FMath::IsNearlyEqual(TargetView.ShadowOpacity, Config.BaseShadowOpacity, DepthScalarTolerance);
	const float ResponseSpeed = bReturningToRest ? Config.ReturnSpeed : Config.ResponseSpeed;
	const float Alpha = ComputeExponentialAlpha(ResponseSpeed, SafeDeltaTime);

	CurrentView.bFake3DEnabled = TargetView.bFake3DEnabled;
	CurrentView.bShadowEnabled = TargetView.bShadowEnabled;
	CurrentView.TiltDegrees = FMath::Lerp(CurrentView.TiltDegrees, TargetView.TiltDegrees, Alpha);
	CurrentView.PerspectiveStrength = FMath::Lerp(
		CurrentView.PerspectiveStrength,
		TargetView.PerspectiveStrength,
		Alpha);
	CurrentView.ShadowOffsetPixels = FMath::Lerp(
		CurrentView.ShadowOffsetPixels,
		TargetView.ShadowOffsetPixels,
		Alpha);
	CurrentView.ShadowOpacity = FMath::Lerp(CurrentView.ShadowOpacity, TargetView.ShadowOpacity, Alpha);
	CurrentView.ShadowScale = FMath::Lerp(CurrentView.ShadowScale, TargetView.ShadowScale, Alpha);

	if (!IsInMotion())
	{
		CurrentView = TargetView;
	}
	return CurrentView;
}

void FWacomFirstPersonCardDepthMotion::Reset()
{
	CurrentView = FWacomFirstPersonCardDepthView();
	TargetView = FWacomFirstPersonCardDepthView();
	FilteredPointerVelocity = FVector2D::ZeroVector;
	LastPointerPosition = FVector2D::ZeroVector;
	bHasLastPointerPosition = false;
	bInitialized = false;
	bHasTarget = false;
}

void FWacomFirstPersonCardDepthMotion::InvalidateTarget()
{
	bHasTarget = false;
}

bool FWacomFirstPersonCardDepthMotion::IsInMotion() const
{
	if (!bInitialized || !bHasTarget)
	{
		return true;
	}
	return !CurrentView.TiltDegrees.Equals(TargetView.TiltDegrees, DepthTiltToleranceDegrees)
		|| !CurrentView.ShadowOffsetPixels.Equals(TargetView.ShadowOffsetPixels, DepthShadowOffsetTolerancePixels)
		|| !FMath::IsNearlyEqual(CurrentView.PerspectiveStrength, TargetView.PerspectiveStrength, DepthScalarTolerance)
		|| !FMath::IsNearlyEqual(CurrentView.ShadowOpacity, TargetView.ShadowOpacity, DepthScalarTolerance)
		|| !FMath::IsNearlyEqual(CurrentView.ShadowScale, TargetView.ShadowScale, DepthScalarTolerance)
		|| FilteredPointerVelocity.SizeSquared() > FMath::Square(PointerVelocityTolerance);
}

float FWacomFirstPersonCardDepthMotion::ComputeExponentialAlpha(float Speed, float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return 0.0f;
	}
	return Speed <= 0.0f
		? 1.0f
		: 1.0f - FMath::Exp(-FMath::Max(0.0f, Speed) * DeltaTime);
}

FVector2D FWacomFirstPersonCardDepthMotion::ResolvePointerInCardSpace(
	const FWacomFirstPersonCardDepthMotionInput& Input)
{
	if (!Input.bHasPointerPosition)
	{
		return FVector2D::ZeroVector;
	}

	const FVector2D Delta = Input.PointerPosition - Input.CardCenter;
	const float AngleRadians = FMath::DegreesToRadians(Input.CardRenderAngleDegrees);
	const float CosAngle = FMath::Cos(AngleRadians);
	const float SinAngle = FMath::Sin(AngleRadians);
	const FVector2D LocalDelta(
		CosAngle * Delta.X + SinAngle * Delta.Y,
		-SinAngle * Delta.X + CosAngle * Delta.Y);
	const FVector2D SafeHalfSize = FVector2D(
		FMath::Max(1.0f, Input.CardBodySize.X * 0.5f * FMath::Max(0.01f, Input.CardRenderScale)),
		FMath::Max(1.0f, Input.CardBodySize.Y * 0.5f * FMath::Max(0.01f, Input.CardRenderScale)));
	return FVector2D(
		FMath::Clamp(LocalDelta.X / SafeHalfSize.X, -1.0f, 1.0f),
		FMath::Clamp(LocalDelta.Y / SafeHalfSize.Y, -1.0f, 1.0f));
}

FWacomFirstPersonCardDepthView FWacomFirstPersonCardDepthMotion::MakeNeutralView(
	const FWacomFirstPersonCardDepthConfig& Config,
	bool bProjected)
{
	FWacomFirstPersonCardDepthView View;
	View.bFake3DEnabled = Config.bEnableFake3D && bProjected;
	View.bShadowEnabled = Config.bEnableIndependentShadow && bProjected;
	View.PerspectiveStrength = View.bFake3DEnabled ? FMath::Max(0.0f, Config.PerspectiveStrength) : 0.0f;
	View.ShadowOffsetPixels = Config.BaseShadowOffsetPixels;
	View.ShadowOpacity = View.bShadowEnabled ? FMath::Clamp(Config.BaseShadowOpacity, 0.0f, 1.0f) : 0.0f;
	View.ShadowScale = FMath::Max(0.01f, Config.BaseShadowScale);
	return View;
}

FWacomFirstPersonCardDepthView FWacomFirstPersonCardDepthMotion::BuildTargetView(
	const FWacomFirstPersonCardDepthConfig& Config,
	const FWacomFirstPersonCardDepthMotionInput& Input) const
{
	FWacomFirstPersonCardDepthView View = MakeNeutralView(Config, Input.bProjected);
	if (!Input.bProjected || Input.bFlattenForSemanticTransition)
	{
		return View;
	}

	float MaxTiltDegrees = FMath::Max(0.0f, Config.HoverMaxTiltDegrees);
	float ShadowBlend = 0.0f;
	if (Input.bDragging)
	{
		MaxTiltDegrees = FMath::Max(0.0f, Config.DragMaxTiltDegrees);
		const float VelocityForMaxTilt = FMath::Max(1.0f, Config.DragVelocityForMaxTiltPixelsPerSecond);
		const FVector2D NormalizedVelocity(
			FMath::Clamp(FilteredPointerVelocity.X / VelocityForMaxTilt, -1.0f, 1.0f),
			FMath::Clamp(FilteredPointerVelocity.Y / VelocityForMaxTilt, -1.0f, 1.0f));
		View.TiltDegrees = FVector2D(
			NormalizedVelocity.Y * MaxTiltDegrees,
			-NormalizedVelocity.X * MaxTiltDegrees);
		View.ShadowOffsetPixels = Config.DragShadowOffsetPixels;
		View.ShadowOpacity = FMath::Clamp(Config.DragShadowOpacity, 0.0f, 1.0f);
		View.ShadowScale = FMath::Max(0.01f, Config.DragShadowScale);
		ShadowBlend = 1.0f;
	}
	else if (Input.bHovered || Input.bPressed)
	{
		const FVector2D PointerInCardSpace = ResolvePointerInCardSpace(Input);
		View.TiltDegrees = FVector2D(
			-PointerInCardSpace.Y * MaxTiltDegrees,
			PointerInCardSpace.X * MaxTiltDegrees);
		ShadowBlend = Input.bPressed
			? FMath::Clamp(Config.PressedTiltMultiplier, 0.0f, 1.0f)
			: 1.0f;
		View.TiltDegrees *= ShadowBlend;
		View.ShadowOffsetPixels = FMath::Lerp(
			Config.BaseShadowOffsetPixels,
			Config.HoverShadowOffsetPixels,
			ShadowBlend);
		View.ShadowOpacity = FMath::Lerp(
			FMath::Clamp(Config.BaseShadowOpacity, 0.0f, 1.0f),
			FMath::Clamp(Config.HoverShadowOpacity, 0.0f, 1.0f),
			ShadowBlend);
		View.ShadowScale = FMath::Lerp(
			FMath::Max(0.01f, Config.BaseShadowScale),
			FMath::Max(0.01f, Config.HoverShadowScale),
			ShadowBlend);
	}

	if (!View.bFake3DEnabled)
	{
		View.TiltDegrees = FVector2D::ZeroVector;
		View.PerspectiveStrength = 0.0f;
	}
	if (!View.bShadowEnabled)
	{
		View.ShadowOpacity = 0.0f;
	}
	else if (MaxTiltDegrees > KINDA_SMALL_NUMBER && ShadowBlend > 0.0f)
	{
		const FVector2D NormalizedTilt = View.TiltDegrees / MaxTiltDegrees;
		View.ShadowOffsetPixels += FVector2D(-NormalizedTilt.Y, NormalizedTilt.X)
			* FMath::Max(0.0f, Config.ShadowTiltInfluencePixels);
	}
	return View;
}

void FWacomFirstPersonCardDepthMotion::UpdateFilteredPointerVelocity(
	const FWacomFirstPersonCardDepthConfig& Config,
	const FWacomFirstPersonCardDepthMotionInput& Input,
	float DeltaTime)
{
	FVector2D RawVelocity = FVector2D::ZeroVector;
	if (Input.bHasPointerPosition)
	{
		if (bHasLastPointerPosition && Input.bPointerPositionChanged && DeltaTime > SMALL_NUMBER)
		{
			RawVelocity = (Input.PointerPosition - LastPointerPosition) / DeltaTime;
		}
		LastPointerPosition = Input.PointerPosition;
		bHasLastPointerPosition = true;
	}
	else
	{
		bHasLastPointerPosition = false;
	}

	const float VelocityAlpha = ComputeExponentialAlpha(Config.DragVelocityFilterSpeed, DeltaTime);
	FilteredPointerVelocity = FMath::Lerp(FilteredPointerVelocity, RawVelocity, VelocityAlpha);
}
