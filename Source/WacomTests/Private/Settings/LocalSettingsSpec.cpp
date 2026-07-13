// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Settings/WacomGameUserSettings.h"
#include "Settings/WacomSettingsDeveloperSettings.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Settings/WacomSettingsTestAccess.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/AudioSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomLocalSettingsDefaultsAndRoundTripTest,
	"Wacom.Settings.DefaultsSchemaAndRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomLocalSettingsDefaultsAndRoundTripTest::RunTest(const FString& Parameters)
{
	UWacomGameUserSettings* Settings = NewObject<UWacomGameUserSettings>();
	Settings->SetToDefaults();
	FWacomLocalSettingsSnapshot Defaults = Settings->MakeSnapshot();
	TestEqual(TEXT("Schema version is current"), Settings->GetWacomSettingsSchemaVersion(), 1);
	TestTrue(TEXT("Project defaults resolve a usable desktop resolution"),
		Defaults.ScreenResolution.X > 0 && Defaults.ScreenResolution.Y > 0);
	TestEqual(TEXT("Project defaults use borderless windowed mode"),
		Defaults.WindowMode, EWindowMode::WindowedFullscreen);
	TestTrue(TEXT("Project defaults enable VSync"), Defaults.bVSyncEnabled);
	TestEqual(TEXT("Project defaults cap frame rate at 60"), Defaults.FrameRateLimit, 60.0f);
	TestEqual(TEXT("Project defaults use high graphics quality"), Defaults.GraphicsQuality, 2);
	TestEqual(TEXT("Master defaults to one"), Defaults.MasterVolume, 1.0f);
	TestEqual(TEXT("Music defaults to one"), Defaults.MusicVolume, 1.0f);
	TestEqual(TEXT("SFX defaults to one"), Defaults.SFXVolume, 1.0f);
	TestEqual(TEXT("UI sound defaults to one"), Defaults.UISoundVolume, 1.0f);
	TestEqual(TEXT("Look response defaults to one"), Defaults.LookResponseStrength, 1.0f);
	TestFalse(TEXT("Y look is not inverted by default"), Defaults.bInvertLookY);
	TestEqual(TEXT("Camera motion defaults to one"), Defaults.CameraMotionStrength, 1.0f);
	TestEqual(TEXT("Flash defaults to full"), Defaults.FlashEffectMode, EWacomFlashEffectMode::Full);
	TestEqual(TEXT("UI motion defaults to full"), Defaults.UIMotionMode, EWacomUIMotionMode::Full);

	UGameInstance* DefaultTestGameInstance = NewObject<UGameInstance>();
	UWacomSettingsSubsystem* DefaultSubsystem = NewObject<UWacomSettingsSubsystem>(DefaultTestGameInstance);
	FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(*DefaultSubsystem, *Settings);
	const FWacomLocalSettingsSnapshot BeforeDefaultQuery = Settings->MakeSnapshot();
	TestTrue(TEXT("Subsystem default contract matches first-launch defaults"),
		DefaultSubsystem->GetDefaultSnapshot().IsEquivalentTo(Defaults));
	TestTrue(TEXT("Querying defaults does not mutate current settings"),
		Settings->MakeSnapshot().IsEquivalentTo(BeforeDefaultQuery));
	TestEqual(TEXT("Querying defaults does not request persistence"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*DefaultSubsystem), 0);
	const FWacomLocalSettingsSnapshot HeadlessFallback =
		FWacomSettingsSubsystemTestAccess::BuildFallbackDefaultSnapshot();
	TestEqual(TEXT("Missing display information falls back to 1280 x 720"),
		HeadlessFallback.ScreenResolution, FIntPoint(1280, 720));

	FWacomLocalSettingsSnapshot Authored = Defaults;
	Authored.ScreenResolution = FIntPoint(1600, 900);
	Authored.WindowMode = EWindowMode::Windowed;
	Authored.MasterVolume = 0.62f;
	Authored.MusicVolume = 0.48f;
	Authored.LookResponseStrength = 1.75f;
	Authored.bInvertLookY = true;
	Authored.CameraMotionStrength = 0.35f;
	Authored.FlashEffectMode = EWacomFlashEffectMode::Reduced;
	Authored.UIMotionMode = EWacomUIMotionMode::Simplified;
	Settings->SetFromSnapshot(Authored);

	const FString TempIni = FPaths::CreateTempFilename(
		*FPaths::ProjectSavedDir(), TEXT("WacomSettingsRoundTrip_"), TEXT(".ini"));
	Settings->SaveConfig(CPF_Config, *TempIni);
	UWacomGameUserSettings* Reloaded = NewObject<UWacomGameUserSettings>();
	Reloaded->SetToDefaults();
	Reloaded->LoadConfig(nullptr, *TempIni);
	Reloaded->ValidateSettings();
	TestTrue(TEXT("Temporary ini round-trip preserves the snapshot"),
		Reloaded->MakeSnapshot().IsEquivalentTo(Settings->MakeSnapshot()));
	IFileManager::Get().Delete(*TempIni);

	Reloaded->SetVSyncEnabled(false);
	Reloaded->SetFrameRateLimit(144.0f);
	Reloaded->SetOverallScalabilityLevel(1);
	const FIntPoint PreservedResolution = Reloaded->GetScreenResolution();
	const EWindowMode::Type PreservedWindowMode = Reloaded->GetFullscreenMode();
	const bool bPreservedVSync = Reloaded->IsVSyncEnabled();
	const float PreservedFrameRateLimit = Reloaded->GetFrameRateLimit();
	const int32 PreservedGraphicsQuality = Reloaded->GetOverallScalabilityLevel();
	FWacomGameUserSettingsTestAccess::SetRawCustomState(
		*Reloaded, 0, 4.0f, -5.0f, 8.0f, 255, 255);
	Reloaded->ValidateSettings();
	const FWacomLocalSettingsSnapshot Migrated = Reloaded->MakeSnapshot();
	TestEqual(TEXT("Custom migration preserves engine resolution"),
		Reloaded->GetScreenResolution(), PreservedResolution);
	TestEqual(TEXT("Custom migration preserves engine window mode"),
		Reloaded->GetFullscreenMode(), PreservedWindowMode);
	TestEqual(TEXT("Custom migration preserves engine VSync"),
		Reloaded->IsVSyncEnabled(), bPreservedVSync);
	TestEqual(TEXT("Custom migration preserves engine frame rate limit"),
		Reloaded->GetFrameRateLimit(), PreservedFrameRateLimit);
	TestEqual(TEXT("Custom migration preserves engine graphics quality"),
		Reloaded->GetOverallScalabilityLevel(), PreservedGraphicsQuality);
	TestEqual(TEXT("Custom migration restores master default"), Migrated.MasterVolume, 1.0f);
	TestEqual(TEXT("Custom migration restores look default"), Migrated.LookResponseStrength, 1.0f);
	TestEqual(TEXT("Custom migration restores camera default"), Migrated.CameraMotionStrength, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomLocalSettingsTransactionTest,
	"Wacom.Settings.TransactionPreviewApplyCancelAndVideoRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomLocalSettingsTransactionTest::RunTest(const FString& Parameters)
{
	UWacomGameUserSettings* Settings = NewObject<UWacomGameUserSettings>();
	Settings->SetToDefaults();
	FWacomLocalSettingsSnapshot Baseline = Settings->MakeSnapshot();
	Baseline.ScreenResolution = FIntPoint(1280, 720);
	Baseline.WindowMode = EWindowMode::Windowed;
	Settings->SetFromSnapshot(Baseline);

	UGameInstance* TestGameInstance = NewObject<UGameInstance>();
	UWacomSettingsSubsystem* Subsystem = NewObject<UWacomSettingsSubsystem>(TestGameInstance);
	FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(*Subsystem, *Settings);
	const FWacomSettingsEditSession Edit = Subsystem->BeginEdit();
	TestTrue(TEXT("BeginEdit returns a unique token"), Edit.IsValid());
	TestFalse(TEXT("A second edit is rejected"), Subsystem->BeginEdit().IsValid());

	FWacomLocalSettingsSnapshot Preview = Edit.Snapshot;
	Preview.MasterVolume = 0.25f;
	Preview.LookResponseStrength = 2.0f;
	Preview.ScreenResolution = FIntPoint(1920, 1080);
	Preview.bVSyncEnabled = !Edit.Snapshot.bVSyncEnabled;
	Preview.FrameRateLimit = Edit.Snapshot.FrameRateLimit + 30.0f;
	Preview.GraphicsQuality = Edit.Snapshot.GraphicsQuality == 3 ? 1 : 3;
	TestTrue(TEXT("Preview accepts active token"), Subsystem->Preview(Edit.Token, Preview).bSucceeded);
	TestEqual(TEXT("Preview updates audio"), Subsystem->GetCurrentSnapshot().MasterVolume, 0.25f);
	TestEqual(TEXT("Preview updates look"), Subsystem->GetCurrentSnapshot().LookResponseStrength, 2.0f);
	TestEqual(TEXT("Preview does not apply video"),
		Subsystem->GetCurrentSnapshot().ScreenResolution, Baseline.ScreenResolution);
	TestEqual(TEXT("Preview does not apply VSync"),
		Subsystem->GetCurrentSnapshot().bVSyncEnabled, Baseline.bVSyncEnabled);
	TestEqual(TEXT("Preview does not apply frame limit"),
		Subsystem->GetCurrentSnapshot().FrameRateLimit, Baseline.FrameRateLimit);
	TestEqual(TEXT("Preview does not apply graphics quality"),
		Subsystem->GetCurrentSnapshot().GraphicsQuality, Baseline.GraphicsQuality);
	TestEqual(TEXT("Preview and cancel do not request persistence"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Subsystem), 0);
	TestFalse(TEXT("Stale preview is rejected"),
		Subsystem->Preview(FGuid::NewGuid(), Preview).bSucceeded);
	TestTrue(TEXT("Cancel restores preview baseline"), Subsystem->Cancel(Edit.Token).bSucceeded);
	TestTrue(TEXT("Cancel restores all previewable values"),
		Subsystem->GetCurrentSnapshot().IsEquivalentTo(Baseline));
	TestFalse(TEXT("Repeated cancel is rejected"), Subsystem->Cancel(Edit.Token).bSucceeded);

	const FWacomSettingsEditSession ApplyEdit = Subsystem->BeginEdit();
	FWacomLocalSettingsSnapshot Applied = ApplyEdit.Snapshot;
	Applied.MasterVolume = 0.7f;
	Applied.GraphicsQuality = 2;
	const FWacomSettingsOperationResult ApplyResult = Subsystem->Apply(ApplyEdit.Token, Applied);
	TestTrue(TEXT("Non-video apply succeeds"), ApplyResult.bSucceeded);
	TestFalse(TEXT("Non-video apply does not require confirmation"), ApplyResult.bVideoConfirmationRequired);
	TestEqual(TEXT("Applied master persists in current state"),
		Subsystem->GetCurrentSnapshot().MasterVolume, 0.7f);
	TestEqual(TEXT("Non-video apply requests one persistence write"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Subsystem), 1);
	TestFalse(TEXT("Applied token cannot be reused"), Subsystem->Apply(ApplyEdit.Token, Applied).bSucceeded);

	const FWacomSettingsEditSession VideoEdit = Subsystem->BeginEdit();
	FWacomLocalSettingsSnapshot VideoDraft = VideoEdit.Snapshot;
	VideoDraft.ScreenResolution = FIntPoint(1920, 1080);
	VideoDraft.WindowMode = EWindowMode::WindowedFullscreen;
	VideoDraft.MusicVolume = 0.4f;
	const FWacomSettingsOperationResult VideoApply = Subsystem->Apply(VideoEdit.Token, VideoDraft);
	TestTrue(TEXT("Video apply succeeds"), VideoApply.bSucceeded);
	TestTrue(TEXT("Video apply requires confirmation"), VideoApply.bVideoConfirmationRequired);
	TestTrue(TEXT("Video confirmation is pending"), Subsystem->IsVideoModeConfirmationPending());
	TestEqual(TEXT("Unconfirmed video apply does not request persistence"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Subsystem), 1);
	TestFalse(TEXT("Wrong confirmation token is rejected"),
		Subsystem->ConfirmVideoMode(FGuid::NewGuid()).bSucceeded);
	TestFalse(TEXT("Timeout ticker does not continue after rollback"),
		FWacomSettingsSubsystemTestAccess::ExpirePendingVideoMode(*Subsystem));
	const FWacomLocalSettingsSnapshot Reverted = Subsystem->GetCurrentSnapshot();
	TestEqual(TEXT("Timeout restores confirmed resolution"),
		Reverted.ScreenResolution, Applied.ScreenResolution);
	TestEqual(TEXT("Timeout restores confirmed window mode"),
		Reverted.WindowMode, Applied.WindowMode);
	TestEqual(TEXT("Other applied settings survive video rollback"), Reverted.MusicVolume, 0.4f);
	TestFalse(TEXT("Video confirmation is cleared after timeout"),
		Subsystem->IsVideoModeConfirmationPending());
	TestEqual(TEXT("Timeout rollback persists the safe final state"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Subsystem), 2);

	const FWacomSettingsEditSession ConfirmEdit = Subsystem->BeginEdit();
	FWacomLocalSettingsSnapshot ConfirmDraft = ConfirmEdit.Snapshot;
	ConfirmDraft.ScreenResolution = FIntPoint(1600, 900);
	TestTrue(TEXT("Second video apply succeeds"),
		Subsystem->Apply(ConfirmEdit.Token, ConfirmDraft).bSucceeded);
	TestTrue(TEXT("Matching video token confirms"),
		Subsystem->ConfirmVideoMode(ConfirmEdit.Token).bSucceeded);
	TestEqual(TEXT("Confirmation persists the new video mode"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Subsystem), 3);
	TestFalse(TEXT("Repeated video confirmation is rejected"),
		Subsystem->ConfirmVideoMode(ConfirmEdit.Token).bSucceeded);

	const FWacomSettingsEditSession ManualRevertEdit = Subsystem->BeginEdit();
	FWacomLocalSettingsSnapshot ManualRevertDraft = ManualRevertEdit.Snapshot;
	ManualRevertDraft.ScreenResolution = FIntPoint(2560, 1440);
	ManualRevertDraft.UISoundVolume = 0.3f;
	TestTrue(TEXT("Manual rollback video apply succeeds"),
		Subsystem->Apply(ManualRevertEdit.Token, ManualRevertDraft).bSucceeded);
	TestTrue(TEXT("Manual rollback accepts the matching token"),
		Subsystem->RevertVideoMode(ManualRevertEdit.Token).bSucceeded);
	TestEqual(TEXT("Manual rollback restores the prior resolution"),
		Subsystem->GetCurrentSnapshot().ScreenResolution, ConfirmDraft.ScreenResolution);
	TestEqual(TEXT("Manual rollback preserves other applied settings"),
		Subsystem->GetCurrentSnapshot().UISoundVolume, 0.3f);
	TestEqual(TEXT("Manual rollback persists the safe final state"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Subsystem), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomLocalSettingsPresentationPolicyTest,
	"Wacom.Settings.PresentationPolicyAndCameraMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomLocalSettingsPresentationPolicyTest::RunTest(const FString& Parameters)
{
	FWacomLocalSettingsSnapshot Snapshot;
	const FWacomPresentationPolicyTestView Full =
		FWacomSettingsSubsystemTestAccess::EvaluatePresentationPolicy(Snapshot);
	TestEqual(TEXT("Full flash preserves sweep"), Full.SelectionSweepIntensity, 0.95f);
	TestTrue(TEXT("Full flash preserves trail"), Full.bTrailEnabled);
	TestFalse(TEXT("Full motion does not force reduced card motion"), Full.bCardUseReducedMotion);

	Snapshot.FlashEffectMode = EWacomFlashEffectMode::Reduced;
	Snapshot.UIMotionMode = EWacomUIMotionMode::Simplified;
	const FWacomPresentationPolicyTestView Reduced =
		FWacomSettingsSubsystemTestAccess::EvaluatePresentationPolicy(Snapshot);
	TestTrue(TEXT("Simplified motion forces every existing reduced path"),
		Reduced.bCardUseReducedMotion
		&& Reduced.bPlayedDissolveReducedMotion
		&& Reduced.bPileTransferReducedMotion
		&& Reduced.bSelectionReducedMotion
		&& Reduced.bDragPickupReducedMotion);
	TestTrue(TEXT("Reduced flash maps to 35 percent"),
		FMath::IsNearlyEqual(Reduced.SelectionSweepIntensity, 0.95f * 0.35f));
	TestTrue(TEXT("Reduced trail maps to 35 percent"),
		FMath::IsNearlyEqual(Reduced.TrailHeadOpacity, 0.44f * 0.35f));

	Snapshot.FlashEffectMode = EWacomFlashEffectMode::Off;
	const FWacomPresentationPolicyTestView Off =
		FWacomSettingsSubsystemTestAccess::EvaluatePresentationPolicy(Snapshot);
	TestEqual(TEXT("Off removes decorative sweep"), Off.SelectionSweepIntensity, 0.0f);
	TestFalse(TEXT("Off removes decorative trail"), Off.bTrailEnabled);
	TestEqual(TEXT("Off removes decorative motes"), Off.MaxMoteQuadCount, 0);

	UWacomFirstPersonWalkBobComponent* FullBob = NewObject<UWacomFirstPersonWalkBobComponent>();
	UWacomFirstPersonWalkBobComponent* HalfBob = NewObject<UWacomFirstPersonWalkBobComponent>();
	UWacomFirstPersonWalkBobComponent* ZeroBob = NewObject<UWacomFirstPersonWalkBobComponent>();
	HalfBob->SetRuntimeMotionStrength(0.5f);
	ZeroBob->SetRuntimeMotionStrength(0.0f);
	FullBob->UpdateWalkBobFromMovementDelta(0.1f, 10.0f, 100.0f);
	HalfBob->UpdateWalkBobFromMovementDelta(0.1f, 10.0f, 100.0f);
	ZeroBob->UpdateWalkBobFromMovementDelta(0.1f, 10.0f, 100.0f);
	const float FullMagnitude = FullBob->GetCurrentLocationOffset().Size();
	const float HalfMagnitude = HalfBob->GetCurrentLocationOffset().Size();
	TestTrue(TEXT("Half camera motion halves walk bob"),
		FMath::IsNearlyEqual(HalfMagnitude, FullMagnitude * 0.5f, 0.001f));
	TestTrue(TEXT("Zero camera motion outputs zero walk bob"),
		ZeroBob->GetCurrentLocationOffset().IsNearlyZero());

	UWacomCursorLookDriverComponent* CursorLook = NewObject<UWacomCursorLookDriverComponent>();
	CursorLook->UpdateFromNormalizedCursor(
		FVector2D(0.25f, 0.5f), 0.0f, 12.0f, 8.0f, 2.0f, 2.0f, 0.0f);
	TestEqual(TEXT("Look response scales cursor yaw"),
		CursorLook->GetCurrentLookOffset().Yaw, 6.0);
	TestEqual(TEXT("Normal Y cursor direction pitches down"),
		CursorLook->GetCurrentLookOffset().Pitch, -8.0);
	CursorLook->ResetLookOffset();
	CursorLook->UpdateFromNormalizedCursor(
		FVector2D(0.0f, 0.5f), 0.0f, 12.0f, 8.0f, 1.0f, -1.0f, 0.0f);
	TestEqual(TEXT("Negative effective pitch scale implements inverted Y"),
		CursorLook->GetCurrentLookOffset().Pitch, 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomLocalSettingsAudioRoutingTest,
	"Wacom.Settings.AudioRoutingAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomLocalSettingsAudioRoutingTest::RunTest(const FString& Parameters)
{
	const UWacomSettingsDeveloperSettings* Config = GetDefault<UWacomSettingsDeveloperSettings>();
	USoundClass* Master = Config->MasterSoundClass.LoadSynchronous();
	USoundClass* Music = Config->MusicSoundClass.LoadSynchronous();
	USoundClass* SFX = Config->SFXSoundClass.LoadSynchronous();
	USoundClass* UI = Config->UISoundClass.LoadSynchronous();
	USoundMix* Mix = Config->UserSettingsSoundMix.LoadSynchronous();
	const UAudioSettings* EngineAudioSettings = GetDefault<UAudioSettings>();
	TestNotNull(TEXT("Master SoundClass"), Master);
	TestNotNull(TEXT("Music SoundClass"), Music);
	TestNotNull(TEXT("SFX SoundClass"), SFX);
	TestNotNull(TEXT("UI SoundClass"), UI);
	TestNotNull(TEXT("User settings SoundMix"), Mix);
	TestEqual(TEXT("Project default SoundClass uses Wacom master"),
		EngineAudioSettings->DefaultSoundClassName,
		FSoftObjectPath(TEXT("/Game/Wacom/Audio/Settings/SC_Wacom_Master.SC_Wacom_Master")));
	TestEqual(TEXT("Project base SoundMix uses user settings mix"),
		EngineAudioSettings->DefaultBaseSoundMix,
		FSoftObjectPath(TEXT("/Game/Wacom/Audio/Settings/SM_Wacom_UserSettings.SM_Wacom_UserSettings")));
	if (Master)
	{
		TestTrue(TEXT("Music is a master child"), Master->ChildClasses.Contains(Music));
		TestTrue(TEXT("SFX is a master child"), Master->ChildClasses.Contains(SFX));
		TestTrue(TEXT("UI is a master child"), Master->ChildClasses.Contains(UI));
	}
	return true;
}
