// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "WacomLocalSettingsTypes.generated.h"

/** Controls decorative flash and pulse intensity without suppressing semantic feedback. */
UENUM()
enum class EWacomFlashEffectMode : uint8
{
	Full,
	Reduced,
	Off
};

/** Controls optional UI interpolation while preserving required state transitions. */
UENUM()
enum class EWacomUIMotionMode : uint8
{
	Full,
	Simplified
};

enum class EWacomRuntimeSettingsChangeReason : uint8
{
	Startup,
	Preview,
	PreviewCancelled,
	Applied,
	VideoConfirmed,
	VideoReverted
};

/** Complete local-machine settings state. This is deliberately separate from SaveGame data. */
struct WACOMAPP_API FWacomLocalSettingsSnapshot
{
	FIntPoint ScreenResolution = FIntPoint::ZeroValue;
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;
	bool bVSyncEnabled = false;
	float FrameRateLimit = 0.0f;
	int32 GraphicsQuality = 3;

	float MasterVolume = 1.0f;
	float MusicVolume = 1.0f;
	float SFXVolume = 1.0f;
	float UISoundVolume = 1.0f;

	float LookResponseStrength = 1.0f;
	bool bInvertLookY = false;
	float CameraMotionStrength = 1.0f;
	EWacomFlashEffectMode FlashEffectMode = EWacomFlashEffectMode::Full;
	EWacomUIMotionMode UIMotionMode = EWacomUIMotionMode::Full;

	void Sanitize();
	bool IsEquivalentTo(const FWacomLocalSettingsSnapshot& Other) const;
};

struct WACOMAPP_API FWacomSettingsEditSession
{
	FGuid Token;
	FWacomLocalSettingsSnapshot Snapshot;

	bool IsValid() const { return Token.IsValid(); }
};

struct WACOMAPP_API FWacomSettingsOperationResult
{
	bool bSucceeded = false;
	bool bVideoConfirmationRequired = false;
	FText FailureReason;

	static FWacomSettingsOperationResult Success(bool bInVideoConfirmationRequired = false);
	static FWacomSettingsOperationResult Failure(const FText& InReason);
};
