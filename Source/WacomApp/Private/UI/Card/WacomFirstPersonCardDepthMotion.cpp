// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardDepthMotion.h"

#include "UI/Card/WacomCardMotionKernel.h"

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
		FilteredSurfaceStrength = CurrentView.SurfacePerspective.Strength;
		bInitialized = true;
	}

	const bool bReturningToRest =
		TargetView.TiltDegrees.IsNearlyZero(DepthTiltToleranceDegrees)
		&& FMath::IsNearlyZero(TargetView.ContactShadowLift, ContactShadowLiftTolerance);
	const float ResponseSpeed = bReturningToRest ? Config.ReturnSpeed : Config.ResponseSpeed;
	const float Alpha = FWacomCardMotionKernel::ComputeExponentialAlpha(ResponseSpeed, SafeDeltaTime);

	CurrentView.bFake3DEnabled = TargetView.bFake3DEnabled;
	CurrentView.TiltDegrees = FMath::Lerp(CurrentView.TiltDegrees, TargetView.TiltDegrees, Alpha);
	CurrentView.PerspectiveStrength = FMath::Lerp(
		CurrentView.PerspectiveStrength,
		TargetView.PerspectiveStrength,
		Alpha);
	CurrentView.bContactShadowEnabled = TargetView.bContactShadowEnabled;
	CurrentView.ContactShadowOpacityMultiplier = TargetView.ContactShadowOpacityMultiplier;
	CurrentView.ContactShadowLift = FMath::Lerp(
		CurrentView.ContactShadowLift,
		TargetView.ContactShadowLift,
		Alpha);
	PopulateContactShadowOffset(Config, CurrentView);

	const bool bSurfaceReturningToRest = !TargetView.SurfacePerspective.bEnabled
		|| CurrentView.TiltDegrees.IsNearlyZero(DepthTiltToleranceDegrees);
	const float SurfaceResponseSpeed = bSurfaceReturningToRest
		? Config.SurfaceParallaxReturnSpeed
		: Config.SurfaceParallaxResponseSpeed;
	const float SurfaceAlpha = FWacomCardMotionKernel::ComputeExponentialAlpha(
		SurfaceResponseSpeed,
		SafeDeltaTime);
	const FVector2D SurfaceTiltTarget = TargetView.SurfacePerspective.bEnabled
		? CurrentView.TiltDegrees
		: FVector2D::ZeroVector;
	FilteredSurfaceTiltDegrees = FMath::Lerp(
		FilteredSurfaceTiltDegrees,
		SurfaceTiltTarget,
		SurfaceAlpha);
	const float SurfaceStrengthTarget = TargetView.SurfacePerspective.bEnabled
		? TargetView.SurfacePerspective.Strength
		: 0.0f;
	FilteredSurfaceStrength = FMath::Lerp(
		FilteredSurfaceStrength,
		SurfaceStrengthTarget,
		SurfaceAlpha);
	PopulateSurfacePerspective(
		Config,
		CurrentView,
		FilteredSurfaceTiltDegrees,
		FilteredSurfaceStrength,
		Input.bProjected);

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
	FilteredSurfaceTiltDegrees = FVector2D::ZeroVector;
	FilteredSurfaceStrength = 0.0f;
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
		|| !FilteredSurfaceTiltDegrees.Equals(
			CurrentView.SurfacePerspective.bEnabled
				? CurrentView.TiltDegrees
				: FVector2D::ZeroVector,
			DepthTiltToleranceDegrees)
		|| !FMath::IsNearlyEqual(
			FilteredSurfaceStrength,
			TargetView.SurfacePerspective.bEnabled
				? TargetView.SurfacePerspective.Strength
				: 0.0f,
			DepthScalarTolerance)
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
	View.ContactShadowOpacityMultiplier = FMath::Max(0.0f, Config.ContactShadowOpacityMultiplier);
	PopulateContactShadowOffset(Config, View);
	PopulateSurfacePerspective(
		Config,
		View,
		FVector2D::ZeroVector,
		Config.bEnableSurfaceParallax && bProjected && !Config.bReduceSurfaceParallaxMotion
			? FMath::Max(0.0f, Config.SurfaceParallaxStrength)
			: 0.0f,
		bProjected);
	return View;
}

void FWacomFirstPersonCardDepthMotion::PopulateContactShadowOffset(
	const FWacomFirstPersonCardDepthConfig& Config,
	FWacomFirstPersonCardDepthView& View)
{
	View.ContactShadowOffsetPixels = FVector2D::ZeroVector;
	if (!View.bContactShadowEnabled || !View.bFake3DEnabled)
	{
		return;
	}

	const float ReferenceTiltDegrees = FMath::Max(
		1.0f,
		FMath::Max(Config.HoverMaxTiltDegrees, Config.DragMaxTiltDegrees));
	const float ReferenceTangent = FMath::Max(
		0.001f,
		FMath::Tan(FMath::DegreesToRadians(ReferenceTiltDegrees)));
	const float MaxOffsetPixels = FMath::Max(0.0f, Config.ContactShadowTiltOffsetPixels);
	const FVector2D SurfaceDirection(
		FMath::Tan(FMath::DegreesToRadians(View.TiltDegrees.Y)) / ReferenceTangent,
		-FMath::Tan(FMath::DegreesToRadians(View.TiltDegrees.X)) / ReferenceTangent);

	// The card face leans toward SurfaceDirection; its contact shadow falls away from it.
	View.ContactShadowOffsetPixels =
		(-SurfaceDirection * MaxOffsetPixels).GetClampedToMaxSize(MaxOffsetPixels);
}

void FWacomFirstPersonCardDepthMotion::PopulateSurfacePerspective(
	const FWacomFirstPersonCardDepthConfig& Config,
	FWacomFirstPersonCardDepthView& View,
	const FVector2D& SurfaceTiltDegrees,
	float SurfaceStrength,
	bool bProjected)
{
	FWacomCardSurfacePerspectiveView& SurfaceView = View.SurfacePerspective;
	SurfaceView.bEnabled = Config.bEnableSurfaceParallax
		&& View.bFake3DEnabled
		&& !Config.bReduceSurfaceParallaxMotion;
	SurfaceView.bReducedMotion = Config.bReduceSurfaceParallaxMotion;
	SurfaceView.Strength = SurfaceView.bEnabled ? FMath::Max(0.0f, SurfaceStrength) : 0.0f;
	SurfaceView.TiltDegrees = SurfaceView.bEnabled
		? SurfaceTiltDegrees
		: FVector2D::ZeroVector;
	SurfaceView.bAttachmentCastShadowEnabled =
		Config.bEnableAttachmentCastShadow && bProjected;
	SurfaceView.AttachmentCastShadowColor = Config.AttachmentCastShadowColor;
	SurfaceView.AttachmentCastShadowOpacity = SurfaceView.bAttachmentCastShadowEnabled
		? FMath::Clamp(Config.AttachmentCastShadowOpacity, 0.0f, 1.0f)
		: 0.0f;

	if (!SurfaceView.bEnabled)
	{
		SurfaceView.AttachmentOffsetPixels = FVector2D::ZeroVector;
	}
	else
	{
		const float ReferenceTiltDegrees = FMath::Max(
			1.0f,
			FMath::Max(Config.HoverMaxTiltDegrees, Config.DragMaxTiltDegrees));
		const float ReferenceTangent = FMath::Max(
			0.001f,
			FMath::Tan(FMath::DegreesToRadians(ReferenceTiltDegrees)));
		const float DepthPixels = FMath::Max(0.0f, Config.AttachmentParallaxDepthPixels);
		FVector2D Offset(
			FMath::Tan(FMath::DegreesToRadians(SurfaceView.TiltDegrees.Y)) / ReferenceTangent * DepthPixels,
			-FMath::Tan(FMath::DegreesToRadians(SurfaceView.TiltDegrees.X)) / ReferenceTangent * DepthPixels);
		Offset *= SurfaceView.Strength;
		SurfaceView.AttachmentOffsetPixels = Offset.GetClampedToMaxSize(
			FMath::Max(0.0f, Config.AttachmentParallaxMaxOffsetPixels));
	}

	const FVector2D DynamicCounterOffset = Config.bReduceSurfaceParallaxMotion
		? FVector2D::ZeroVector
		: -SurfaceView.AttachmentOffsetPixels
			* FMath::Max(0.0f, Config.AttachmentCastShadowCounterMotionRatio);
	SurfaceView.AttachmentCastShadowOffsetPixels =
		(Config.AttachmentCastShadowStaticOffsetPixels + DynamicCounterOffset)
		.GetClampedToMaxSize(FMath::Max(0.0f, Config.AttachmentCastShadowMaxOffsetPixels));
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
	else if (Input.bHovered || Input.bPressed || Input.PressedFeedbackAmount > KINDA_SMALL_NUMBER)
	{
		const float PressedAmount = FMath::Clamp(Input.PressedFeedbackAmount, 0.0f, 1.0f);
		if (View.bFake3DEnabled)
		{
			const float MaxTiltDegrees = FMath::Max(0.0f, Config.HoverMaxTiltDegrees);
			const FVector2D PointerInCardSpace = ResolvePointerInCardSpace(Input);
			View.TiltDegrees = FVector2D(
				-PointerInCardSpace.Y * MaxTiltDegrees,
				PointerInCardSpace.X * MaxTiltDegrees);
			View.TiltDegrees *= FMath::Lerp(
				1.0f,
				FMath::Clamp(Config.PressedTiltMultiplier, 0.0f, 1.0f),
				PressedAmount);
		}
		if (View.bContactShadowEnabled)
		{
			const float PressedLiftMultiplier = FMath::Lerp(
				1.0f,
				FMath::Clamp(Input.PressedContactShadowLiftMultiplier, 0.0f, 1.0f),
				PressedAmount);
			View.ContactShadowLift = FMath::Clamp(
				Config.HoverContactShadowLift * PressedLiftMultiplier,
				0.0f,
				1.0f);
		}
	}
	PopulateContactShadowOffset(Config, View);
	const float StateStrengthMultiplier = Input.bDragging
		? FMath::Max(0.0f, Config.DragSurfaceParallaxStrengthMultiplier)
		: 1.0f;
	PopulateSurfacePerspective(
		Config,
		View,
		View.TiltDegrees,
		FMath::Max(0.0f, Config.SurfaceParallaxStrength) * StateStrengthMultiplier,
		Input.bProjected);
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

	const float VelocityAlpha = FWacomCardMotionKernel::ComputeExponentialAlpha(
		Config.DragVelocityFilterSpeed,
		DeltaTime);
	FilteredPointerVelocity = FMath::Lerp(FilteredPointerVelocity, RawVelocity, VelocityAlpha);
}
