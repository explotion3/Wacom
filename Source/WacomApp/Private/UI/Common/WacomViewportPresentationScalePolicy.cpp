// Copyright Wacom. All Rights Reserved.

#include "UI/Common/WacomViewportPresentationScalePolicy.h"

namespace
{
	constexpr float StableScalePrecision = 1000.0f;

	float Stabilize(float Value)
	{
		return FMath::RoundToFloat(Value * StableScalePrecision) / StableScalePrecision;
	}

	bool IsValidProfile(const FWacomViewportPresentationScaleProfile& Profile)
	{
		return FMath::IsFinite(Profile.ReferenceViewportPixels.X)
			&& FMath::IsFinite(Profile.ReferenceViewportPixels.Y)
			&& Profile.ReferenceViewportPixels.X > 0.0f
			&& Profile.ReferenceViewportPixels.Y > 0.0f
			&& FMath::IsFinite(Profile.MinimumTargetPhysicalScale)
			&& FMath::IsFinite(Profile.MaximumTargetPhysicalScale)
			&& Profile.MinimumTargetPhysicalScale > 0.0f
			&& Profile.MaximumTargetPhysicalScale >= Profile.MinimumTargetPhysicalScale
			&& FMath::IsFinite(Profile.MinimumLocalScale)
			&& FMath::IsFinite(Profile.MaximumLocalScale)
			&& Profile.MinimumLocalScale > 0.0f
			&& Profile.MaximumLocalScale >= Profile.MinimumLocalScale;
	}
}

FWacomViewportPresentationScaleResult
FWacomViewportPresentationScalePolicy::Resolve(
	const FVector2D& ViewportPixelSize,
	float GlobalUIScale,
	const FWacomViewportPresentationScaleProfile& Profile)
{
	FWacomViewportPresentationScaleResult Result;
	if (!IsValidProfile(Profile)
		|| !FMath::IsFinite(ViewportPixelSize.X)
		|| !FMath::IsFinite(ViewportPixelSize.Y)
		|| ViewportPixelSize.X <= 0.0f
		|| ViewportPixelSize.Y <= 0.0f
		|| !FMath::IsFinite(GlobalUIScale)
		|| GlobalUIScale <= 0.0f)
	{
		return Result;
	}

	const float TargetPhysicalScale = FMath::Clamp(
		FMath::Min(
			ViewportPixelSize.X / Profile.ReferenceViewportPixels.X,
			ViewportPixelSize.Y / Profile.ReferenceViewportPixels.Y),
		Profile.MinimumTargetPhysicalScale,
		Profile.MaximumTargetPhysicalScale);
	const float LocalScale = FMath::Clamp(
		TargetPhysicalScale / GlobalUIScale,
		Profile.MinimumLocalScale,
		Profile.MaximumLocalScale);

	Result.TargetPhysicalScale = Stabilize(TargetPhysicalScale);
	Result.LocalScale = Stabilize(LocalScale);
	return Result;
}
