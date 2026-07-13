// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "WacomGameUserSettings.generated.h"

/** Local-machine settings persisted by UE in GameUserSettings.ini. */
UCLASS(Config = GameUserSettings)
class WACOMAPP_API UWacomGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentWacomSettingsSchemaVersion = 1;

	virtual void SetToDefaults() override;
	virtual void ValidateSettings() override;
	virtual void LoadSettings(bool bForceReload = false) override;
	virtual void ApplyNonResolutionSettings() override;

	FWacomLocalSettingsSnapshot MakeSnapshot() const;
	void SetFromSnapshot(const FWacomLocalSettingsSnapshot& Snapshot);
	void SetPreviewableFieldsFromSnapshot(const FWacomLocalSettingsSnapshot& Snapshot);
	void ResetWacomCustomFieldsToDefaults();

	int32 GetWacomSettingsSchemaVersion() const { return WacomSettingsSchemaVersion; }

private:
#if WITH_AUTOMATION_TESTS
	friend struct FWacomGameUserSettingsTestAccess;
#endif

	// UGameUserSettings::ValidateSettings may call the virtual LoadSettings and
	// SetToDefaults hooks while repairing a missing/invalid engine settings file.
	// These guards keep that engine-owned repair pass non-recursive and let us
	// restore the project VSync default after the engine applies r.VSync.
	bool bIsValidatingSettings = false;
	bool bReapplyProjectDefaultsAfterValidation = false;
	FWacomLocalSettingsSnapshot ProjectDefaultsAppliedDuringValidation;

	UPROPERTY(Config)
	int32 WacomSettingsSchemaVersion = CurrentWacomSettingsSchemaVersion;

	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float MusicVolume = 1.0f;

	UPROPERTY(Config)
	float SFXVolume = 1.0f;

	UPROPERTY(Config)
	float UISoundVolume = 1.0f;

	UPROPERTY(Config)
	float LookResponseStrength = 1.0f;

	UPROPERTY(Config)
	bool bInvertLookY = false;

	UPROPERTY(Config)
	float CameraMotionStrength = 1.0f;

	UPROPERTY(Config)
	EWacomFlashEffectMode FlashEffectMode = EWacomFlashEffectMode::Full;

	UPROPERTY(Config)
	EWacomUIMotionMode UIMotionMode = EWacomUIMotionMode::Full;
};
