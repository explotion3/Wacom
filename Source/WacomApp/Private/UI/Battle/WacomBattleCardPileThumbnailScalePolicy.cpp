// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCardPileThumbnailScalePolicy.h"
#include "UI/Card/WacomFirstPersonCardPresentationMetrics.h"
#include "UI/Card/WacomFirstPersonCardPresentationScalePolicy.h"
#include "UI/Common/WacomViewportPresentationScalePolicy.h"

FWacomBattleCardPileThumbnailScaleResult
FWacomBattleCardPileThumbnailScalePolicy::Resolve(
	const FVector2D& ViewportPixelSize,
	float GlobalUIScale,
	const FVector2D& ReferenceViewportPixels,
	float MinimumTargetPhysicalScale,
	float MaximumTargetPhysicalScale)
{
	FWacomViewportPresentationScaleProfile Profile;
	Profile.ReferenceViewportPixels = ReferenceViewportPixels;
	Profile.MinimumTargetPhysicalScale = MinimumTargetPhysicalScale;
	Profile.MaximumTargetPhysicalScale = MaximumTargetPhysicalScale;
	Profile.MinimumLocalScale = 0.5f;
	Profile.MaximumLocalScale = 2.0f;

	const FWacomViewportPresentationScaleResult SharedResult =
		FWacomViewportPresentationScalePolicy::Resolve(
			ViewportPixelSize,
			GlobalUIScale,
			Profile);

	FWacomBattleCardPileThumbnailScaleResult Result;
	Result.TargetPhysicalScale = SharedResult.TargetPhysicalScale;
	Result.LocalScale = SharedResult.LocalScale;
	return Result;
}

FWacomBattleCardPileHandSizeMatchResult
FWacomBattleCardPileThumbnailScalePolicy::ResolveMatchingRestingHand(
	const FWacomFirstPersonCardRestingPresentationProfile& Profile,
	const FVector2D& ViewportPixelSize,
	float GlobalUIScale)
{
	FWacomBattleCardPileHandSizeMatchResult Result;
	if (!Profile.IsValid()
		|| !FMath::IsFinite(GlobalUIScale)
		|| GlobalUIScale <= 0.0f)
	{
		return Result;
	}

	const FWacomFirstPersonCardPresentationScaleResult HandScale =
		FWacomFirstPersonCardPresentationScalePolicy::Resolve(
			ViewportPixelSize,
			GlobalUIScale);
	Result.PhysicalCardBodySize =
		Profile.AuthoredCardBodySize
		* Profile.AuthoredRenderScale
		* HandScale.TargetPhysicalScale;
	Result.LogicalCardBodySize = Result.PhysicalCardBodySize / GlobalUIScale;
	Result.bValid = Result.PhysicalCardBodySize.X > 0.0f
		&& Result.PhysicalCardBodySize.Y > 0.0f
		&& Result.LogicalCardBodySize.X > 0.0f
		&& Result.LogicalCardBodySize.Y > 0.0f;
	return Result;
}
