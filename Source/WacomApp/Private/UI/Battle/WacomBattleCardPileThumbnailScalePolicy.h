// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWacomFirstPersonCardRestingPresentationProfile;

struct FWacomBattleCardPileThumbnailScaleResult
{
	float TargetPhysicalScale = 1.0f;
	float LocalScale = 1.0f;
};

struct FWacomBattleCardPileHandSizeMatchResult
{
	FVector2D PhysicalCardBodySize = FVector2D::ZeroVector;
	FVector2D LogicalCardBodySize = FVector2D::ZeroVector;
	bool bValid = false;
};

/** Responsive scale wrapper owned by the Battle pile browser presentation. */
class WACOMAPP_API FWacomBattleCardPileThumbnailScalePolicy
{
public:
	static FWacomBattleCardPileThumbnailScaleResult Resolve(
		const FVector2D& ViewportPixelSize,
		float GlobalUIScale,
		const FVector2D& ReferenceViewportPixels = FVector2D(1920.0f, 1080.0f),
		float MinimumTargetPhysicalScale = 0.90f,
		float MaximumTargetPhysicalScale = 1.15f);

	static FWacomBattleCardPileHandSizeMatchResult ResolveMatchingRestingHand(
		const FWacomFirstPersonCardRestingPresentationProfile& Profile,
		const FVector2D& ViewportPixelSize,
		float GlobalUIScale);
};
