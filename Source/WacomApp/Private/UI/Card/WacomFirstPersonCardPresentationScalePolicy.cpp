// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPresentationScalePolicy.h"

namespace
{
	constexpr float CardReferenceWidth = 2560.0f;
	constexpr float CardReferenceHeight = 1440.0f;
	constexpr float MinimumPhysicalScale = 0.5f;
	constexpr float MaximumPresentationScale = 1.0f;
	constexpr float StableScalePrecision = 1000.0f;
}

FWacomFirstPersonCardPresentationScaleResult
FWacomFirstPersonCardPresentationScalePolicy::Resolve(
	const FVector2D& ViewportPixelSize,
	float GlobalUIScale)
{
	FWacomFirstPersonCardPresentationScaleResult Result;
	if (!FMath::IsFinite(ViewportPixelSize.X)
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
			ViewportPixelSize.X / CardReferenceWidth,
			ViewportPixelSize.Y / CardReferenceHeight),
		MinimumPhysicalScale,
		MaximumPresentationScale);
	const float PresentationScale = FMath::Clamp(
		TargetPhysicalScale / GlobalUIScale,
		MinimumPhysicalScale,
		MaximumPresentationScale);

	Result.TargetPhysicalScale = Stabilize(TargetPhysicalScale);
	Result.PresentationScale = Stabilize(PresentationScale);
	return Result;
}

float FWacomFirstPersonCardPresentationScalePolicy::Stabilize(float Value)
{
	return FMath::RoundToFloat(Value * StableScalePrecision) / StableScalePrecision;
}
