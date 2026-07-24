// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Read-only authoring profile for the resting first-person hand card body.
 * It deliberately excludes hover, selection, fan rotation and transient feedback.
 */
struct WACOMAPP_API FWacomFirstPersonCardRestingPresentationProfile
{
	FVector2D AuthoredCardBodySize = FVector2D(296.0f, 420.0f);
	float AuthoredRenderScale = 0.0f;

	bool IsValid() const
	{
		return FMath::IsFinite(AuthoredCardBodySize.X)
			&& FMath::IsFinite(AuthoredCardBodySize.Y)
			&& AuthoredCardBodySize.X > 0.0f
			&& AuthoredCardBodySize.Y > 0.0f
			&& FMath::IsFinite(AuthoredRenderScale)
			&& AuthoredRenderScale > 0.0f;
	}
};
