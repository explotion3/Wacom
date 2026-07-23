// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UWidgetComponent;

/**
 * App-private world-card material contract shared by diagnostic and production
 * WidgetComponents. It owns no Run state and exposes no gameplay API.
 */
class WACOMAPP_API FWacomWorldCardSurfaceMaterialAdapter
{
public:
	static const TCHAR* GetMaterialPath();
	static FName GetExposureStrengthParameterName();
	static float GetProductionExposureStrength();

	static UMaterialInterface* ResolveMaterial();
	static bool Apply(UWidgetComponent& Component, float ExposureStrength);
	static bool ApplyResolvedMaterial(
		UWidgetComponent& Component,
		UMaterialInterface* Material,
		float ExposureStrength);
};
