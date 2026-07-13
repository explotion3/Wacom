// Copyright Wacom. All Rights Reserved.

#include "Settings/WacomGameUserSettings.h"

#include "Settings/WacomLocalSettingsDefaults.h"

void UWacomGameUserSettings::SetToDefaults()
{
	// Resolve the monitor before Super clears DisplayID / DisplayIndex so an
	// explicit reset keeps using the display that currently owns the game.
	const FWacomLocalSettingsSnapshot ProjectDefaults =
		FWacomLocalSettingsDefaults::Build(this);
	Super::SetToDefaults();
	SetFromSnapshot(ProjectDefaults);
	ConfirmVideoMode();

	if (bIsValidatingSettings)
	{
		// Super::ValidateSettings overwrites VSync from r.VSync immediately after
		// invoking this hook. Reapply the one authoritative project profile once
		// the engine repair pass has completed.
		ProjectDefaultsAppliedDuringValidation = ProjectDefaults;
		bReapplyProjectDefaultsAfterValidation = true;
	}
}

void UWacomGameUserSettings::ValidateSettings()
{
	if (bIsValidatingSettings)
	{
		return;
	}

	TGuardValue<bool> ValidationGuard(bIsValidatingSettings, true);
	Super::ValidateSettings();

	if (bReapplyProjectDefaultsAfterValidation)
	{
		SetFromSnapshot(ProjectDefaultsAppliedDuringValidation);
		ConfirmVideoMode();
		bReapplyProjectDefaultsAfterValidation = false;
	}

	if (WacomSettingsSchemaVersion != CurrentWacomSettingsSchemaVersion)
	{
		// The engine-owned video/scalability state has already been validated by Super.
		// A Wacom schema mismatch only resets Wacom fields.
		ResetWacomCustomFieldsToDefaults();
	}

	FWacomLocalSettingsSnapshot Snapshot = MakeSnapshot();
	Snapshot.Sanitize();
	SetPreviewableFieldsFromSnapshot(Snapshot);
}

void UWacomGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	// Super::ValidateSettings can force-reload through this virtual hook. The
	// outer validation call owns the remainder of that repair pass.
	if (!bIsValidatingSettings)
	{
		ValidateSettings();
	}
}

void UWacomGameUserSettings::ApplyNonResolutionSettings()
{
	ValidateSettings();
	Super::ApplyNonResolutionSettings();
}

FWacomLocalSettingsSnapshot UWacomGameUserSettings::MakeSnapshot() const
{
	FWacomLocalSettingsSnapshot Snapshot;
	Snapshot.ScreenResolution = GetScreenResolution();
	Snapshot.WindowMode = GetFullscreenMode();
	Snapshot.bVSyncEnabled = IsVSyncEnabled();
	Snapshot.FrameRateLimit = GetFrameRateLimit();
	const int32 OverallQuality = GetOverallScalabilityLevel();
	Snapshot.GraphicsQuality = OverallQuality < 0 ? 3 : FMath::Clamp(OverallQuality, 0, 3);
	Snapshot.MasterVolume = MasterVolume;
	Snapshot.MusicVolume = MusicVolume;
	Snapshot.SFXVolume = SFXVolume;
	Snapshot.UISoundVolume = UISoundVolume;
	Snapshot.LookResponseStrength = LookResponseStrength;
	Snapshot.bInvertLookY = bInvertLookY;
	Snapshot.CameraMotionStrength = CameraMotionStrength;
	Snapshot.FlashEffectMode = FlashEffectMode;
	Snapshot.UIMotionMode = UIMotionMode;
	Snapshot.Sanitize();
	return Snapshot;
}

void UWacomGameUserSettings::SetFromSnapshot(const FWacomLocalSettingsSnapshot& InSnapshot)
{
	FWacomLocalSettingsSnapshot Snapshot = InSnapshot;
	Snapshot.Sanitize();
	SetScreenResolution(Snapshot.ScreenResolution);
	SetFullscreenMode(Snapshot.WindowMode);
	SetVSyncEnabled(Snapshot.bVSyncEnabled);
	SetFrameRateLimit(Snapshot.FrameRateLimit);
	SetOverallScalabilityLevel(Snapshot.GraphicsQuality);
	SetPreviewableFieldsFromSnapshot(Snapshot);
}

void UWacomGameUserSettings::SetPreviewableFieldsFromSnapshot(
	const FWacomLocalSettingsSnapshot& InSnapshot)
{
	FWacomLocalSettingsSnapshot Snapshot = InSnapshot;
	Snapshot.Sanitize();
	MasterVolume = Snapshot.MasterVolume;
	MusicVolume = Snapshot.MusicVolume;
	SFXVolume = Snapshot.SFXVolume;
	UISoundVolume = Snapshot.UISoundVolume;
	LookResponseStrength = Snapshot.LookResponseStrength;
	bInvertLookY = Snapshot.bInvertLookY;
	CameraMotionStrength = Snapshot.CameraMotionStrength;
	FlashEffectMode = Snapshot.FlashEffectMode;
	UIMotionMode = Snapshot.UIMotionMode;
	WacomSettingsSchemaVersion = CurrentWacomSettingsSchemaVersion;
}

void UWacomGameUserSettings::ResetWacomCustomFieldsToDefaults()
{
	SetPreviewableFieldsFromSnapshot(FWacomLocalSettingsDefaults::Build(this));
}
