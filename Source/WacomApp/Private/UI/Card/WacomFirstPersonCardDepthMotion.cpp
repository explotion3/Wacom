// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDepthMotion.h"

namespace
{
	constexpr float DepthTiltToleranceDegrees = 0.01f;
	constexpr float DepthScalarTolerance = 0.001f;
	constexpr float ContactShadowLiftTolerance = 0.001f;
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

	const bool bReturningToRest =
		TargetView.TiltDegrees.IsNearlyZero(DepthTiltToleranceDegrees)
		&& FMath::IsNearlyZero(TargetView.ContactShadowLift, ContactShadowLiftTolerance);
	const float ResponseSpeed = bReturningToRest ? Config.ReturnSpeed : Config.ResponseSpeed;
	const float Alpha = ComputeExponentialAlpha(ResponseSpeed, SafeDeltaTime);

	CurrentView.bFake3DEnabled = TargetView.bFake3DEnabled;
	CurrentView.TiltDegrees = FMath::Lerp(CurrentView.TiltDegrees, TargetView.TiltDegrees, Alpha);
	CurrentView.PerspectiveStrength = FMath::Lerp(
		CurrentView.PerspectiveStrength,
		TargetView.PerspectiveStrength,
		Alpha);
	CurrentView.bContactShadowEnabled = TargetView.bContactShadowEnabled;
	CurrentView.ContactShadowLift = FMath::Lerp(
		CurrentView.ContactShadowLift,
		TargetView.ContactShadowLift,
		Alpha);
	PopulateSurfacePerspective(Config, CurrentView);

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
		|| !FMath::IsNearlyEqual(
			CurrentView.PerspectiveStrength,
			TargetView.PerspectiveStrength,
			DepthScalarTolerance)
		|| !FMath::IsNearlyEqual(
			CurrentView.ContactShadowLift,
			TargetView.ContactShadowLift,
			ContactShadowLiftTolerance)
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
	const FVector2D SafeHalfSize(
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
	View.PerspectiveStrength = View.bFake3DEnabled
		? FMath::Max(0.0f, Config.PerspectiveStrength)
		: 0.0f;
	View.bContactShadowEnabled = Config.bEnableContactShadow && bProjected;
	PopulateSurfacePerspective(Config, View);
	return View;
}

void FWacomFirstPersonCardDepthMotion::PopulateSurfacePerspective(
	const FWacomFirstPersonCardDepthConfig& Config,
	FWacomFirstPersonCardDepthView& View)
{
	FWacomCardSurfacePerspectiveView& SurfaceView = View.SurfacePerspective;
	SurfaceView.bEnabled = Config.bEnableSurfaceParallax
		&& View.bFake3DEnabled
		&& !Config.bReduceSurfaceParallaxMotion;
	SurfaceView.bReducedMotion = Config.bReduceSurfaceParallaxMotion;
	SurfaceView.Strength = SurfaceView.bEnabled
		? FMath::Max(0.0f, Config.SurfaceParallaxStrength)
		: 0.0f;
	SurfaceView.TiltDegrees = SurfaceView.bEnabled
		? View.TiltDegrees
		: FVector2D::ZeroVector;

	if (!SurfaceView.bEnabled)
	{
		SurfaceView.AttachmentOffsetPixels = FVector2D::ZeroVector;
		return;
	}

	const float ReferenceTiltDegrees = FMath::Max(
		1.0f,
		FMath::Max(Config.HoverMaxTiltDegrees, Config.DragMaxTiltDegrees));
	const float ReferenceTangent = FMath::Max(
		0.001f,
		FMath::Tan(FMath::DegreesToRadians(ReferenceTiltDegrees)));
	const float DepthPixels = FMath::Max(0.0f, Config.AttachmentParallaxDepthPixels);
	FVector2D Offset(
		FMath::Tan(FMath::DegreesToRadians(View.TiltDegrees.Y)) / ReferenceTangent * DepthPixels,
		-FMath::Tan(FMath::DegreesToRadians(View.TiltDegrees.X)) / ReferenceTangent * DepthPixels);
	Offset *= SurfaceView.Strength;
	SurfaceView.AttachmentOffsetPixels = Offset.GetClampedToMaxSize(
		FMath::Max(0.0f, Config.AttachmentParallaxMaxOffsetPixels));
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

	if (Input.bDragging)
	{
		if (View.bFake3DEnabled)
		{
			const float MaxTiltDegrees = FMath::Max(0.0f, Config.DragMaxTiltDegrees);
			const float VelocityForMaxTilt =
				FMath::Max(1.0f, Config.DragVelocityForMaxTiltPixelsPerSecond);
			const FVector2D NormalizedVelocity(
				FMath::Clamp(FilteredPointerVelocity.X / VelocityForMaxTilt, -1.0f, 1.0f),
				FMath::Clamp(FilteredPointerVelocity.Y / VelocityForMaxTilt, -1.0f, 1.0f));
			View.TiltDegrees = FVector2D(
				NormalizedVelocity.Y * MaxTiltDegrees,
				-NormalizedVelocity.X * MaxTiltDegrees);
		}
		if (View.bContactShadowEnabled)
		{
			View.ContactShadowLift = FMath::Clamp(Config.DragContactShadowLift, 0.0f, 1.0f);
		}
	}
	else if (Input.bHovered || Input.bPressed)
	{
		if (View.bFake3DEnabled)
		{
			const float MaxTiltDegrees = FMath::Max(0.0f, Config.HoverMaxTiltDegrees);
			const FVector2D PointerInCardSpace = ResolvePointerInCardSpace(Input);
			View.TiltDegrees = FVector2D(
				-PointerInCardSpace.Y * MaxTiltDegrees,
				PointerInCardSpace.X * MaxTiltDegrees);
			if (Input.bPressed)
			{
				View.TiltDegrees *= FMath::Clamp(Config.PressedTiltMultiplier, 0.0f, 1.0f);
			}
		}
		if (View.bContactShadowEnabled)
		{
			View.ContactShadowLift = FMath::Clamp(Config.HoverContactShadowLift, 0.0f, 1.0f);
		}
	}
	PopulateSurfacePerspective(Config, View);
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
