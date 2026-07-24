// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWacomFirstPersonCardPresentationScaleResult
{
	float TargetPhysicalScale = 1.0f;
	float PresentationScale = 1.0f;
};

/**
 * Resolves the project-wide first-person card presentation scale without
 * changing the global UI DPI rule or any authored card asset.
 */
class WACOMAPP_API FWacomFirstPersonCardPresentationScalePolicy
{
public:
	static FWacomFirstPersonCardPresentationScaleResult Resolve(
		const FVector2D& ViewportPixelSize,
		float GlobalUIScale);
};
