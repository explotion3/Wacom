// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomGameUserSettings;
class UWacomSettingsSubsystem;
struct FWacomLocalSettingsSnapshot;

struct FWacomPresentationPolicyTestView
{
	bool bCardUseReducedMotion = false;
	bool bPlayedDissolveReducedMotion = false;
	bool bPileTransferReducedMotion = false;
	bool bSelectionReducedMotion = false;
	bool bDragPickupReducedMotion = false;
	float SelectionSweepIntensity = 0.0f;
	float PlayedDissolveEdgeIntensity = 0.0f;
	float TrailHeadOpacity = 0.0f;
	bool bTrailEnabled = false;
	int32 MaxMoteQuadCount = 0;
};

struct FWacomGameUserSettingsTestAccess
{
	static void SetRawCustomState(
		UWacomGameUserSettings& Settings,
		int32 SchemaVersion,
		float MasterVolume,
		float LookResponseStrength,
		float CameraMotionStrength,
		uint8 FlashMode,
		uint8 MotionMode);
};

struct FWacomSettingsSubsystemTestAccess
{
	static void ConfigureIsolatedSettings(
		UWacomSettingsSubsystem& Subsystem,
		UWacomGameUserSettings& Settings);
	static void ConfigureScreenResolutionEnvironment(
		UWacomSettingsSubsystem& Subsystem,
		FIntPoint DesktopResolution,
		FIntPoint WindowWorkArea,
		const TArray<FIntPoint>& FullscreenResolutions);
	static bool ExpirePendingVideoMode(UWacomSettingsSubsystem& Subsystem);
	static int32 GetPersistenceRequestCount(const UWacomSettingsSubsystem& Subsystem);
	static FWacomLocalSettingsSnapshot BuildFallbackDefaultSnapshot();
	static FWacomPresentationPolicyTestView EvaluatePresentationPolicy(
		const FWacomLocalSettingsSnapshot& Snapshot);
};
