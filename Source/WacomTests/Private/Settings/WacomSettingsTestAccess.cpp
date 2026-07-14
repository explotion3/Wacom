// Copyright Wacom. All Rights Reserved.

#include "Settings/WacomSettingsTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Settings/WacomGameUserSettings.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "../../../WacomApp/Private/Settings/WacomPresentationAccessibilityPolicy.h"
#include "../../../WacomApp/Private/Settings/WacomLocalSettingsDefaults.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardSlotLayoutBuilder.h"

void FWacomGameUserSettingsTestAccess::SetRawCustomState(
	UWacomGameUserSettings& Settings,
	int32 SchemaVersion,
	float MasterVolume,
	float LookResponseStrength,
	float CameraMotionStrength,
	uint8 FlashMode,
	uint8 MotionMode)
{
	Settings.WacomSettingsSchemaVersion = SchemaVersion;
	Settings.MasterVolume = MasterVolume;
	Settings.LookResponseStrength = LookResponseStrength;
	Settings.CameraMotionStrength = CameraMotionStrength;
	Settings.FlashEffectMode = static_cast<EWacomFlashEffectMode>(FlashMode);
	Settings.UIMotionMode = static_cast<EWacomUIMotionMode>(MotionMode);
}

void FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(
	UWacomSettingsSubsystem& Subsystem,
	UWacomGameUserSettings& Settings)
{
	Subsystem.SettingsOverrideForTest = &Settings;
	Subsystem.bSuppressEngineApplicationAndPersistenceForTest = true;
}

void FWacomSettingsSubsystemTestAccess::ConfigureScreenResolutionEnvironment(
	UWacomSettingsSubsystem& Subsystem,
	FIntPoint DesktopResolution,
	FIntPoint WindowWorkArea,
	const TArray<FIntPoint>& FullscreenResolutions)
{
	Subsystem.bHasScreenResolutionEnvironmentOverrideForTest = true;
	Subsystem.DesktopResolutionOverrideForTest = DesktopResolution;
	Subsystem.WindowWorkAreaOverrideForTest = WindowWorkArea;
	Subsystem.FullscreenResolutionsOverrideForTest = FullscreenResolutions;
}

bool FWacomSettingsSubsystemTestAccess::ExpirePendingVideoMode(
	UWacomSettingsSubsystem& Subsystem)
{
	Subsystem.PendingVideoModeDeadlineSeconds = 0.0;
	return Subsystem.TickVideoModeConfirmation(0.0f);
}

int32 FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(
	const UWacomSettingsSubsystem& Subsystem)
{
	return Subsystem.PersistenceRequestCountForTest;
}

FWacomLocalSettingsSnapshot FWacomSettingsSubsystemTestAccess::BuildFallbackDefaultSnapshot()
{
	return FWacomLocalSettingsDefaults::Build(nullptr);
}

FWacomPresentationPolicyTestView FWacomSettingsSubsystemTestAccess::EvaluatePresentationPolicy(
	const FWacomLocalSettingsSnapshot& Snapshot)
{
	FWacomFirstPersonCardResolvedLayoutConfig Config;
	FWacomPresentationAccessibilityPolicy::ApplyToFirstPersonCardConfig(
		Config,
		FWacomPresentationAccessibilityPolicy::GetDecorativeFlashIntensityScale(
			Snapshot.FlashEffectMode),
		FWacomPresentationAccessibilityPolicy::UsesSimplifiedMotion(Snapshot.UIMotionMode));

	FWacomPresentationPolicyTestView View;
	View.bCardUseReducedMotion = Config.CardUseEffect.bReducedMotion;
	View.bPlayedDissolveReducedMotion = Config.PlayedDissolve.bReducedMotion;
	View.bPileTransferReducedMotion = Config.PileTransfer.bReducedMotion;
	View.bSelectionReducedMotion = Config.Selection.bReducedMotion;
	View.bDragPickupReducedMotion = Config.bReduceDragPickupMotion;
	View.SelectionSweepIntensity = Config.Selection.Style.SweepIntensity;
	View.PlayedDissolveEdgeIntensity = Config.PlayedDissolve.Style.EdgeIntensity;
	View.TrailHeadOpacity = Config.PileTransfer.Style.TrailHeadOpacity;
	View.bTrailEnabled = Config.PileTransfer.Style.bEnableTrail;
	View.MaxMoteQuadCount = Config.PileTransfer.Style.MaxMoteQuadCount;
	return View;
}

#endif
