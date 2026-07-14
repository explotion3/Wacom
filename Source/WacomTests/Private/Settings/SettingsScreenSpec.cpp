// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Components/WacomFirstPersonWalkBobComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Settings/SettingsScreenTestAccess.h"
#include "Settings/WacomGameUserSettings.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Settings/WacomSettingsTestAccess.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Menus/WacomPauseMenuScreen.h"
#include "UI/Settings/WacomSettingsConfirmationDialog.h"
#include "UI/Settings/WacomSettingsOptionRow.h"
#include "UI/Settings/WacomSettingsScreen.h"

namespace
{
	const TArray<FIntPoint>& GetTestFullscreenResolutions()
	{
		static const TArray<FIntPoint> Resolutions = {
			FIntPoint(1280, 720), FIntPoint(1366, 768), FIntPoint(1600, 900),
			FIntPoint(1920, 1080), FIntPoint(2560, 1440), FIntPoint(3840, 2160)
		};
		return Resolutions;
	}

	struct FSettingsScreenFixture
	{
		TStrongObjectPtr<UWacomGameUserSettings> Settings;
		TStrongObjectPtr<UGameInstance> GameInstance;
		TStrongObjectPtr<UWacomSettingsSubsystem> Subsystem;
		TStrongObjectPtr<UWacomSettingsScreen> Screen;

		FSettingsScreenFixture()
			: Settings(NewObject<UWacomGameUserSettings>())
			, GameInstance(NewObject<UGameInstance>())
			, Subsystem(NewObject<UWacomSettingsSubsystem>(GameInstance.Get()))
			, Screen(NewObject<UWacomSettingsScreen>())
		{
			Settings->SetToDefaults();
		}

		bool Start(const FWacomLocalSettingsSnapshot& Snapshot)
		{
			Settings->SetFromSnapshot(Snapshot);
			FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(*Subsystem, *Settings);
			FWacomSettingsSubsystemTestAccess::ConfigureScreenResolutionEnvironment(
				*Subsystem,
				Subsystem->GetDefaultSnapshot().ScreenResolution,
				FIntPoint(3840, 2160),
				GetTestFullscreenResolutions());
			FWacomSettingsScreenTestAccess::BuildAndConstruct(*Screen);
			return FWacomSettingsScreenTestAccess::BeginWithSubsystem(*Screen, *Subsystem);
		}

		~FSettingsScreenFixture()
		{
			if (Screen.IsValid())
			{
				FWacomSettingsScreenTestAccess::Destruct(*Screen);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAuthoredWidgetContractSpec,
	"Wacom.UI.Settings.AuthoredWidgetAndRegistryContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAuthoredWidgetContractSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* ButtonClass = LoadClass<UWacomMenuButtonWidget>(
		nullptr, TEXT("/Game/Wacom/UI/Settings/WBP_SettingsButton.WBP_SettingsButton_C"));
	UClass* RowClass = LoadClass<UWacomSettingsOptionRow>(
		nullptr, TEXT("/Game/Wacom/UI/Settings/WBP_SettingsOptionRow.WBP_SettingsOptionRow_C"));
	UClass* DialogClass = LoadClass<UWacomSettingsConfirmationDialog>(
		nullptr, TEXT("/Game/Wacom/UI/Settings/WBP_SettingsConfirmationDialog.WBP_SettingsConfirmationDialog_C"));
	UClass* ScreenClass = LoadClass<UWacomSettingsScreen>(
		nullptr, TEXT("/Game/Wacom/UI/Settings/WBP_SettingsScreen.WBP_SettingsScreen_C"));
	if (!TestNotNull(TEXT("Settings button WBP"), ButtonClass)
		|| !TestNotNull(TEXT("Settings option row WBP"), RowClass)
		|| !TestNotNull(TEXT("Settings confirmation WBP"), DialogClass)
		|| !TestNotNull(TEXT("Settings screen WBP"), ScreenClass))
	{
		return false;
	}

	TStrongObjectPtr<UWacomSettingsScreen> Screen(
		NewObject<UWacomSettingsScreen>(GetTransientPackage(), ScreenClass));
	FWacomSettingsScreenTestAccess::BuildAndConstruct(*Screen);
	TestTrue(TEXT("Authored Settings Screen binds every required widget"),
		FWacomSettingsScreenTestAccess::HasRequiredBindings(*Screen));
	const FWacomSettingsScreenClassView Classes = FWacomSettingsScreenTestAccess::Classes(*Screen);
	TestEqual(TEXT("Screen reuses authored settings button"), Classes.ButtonClass, ButtonClass);
	TestEqual(TEXT("Screen reuses authored option row"), Classes.RowClass, RowClass);
	TestEqual(TEXT("Screen reuses authored confirmation modal"), Classes.DialogClass, DialogClass);
	FWacomSettingsScreenTestAccess::Destruct(*Screen);

	const UWacomUIDeveloperSettings* UISettings = GetDefault<UWacomUIDeveloperSettings>();
	const FWacomUIWidgetClassEntry* Registration = UISettings->WidgetClasses.FindByPredicate(
		[](const FWacomUIWidgetClassEntry& Entry)
		{
			return Entry.WidgetTag == WacomUITags::UI_Widget_SettingsScreen.GetTag();
		});
	TestNotNull(TEXT("Settings screen has a Widget Tag registration"), Registration);
	if (Registration)
	{
		TestEqual(TEXT("Widget Tag resolves the authored Settings Screen"),
			Registration->WidgetClass.LoadSynchronous(), ScreenClass);
	}
	TArray<FText> ValidationErrors;
	TestTrue(TEXT("UI registry validates Settings Screen parent contract"),
		UISettings->ValidateSettings(ValidationErrors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsFieldContractSpec,
	"Wacom.UI.Settings.FieldsFormattingAndBorderlessContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsFieldContractSpec::RunTest(const FString& /*Parameters*/)
{
	FSettingsScreenFixture Fixture;
	FWacomLocalSettingsSnapshot Snapshot;
	Snapshot.ScreenResolution = FIntPoint(1536, 864);
	Snapshot.WindowMode = EWindowMode::Windowed;
	Snapshot.FrameRateLimit = 73.0f;
	Snapshot.MasterVolume = 0.42f;
	Snapshot.LookResponseStrength = 2.5f;
	Snapshot.CameraMotionStrength = 0.35f;
	if (!TestTrue(TEXT("Settings edit starts"), Fixture.Start(Snapshot)))
	{
		return false;
	}

	struct FCategoryExpectation
	{
		EWacomSettingsCategory Category;
		int32 RowCount;
	};
	const FCategoryExpectation Expectations[] = {
		{ EWacomSettingsCategory::Display, 4 },
		{ EWacomSettingsCategory::Graphics, 1 },
		{ EWacomSettingsCategory::Audio, 4 },
		{ EWacomSettingsCategory::View, 3 },
		{ EWacomSettingsCategory::Accessibility, 2 }
	};
	for (const FCategoryExpectation& Expectation : Expectations)
	{
		FWacomSettingsScreenTestAccess::SelectCategory(*Fixture.Screen, Expectation.Category);
		TestEqual(TEXT("Category exposes the expected field count"),
			FWacomSettingsScreenTestAccess::View(*Fixture.Screen).VisibleOptionCount,
			Expectation.RowCount);
	}

	FWacomSettingsScreenTestAccess::SelectCategory(
		*Fixture.Screen, EWacomSettingsCategory::Display);
	TestEqual(TEXT("Custom resolution is retained and formatted"),
		FWacomSettingsScreenTestAccess::Row(
			*Fixture.Screen, EWacomSettingsField::ScreenResolution).Value.ToString(),
		FString(TEXT("1536 × 864")));
	TestEqual(TEXT("Custom frame limit is retained and formatted"),
		FWacomSettingsScreenTestAccess::Row(
			*Fixture.Screen, EWacomSettingsField::FrameRateLimit).Value.ToString(),
		FString(TEXT("73 FPS")));

	FWacomSettingsScreenTestAccess::Step(
		*Fixture.Screen, EWacomSettingsField::WindowMode, -1);
	FWacomSettingsScreenTestAccess::Step(
		*Fixture.Screen, EWacomSettingsField::WindowMode, -1);
	const FWacomSettingsOptionRowViewData BorderlessResolution =
		FWacomSettingsScreenTestAccess::Row(
			*Fixture.Screen, EWacomSettingsField::ScreenResolution);
	TestFalse(TEXT("Borderless mode keeps resolution visible but disables it"),
		BorderlessResolution.bEnabled);
	TestTrue(TEXT("Borderless resolution explains desktop ownership"),
		BorderlessResolution.Value.ToString().Contains(TEXT("跟随桌面")));

	FWacomSettingsScreenTestAccess::SelectCategory(
		*Fixture.Screen, EWacomSettingsCategory::Audio);
	TestEqual(TEXT("Audio volume uses percentage formatting"),
		FWacomSettingsScreenTestAccess::Row(
			*Fixture.Screen, EWacomSettingsField::MasterVolume).Value.ToString(),
		FString(TEXT("42%")));
	FWacomSettingsScreenTestAccess::SelectCategory(
		*Fixture.Screen, EWacomSettingsCategory::View);
	TestEqual(TEXT("Look response exposes the 0-300 percent contract"),
		FWacomSettingsScreenTestAccess::Row(
			*Fixture.Screen, EWacomSettingsField::LookResponseStrength).Value.ToString(),
		FString(TEXT("250%")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsRestoreDefaultsContractSpec,
	"Wacom.UI.Settings.RestoreDefaultsDraftPreviewAndCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsRestoreDefaultsContractSpec::RunTest(const FString& /*Parameters*/)
{
	FSettingsScreenFixture Fixture;
	FWacomLocalSettingsSnapshot Baseline;
	Baseline.ScreenResolution = FIntPoint(1600, 900);
	Baseline.WindowMode = EWindowMode::Windowed;
	Baseline.bVSyncEnabled = false;
	Baseline.FrameRateLimit = 144.0f;
	Baseline.GraphicsQuality = 0;
	Baseline.MasterVolume = 0.2f;
	Baseline.MusicVolume = 0.3f;
	Baseline.SFXVolume = 0.4f;
	Baseline.UISoundVolume = 0.5f;
	Baseline.LookResponseStrength = 2.5f;
	Baseline.bInvertLookY = true;
	Baseline.CameraMotionStrength = 0.25f;
	Baseline.FlashEffectMode = EWacomFlashEffectMode::Reduced;
	Baseline.UIMotionMode = EWacomUIMotionMode::Simplified;
	if (!TestTrue(TEXT("Settings edit starts"), Fixture.Start(Baseline)))
	{
		return false;
	}

	const FWacomSettingsScreenAutomationTestView Initial =
		FWacomSettingsScreenTestAccess::View(*Fixture.Screen);
	TestTrue(TEXT("Restore defaults starts enabled for a non-default draft"),
		Initial.bRestoreDefaultsEnabled);
	TestEqual(TEXT("Screen caches the project 60 FPS default"),
		Initial.DefaultSnapshot.FrameRateLimit, 60.0f);
	TestEqual(TEXT("Screen caches the project high-quality default"),
		Initial.DefaultSnapshot.GraphicsQuality, 2);

	int32 RuntimeBroadcastCount = 0;
	const FDelegateHandle RuntimeHandle = Fixture.Subsystem->OnRuntimeSettingsChangedNative().AddLambda(
		[&RuntimeBroadcastCount](
			const FWacomLocalSettingsSnapshot&,
			EWacomRuntimeSettingsChangeReason)
		{
			++RuntimeBroadcastCount;
		});
	FWacomSettingsScreenTestAccess::RestoreDefaults(*Fixture.Screen);

	const FWacomSettingsScreenAutomationTestView Restored =
		FWacomSettingsScreenTestAccess::View(*Fixture.Screen);
	TestTrue(TEXT("Restore defaults replaces the complete draft"),
		Restored.Draft.IsEquivalentTo(Restored.DefaultSnapshot));
	TestTrue(TEXT("Restore defaults leaves the transaction dirty until Apply"),
		Restored.bDirty);
	TestFalse(TEXT("Restore defaults disables itself when the draft is default"),
		Restored.bRestoreDefaultsEnabled);
	TestEqual(TEXT("Restore defaults previews runtime settings once"),
		RuntimeBroadcastCount, 1);

	const FWacomLocalSettingsSnapshot Previewed = Fixture.Subsystem->GetCurrentSnapshot();
	TestEqual(TEXT("Restore defaults previews master volume"), Previewed.MasterVolume, 1.0f);
	TestEqual(TEXT("Restore defaults previews look response"), Previewed.LookResponseStrength, 1.0f);
	TestFalse(TEXT("Restore defaults previews Y inversion"), Previewed.bInvertLookY);
	TestEqual(TEXT("Restore defaults previews camera motion"), Previewed.CameraMotionStrength, 1.0f);
	TestEqual(TEXT("Restore defaults does not preview resolution"),
		Previewed.ScreenResolution, Baseline.ScreenResolution);
	TestEqual(TEXT("Restore defaults does not preview window mode"),
		Previewed.WindowMode, Baseline.WindowMode);
	TestEqual(TEXT("Restore defaults does not preview VSync"),
		Previewed.bVSyncEnabled, Baseline.bVSyncEnabled);
	TestEqual(TEXT("Restore defaults does not preview frame rate"),
		Previewed.FrameRateLimit, Baseline.FrameRateLimit);
	TestEqual(TEXT("Restore defaults does not preview graphics quality"),
		Previewed.GraphicsQuality, Baseline.GraphicsQuality);
	TestEqual(TEXT("Restore defaults preview does not persist"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Fixture.Subsystem), 0);

	FWacomSettingsScreenTestAccess::RestoreDefaults(*Fixture.Screen);
	TestEqual(TEXT("Repeated restore defaults does not preview again"),
		RuntimeBroadcastCount, 1);
	TestTrue(TEXT("Cancel after restore defaults succeeds"),
		Fixture.Subsystem->Cancel(Restored.Token).bSucceeded);
	TestTrue(TEXT("Cancel after restore defaults restores the complete baseline"),
		Fixture.Subsystem->GetCurrentSnapshot().IsEquivalentTo(Baseline));
	TestEqual(TEXT("Cancel after restore defaults still does not persist"),
		FWacomSettingsSubsystemTestAccess::GetPersistenceRequestCount(*Fixture.Subsystem), 0);
	Fixture.Subsystem->OnRuntimeSettingsChangedNative().Remove(RuntimeHandle);

	FSettingsScreenFixture ApplyFixture;
	FWacomLocalSettingsSnapshot ApplyBaseline = ApplyFixture.Settings->MakeSnapshot();
	ApplyBaseline.MasterVolume = 0.35f;
	ApplyBaseline.LookResponseStrength = 2.0f;
	if (!TestTrue(TEXT("Second settings edit starts"), ApplyFixture.Start(ApplyBaseline)))
	{
		return false;
	}
	const FGuid ApplyToken =
		FWacomSettingsScreenTestAccess::View(*ApplyFixture.Screen).Token;
	FWacomSettingsScreenTestAccess::RestoreDefaults(*ApplyFixture.Screen);
	FWacomSettingsScreenTestAccess::Apply(*ApplyFixture.Screen);
	const FWacomSettingsScreenAutomationTestView Applied =
		FWacomSettingsScreenTestAccess::View(*ApplyFixture.Screen);
	TestTrue(TEXT("Applying restored defaults opens a fresh edit token"),
		Applied.Token.IsValid() && Applied.Token != ApplyToken);
	TestFalse(TEXT("Applied restored defaults start a clean draft"), Applied.bDirty);
	TestTrue(TEXT("Applying restored defaults commits the project profile"),
		ApplyFixture.Subsystem->GetCurrentSnapshot().IsEquivalentTo(Applied.DefaultSnapshot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsTransactionCoordinatorSpec,
	"Wacom.UI.Settings.PreviewApplyAndModalFailureRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsTransactionCoordinatorSpec::RunTest(const FString& /*Parameters*/)
{
	FSettingsScreenFixture Fixture;
	FWacomLocalSettingsSnapshot Snapshot;
	Snapshot.ScreenResolution = FIntPoint(1280, 720);
	Snapshot.WindowMode = EWindowMode::Windowed;
	Snapshot.GraphicsQuality = 3;
	if (!TestTrue(TEXT("Settings edit starts"), Fixture.Start(Snapshot)))
	{
		return false;
	}
	const FGuid FirstToken = FWacomSettingsScreenTestAccess::View(*Fixture.Screen).Token;

	FWacomSettingsScreenTestAccess::SetNormalized(
		*Fixture.Screen, EWacomSettingsField::MasterVolume, 0.35f);
	TestEqual(TEXT("Previewable audio field applies immediately"),
		Fixture.Subsystem->GetCurrentSnapshot().MasterVolume, 0.35f);
	FWacomSettingsScreenTestAccess::Step(
		*Fixture.Screen, EWacomSettingsField::GraphicsQuality, -1);
	TestEqual(TEXT("Graphics draft does not preview"),
		Fixture.Subsystem->GetCurrentSnapshot().GraphicsQuality, 3);
	TestEqual(TEXT("Graphics draft changes locally"),
		FWacomSettingsScreenTestAccess::View(*Fixture.Screen).Draft.GraphicsQuality, 2);

	FWacomSettingsScreenTestAccess::Apply(*Fixture.Screen);
	const FWacomSettingsScreenAutomationTestView Applied =
		FWacomSettingsScreenTestAccess::View(*Fixture.Screen);
	TestTrue(TEXT("Apply keeps the page in a fresh edit transaction"),
		Applied.Token.IsValid() && Applied.Token != FirstToken);
	TestFalse(TEXT("Fresh transaction is clean"), Applied.bDirty);
	TestEqual(TEXT("Apply commits the deferred graphics draft"),
		Fixture.Subsystem->GetCurrentSnapshot().GraphicsQuality, 2);

	FWacomSettingsScreenTestAccess::Step(
		*Fixture.Screen, EWacomSettingsField::ScreenResolution, 1);
	FWacomSettingsScreenTestAccess::Apply(*Fixture.Screen);
	const FWacomSettingsScreenAutomationTestView Reverted =
		FWacomSettingsScreenTestAccess::View(*Fixture.Screen);
	TestFalse(TEXT("Failed Modal push immediately clears video confirmation"),
		Reverted.bAwaitingVideoConfirmation);
	TestTrue(TEXT("Failed Modal push restarts a safe edit transaction"),
		Reverted.Token.IsValid());
	TestEqual(TEXT("Failed Modal push restores the confirmed resolution"),
		Fixture.Subsystem->GetCurrentSnapshot().ScreenResolution,
		FIntPoint(1280, 720));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsPauseMenuEntryContractSpec,
	"Wacom.UI.Settings.PauseMenuEntryContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsPauseMenuEntryContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomPauseMenuScreen> PauseScreen(NewObject<UWacomPauseMenuScreen>());
	PauseScreen->TakeWidget();

	UWacomMenuButtonWidget* SettingsButton = Cast<UWacomMenuButtonWidget>(
		PauseScreen->GetWidgetFromName(TEXT("SettingsButton")));
	TestNotNull(TEXT("Pause menu fallback exposes the shared Settings entry"), SettingsButton);
	if (SettingsButton)
	{
		TestTrue(TEXT("Pause menu Settings entry participates in CommonUI focus"),
			SettingsButton->GetIsFocusable());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomSettingsRunTunnelCameraShakeAssetSpec,
	"Wacom.Settings.RunTunnelCameraShakeAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomSettingsRunTunnelCameraShakeAssetSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* PlayerClass = LoadClass<AWacomPlayerCharacter>(
		nullptr,
		TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter.BP_WacomPlayerCharacter_C"));
	if (!TestNotNull(TEXT("Player Blueprint class"), PlayerClass))
	{
		return false;
	}
	const AWacomPlayerCharacter* PlayerCDO = Cast<AWacomPlayerCharacter>(
		PlayerClass->GetDefaultObject());
	const UWacomRunTunnelMovementComponent* RunTunnel = PlayerCDO
		? PlayerCDO->GetRunTunnelMovementComponent()
		: nullptr;
	const UWacomFirstPersonWalkBobComponent* WalkBob = PlayerCDO
		? PlayerCDO->GetWalkBobComponent()
		: nullptr;
	TestNotNull(TEXT("Run Tunnel movement component"), RunTunnel);
	TestNotNull(TEXT("Walk Bob component"), WalkBob);
	if (RunTunnel)
	{
		TestTrue(TEXT("Authored player enables Run Tunnel CameraShake"),
			RunTunnel->bUseWalkCameraShake);
		TestNotNull(TEXT("Authored player retains its Walk CameraShake class"),
			RunTunnel->WalkCameraShakeClass.Get());
	}
	if (WalkBob)
	{
		TestFalse(TEXT("Authored player keeps legacy WalkBob disabled"),
			WalkBob->bEnableWalkBob);
	}
	return true;
}

#endif
