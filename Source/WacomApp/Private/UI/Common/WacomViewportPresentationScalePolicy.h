// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FWacomViewportPresentationScaleProfile
{
	FVector2D ReferenceViewportPixels = FVector2D(1920.0f, 1080.0f);
	float MinimumTargetPhysicalScale = 1.0f;
	float MaximumTargetPhysicalScale = 1.0f;
	float MinimumLocalScale = 1.0f;
	float MaximumLocalScale = 1.0f;
};

struct FWacomViewportPresentationScaleResult
{
	float TargetPhysicalScale = 1.0f;
	float LocalScale = 1.0f;
};

/**
 * Shared math for presentation systems that author against a reference viewport
 * while compensating for Unreal's global UI DPI exactly once.
 */
class WACOMAPP_API FWacomViewportPresentationScalePolicy
{
public:
	static FWacomViewportPresentationScaleResult Resolve(
		const FVector2D& ViewportPixelSize,
		float GlobalUIScale,
		const FWacomViewportPresentationScaleProfile& Profile);
};
