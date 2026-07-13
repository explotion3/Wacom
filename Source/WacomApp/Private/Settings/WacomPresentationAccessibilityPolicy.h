// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Settings/WacomLocalSettingsTypes.h"

struct FWacomFirstPersonCardResolvedLayoutConfig;

/** Effective, runtime-only presentation policy derived from local settings. */
struct WACOMAPP_API FWacomPresentationAccessibilityPolicy
{
	static float GetDecorativeFlashIntensityScale(EWacomFlashEffectMode Mode);
	static bool UsesSimplifiedMotion(EWacomUIMotionMode Mode);
	static void ApplyToFirstPersonCardConfig(
		FWacomFirstPersonCardResolvedLayoutConfig& Config,
		float DecorativeFlashIntensityScale,
		bool bSimplifiedMotion);
};
