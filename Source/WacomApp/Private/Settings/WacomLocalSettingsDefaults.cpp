// Copyright Wacom. All Rights Reserved.

#include "Settings/WacomLocalSettingsDefaults.h"

#include "Settings/WacomGameUserSettings.h"

namespace
{
	const FIntPoint SafeFallbackResolution(1280, 720);

	bool IsUsableResolution(const FIntPoint Resolution)
	{
		return Resolution.X > 0 && Resolution.Y > 0;
	}

	FIntPoint ResolveDefaultResolution(const UWacomGameUserSettings* Settings)
	{
		if (!Settings)
		{
			return SafeFallbackResolution;
		}

		const FIntPoint DesktopResolution = Settings->GetDesktopResolution();
		if (IsUsableResolution(DesktopResolution))
		{
			return DesktopResolution;
		}

		const FIntPoint CurrentResolution = Settings->GetScreenResolution();
		return IsUsableResolution(CurrentResolution)
			? CurrentResolution
			: SafeFallbackResolution;
	}
}

FWacomLocalSettingsSnapshot FWacomLocalSettingsDefaults::Build(
	const UWacomGameUserSettings* Settings)
{
	FWacomLocalSettingsSnapshot Defaults;
	Defaults.ScreenResolution = ResolveDefaultResolution(Settings);
	Defaults.WindowMode = EWindowMode::WindowedFullscreen;
	Defaults.bVSyncEnabled = true;
	Defaults.FrameRateLimit = 60.0f;
	Defaults.GraphicsQuality = 2;
	Defaults.MasterVolume = 1.0f;
	Defaults.MusicVolume = 1.0f;
	Defaults.SFXVolume = 1.0f;
	Defaults.UISoundVolume = 1.0f;
	Defaults.LookResponseStrength = 1.0f;
	Defaults.bInvertLookY = false;
	Defaults.CameraMotionStrength = 1.0f;
	Defaults.FlashEffectMode = EWacomFlashEffectMode::Full;
	Defaults.UIMotionMode = EWacomUIMotionMode::Full;
	Defaults.Sanitize();
	return Defaults;
}
