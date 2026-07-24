// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPresentationScalePolicy.h"
#include "UI/Common/WacomViewportPresentationScalePolicy.h"

namespace
{
	FWacomViewportPresentationScaleProfile MakeFirstPersonCardProfile()
	{
		FWacomViewportPresentationScaleProfile Profile;
		Profile.ReferenceViewportPixels = FVector2D(2560.0f, 1440.0f);
		Profile.MinimumTargetPhysicalScale = 0.5f;
		Profile.MaximumTargetPhysicalScale = 1.0f;
		Profile.MinimumLocalScale = 0.5f;
		Profile.MaximumLocalScale = 1.0f;
		return Profile;
	}
}

FWacomFirstPersonCardPresentationScaleResult
FWacomFirstPersonCardPresentationScalePolicy::Resolve(
	const FVector2D& ViewportPixelSize,
	float GlobalUIScale)
{
	FWacomFirstPersonCardPresentationScaleResult Result;
	const FWacomViewportPresentationScaleResult SharedResult =
		FWacomViewportPresentationScalePolicy::Resolve(
			ViewportPixelSize,
			GlobalUIScale,
			MakeFirstPersonCardProfile());
	Result.TargetPhysicalScale = SharedResult.TargetPhysicalScale;
	Result.PresentationScale = SharedResult.LocalScale;
	return Result;
}
