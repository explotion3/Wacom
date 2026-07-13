// Copyright Wacom. All Rights Reserved.

#include "Settings/WacomLocalSettingsTypes.h"

void FWacomLocalSettingsSnapshot::Sanitize()
{
	ScreenResolution.X = FMath::Max(1, ScreenResolution.X);
	ScreenResolution.Y = FMath::Max(1, ScreenResolution.Y);
	if (WindowMode != EWindowMode::Fullscreen
		&& WindowMode != EWindowMode::WindowedFullscreen
		&& WindowMode != EWindowMode::Windowed)
	{
		WindowMode = EWindowMode::WindowedFullscreen;
	}
	FrameRateLimit = FMath::Max(0.0f, FrameRateLimit);
	GraphicsQuality = FMath::Clamp(GraphicsQuality, 0, 3);
	MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
	MusicVolume = FMath::Clamp(MusicVolume, 0.0f, 1.0f);
	SFXVolume = FMath::Clamp(SFXVolume, 0.0f, 1.0f);
	UISoundVolume = FMath::Clamp(UISoundVolume, 0.0f, 1.0f);
	LookResponseStrength = FMath::Clamp(LookResponseStrength, 0.0f, 3.0f);
	CameraMotionStrength = FMath::Clamp(CameraMotionStrength, 0.0f, 1.0f);
	if (FlashEffectMode != EWacomFlashEffectMode::Full
		&& FlashEffectMode != EWacomFlashEffectMode::Reduced
		&& FlashEffectMode != EWacomFlashEffectMode::Off)
	{
		FlashEffectMode = EWacomFlashEffectMode::Full;
	}
	if (UIMotionMode != EWacomUIMotionMode::Full
		&& UIMotionMode != EWacomUIMotionMode::Simplified)
	{
		UIMotionMode = EWacomUIMotionMode::Full;
	}
}

bool FWacomLocalSettingsSnapshot::IsEquivalentTo(const FWacomLocalSettingsSnapshot& Other) const
{
	return ScreenResolution == Other.ScreenResolution
		&& WindowMode == Other.WindowMode
		&& bVSyncEnabled == Other.bVSyncEnabled
		&& FMath::IsNearlyEqual(FrameRateLimit, Other.FrameRateLimit)
		&& GraphicsQuality == Other.GraphicsQuality
		&& FMath::IsNearlyEqual(MasterVolume, Other.MasterVolume)
		&& FMath::IsNearlyEqual(MusicVolume, Other.MusicVolume)
		&& FMath::IsNearlyEqual(SFXVolume, Other.SFXVolume)
		&& FMath::IsNearlyEqual(UISoundVolume, Other.UISoundVolume)
		&& FMath::IsNearlyEqual(LookResponseStrength, Other.LookResponseStrength)
		&& bInvertLookY == Other.bInvertLookY
		&& FMath::IsNearlyEqual(CameraMotionStrength, Other.CameraMotionStrength)
		&& FlashEffectMode == Other.FlashEffectMode
		&& UIMotionMode == Other.UIMotionMode;
}

FWacomSettingsOperationResult FWacomSettingsOperationResult::Success(bool bInVideoConfirmationRequired)
{
	FWacomSettingsOperationResult Result;
	Result.bSucceeded = true;
	Result.bVideoConfirmationRequired = bInVideoConfirmationRequired;
	return Result;
}

FWacomSettingsOperationResult FWacomSettingsOperationResult::Failure(const FText& InReason)
{
	FWacomSettingsOperationResult Result;
	Result.FailureReason = InReason;
	return Result;
}
