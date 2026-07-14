// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Engine/UserInterfaceSettings.h"
#include "Settings/SettingsScreenTestAccess.h"
#include "Settings/WacomGameUserSettings.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Settings/WacomSettingsTestAccess.h"
#include "UI/Foundation/WacomDPIScalingRule.h"
#include "UI/Settings/WacomSettingsOptionRow.h"
#include "UI/Settings/WacomSettingsScreen.h"

namespace
{
	TStrongObjectPtr<UWacomSettingsSubsystem> CreateResolutionSubsystem(
		TStrongObjectPtr<UGameInstance>& GameInstance)
	{
		GameInstance.Reset(NewObject<UGameInstance>());
		return TStrongObjectPtr<UWacomSettingsSubsystem>(
			NewObject<UWacomSettingsSubsystem>(GameInstance.Get()));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDisplayResolutionPolicySpec,
	"Wacom.Settings.DisplayResolutionPolicy.CuratedAndModeSpecific",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDisplayResolutionPolicySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UWacomSettingsSubsystem> Subsystem =
		CreateResolutionSubsystem(GameInstance);

	FWacomSettingsSubsystemTestAccess::ConfigureScreenResolutionEnvironment(
		*Subsystem,
		FIntPoint(2560, 1080),
		FIntPoint(1920, 1040),
		{
			FIntPoint(1024, 768), FIntPoint(1280, 720), FIntPoint(1536, 864),
			FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1080),
			FIntPoint(1920, 1080)
		});

	const FWacomScreenResolutionOptions Fullscreen = Subsystem->GetScreenResolutionOptions(
		EWindowMode::Fullscreen,
		FIntPoint(1536, 864));
	const TArray<FIntPoint> ExpectedFullscreen = {
		FIntPoint(1280, 720), FIntPoint(1536, 864), FIntPoint(1600, 900),
		FIntPoint(1920, 1080), FIntPoint(2560, 1080)
	};
	TestTrue(TEXT("Fullscreen keeps only driver-supported curated/native/current modes"),
		Fullscreen.SelectableResolutions == ExpectedFullscreen);
	TestTrue(TEXT("Fullscreen resolution selection remains enabled"),
		Fullscreen.bCanSelectResolution);
	TestFalse(TEXT("Unsupported curated fullscreen mode is omitted"),
		Fullscreen.SelectableResolutions.Contains(FIntPoint(1366, 768)));
	TestFalse(TEXT("Resolution below the 1280 x 720 project minimum is omitted"),
		Fullscreen.SelectableResolutions.Contains(FIntPoint(1024, 768)));

	const FWacomScreenResolutionOptions Windowed = Subsystem->GetScreenResolutionOptions(
		EWindowMode::Windowed,
		FIntPoint(1536, 864));
	const TArray<FIntPoint> ExpectedWindowed = {
		FIntPoint(1280, 720), FIntPoint(1366, 768), FIntPoint(1536, 864),
		FIntPoint(1600, 900)
	};
	TestTrue(TEXT("Windowed modes are curated against the current work area"),
		Windowed.SelectableResolutions == ExpectedWindowed);
	TestFalse(TEXT("Windowed mode omits a resolution taller than the work area"),
		Windowed.SelectableResolutions.Contains(FIntPoint(1920, 1080)));

	const FWacomScreenResolutionOptions Borderless = Subsystem->GetScreenResolutionOptions(
		EWindowMode::WindowedFullscreen,
		FIntPoint(1280, 720));
	TestEqual(TEXT("Borderless reports the current desktop resolution"),
		Borderless.DesktopResolution, FIntPoint(2560, 1080));
	TestTrue(TEXT("Borderless exposes no selectable resolution list"),
		Borderless.SelectableResolutions.IsEmpty());
	TestFalse(TEXT("Borderless disables resolution selection"),
		Borderless.bCanSelectResolution);

	FWacomSettingsSubsystemTestAccess::ConfigureScreenResolutionEnvironment(
		*Subsystem,
		FIntPoint(1024, 768),
		FIntPoint(1024, 700),
		{});
	const FWacomScreenResolutionOptions UnsupportedDesktop =
		Subsystem->GetScreenResolutionOptions(EWindowMode::Windowed, FIntPoint(800, 600));
	TestTrue(TEXT("A display below the project minimum safely exposes no choices"),
		UnsupportedDesktop.SelectableResolutions.IsEmpty());
	TestFalse(TEXT("An empty catalog disables only the resolution selector"),
		UnsupportedDesktop.bCanSelectResolution);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsResolutionModeRefreshSpec,
	"Wacom.UI.Settings.DisplayResolutionModeRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsResolutionModeRefreshSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomGameUserSettings> Settings(NewObject<UWacomGameUserSettings>());
	Settings->SetToDefaults();
	FWacomLocalSettingsSnapshot Snapshot = Settings->MakeSnapshot();
	Snapshot.ScreenResolution = FIntPoint(1536, 864);
	Snapshot.WindowMode = EWindowMode::Windowed;
	Settings->SetFromSnapshot(Snapshot);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomSettingsSubsystem> Subsystem(
		NewObject<UWacomSettingsSubsystem>(GameInstance.Get()));
	FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(*Subsystem, *Settings);
	FWacomSettingsSubsystemTestAccess::ConfigureScreenResolutionEnvironment(
		*Subsystem,
		FIntPoint(2560, 1440),
		FIntPoint(3840, 2160),
		{ FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080) });

	TStrongObjectPtr<UWacomSettingsScreen> Screen(NewObject<UWacomSettingsScreen>());
	FWacomSettingsScreenTestAccess::BuildAndConstruct(*Screen);
	if (!TestTrue(TEXT("Settings edit starts"),
		FWacomSettingsScreenTestAccess::BeginWithSubsystem(*Screen, *Subsystem)))
	{
		FWacomSettingsScreenTestAccess::Destruct(*Screen);
		return false;
	}

	TestEqual(TEXT("A valid current custom windowed resolution is retained"),
		FWacomSettingsScreenTestAccess::View(*Screen).Draft.ScreenResolution,
		FIntPoint(1536, 864));
	FWacomSettingsScreenTestAccess::Step(
		*Screen, EWacomSettingsField::WindowMode, -1);
	const FWacomSettingsScreenAutomationTestView Fullscreen =
		FWacomSettingsScreenTestAccess::View(*Screen);
	TestEqual(TEXT("Switching mode normalizes an unavailable custom resolution to the nearest mode"),
		Fullscreen.Draft.ScreenResolution, FIntPoint(1600, 900));
	TestTrue(TEXT("Screen refreshes the fullscreen catalog"),
		Fullscreen.ScreenResolutionOptions.SelectableResolutions
			== TArray<FIntPoint>({ FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080) }));

	FWacomSettingsScreenTestAccess::Step(
		*Screen, EWacomSettingsField::WindowMode, -1);
	const FWacomSettingsScreenAutomationTestView Borderless =
		FWacomSettingsScreenTestAccess::View(*Screen);
	TestEqual(TEXT("Switching to borderless stores the current desktop resolution in the draft"),
		Borderless.Draft.ScreenResolution, FIntPoint(2560, 1440));
	const FWacomSettingsOptionRowViewData BorderlessRow = FWacomSettingsScreenTestAccess::Row(
		*Screen, EWacomSettingsField::ScreenResolution);
	TestFalse(TEXT("Borderless keeps the resolution row disabled"), BorderlessRow.bEnabled);
	TestTrue(TEXT("Borderless displays desktop ownership"),
		BorderlessRow.Value.ToString().Contains(TEXT("2560 × 1440")));
	FWacomSettingsScreenTestAccess::Step(
		*Screen, EWacomSettingsField::ScreenResolution, 1);
	TestEqual(TEXT("A disabled borderless resolution request has no side effect"),
		FWacomSettingsScreenTestAccess::View(*Screen).Draft.ScreenResolution,
		FIntPoint(2560, 1440));
	FWacomSettingsScreenTestAccess::Destruct(*Screen);

	TStrongObjectPtr<UWacomGameUserSettings> LegacySettings(NewObject<UWacomGameUserSettings>());
	LegacySettings->SetToDefaults();
	FWacomLocalSettingsSnapshot LegacySnapshot = LegacySettings->MakeSnapshot();
	LegacySnapshot.ScreenResolution = FIntPoint(1024, 600);
	LegacySnapshot.WindowMode = EWindowMode::Windowed;
	LegacySettings->SetFromSnapshot(LegacySnapshot);
	TStrongObjectPtr<UGameInstance> LegacyGameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomSettingsSubsystem> LegacySubsystem(
		NewObject<UWacomSettingsSubsystem>(LegacyGameInstance.Get()));
	FWacomSettingsSubsystemTestAccess::ConfigureIsolatedSettings(
		*LegacySubsystem, *LegacySettings);
	FWacomSettingsSubsystemTestAccess::ConfigureScreenResolutionEnvironment(
		*LegacySubsystem,
		FIntPoint(1920, 1080),
		FIntPoint(1920, 1080),
		{ FIntPoint(1280, 720), FIntPoint(1920, 1080) });
	TStrongObjectPtr<UWacomSettingsScreen> LegacyScreen(NewObject<UWacomSettingsScreen>());
	FWacomSettingsScreenTestAccess::BuildAndConstruct(*LegacyScreen);
	TestTrue(TEXT("Legacy settings edit starts"),
		FWacomSettingsScreenTestAccess::BeginWithSubsystem(*LegacyScreen, *LegacySubsystem));
	TestEqual(TEXT("Opening the screen does not silently rewrite a legacy resolution"),
		FWacomSettingsScreenTestAccess::View(*LegacyScreen).Draft.ScreenResolution,
		FIntPoint(1024, 600));
	FWacomSettingsScreenTestAccess::Step(
		*LegacyScreen, EWacomSettingsField::ScreenResolution, 1);
	TestEqual(TEXT("The first explicit step exits the legacy resolution through the minimum preset"),
		FWacomSettingsScreenTestAccess::View(*LegacyScreen).Draft.ScreenResolution,
		FIntPoint(1280, 720));
	FWacomSettingsScreenTestAccess::Destruct(*LegacyScreen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICappedDesignScaleContractSpec,
	"Wacom.UI.Settings.CappedDesignScaleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICappedDesignScaleContractSpec::RunTest(const FString& /*Parameters*/)
{
	const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>();
	if (!TestNotNull(TEXT("Project UI settings"), UISettings))
	{
		return false;
	}
	TestEqual(TEXT("Project UI uses the custom capped design rule"),
		UISettings->UIScaleRule, EUIScalingRule::Custom);
	TestEqual(TEXT("Project UI resolves the Wacom capped design rule"),
		UISettings->CustomScalingRuleClass.TryLoadClass<UDPICustomScalingRule>(),
		UWacomCappedDesignDPIScalingRule::StaticClass());
	TestTrue(TEXT("Project application scale remains neutral"),
		FMath::IsNearlyEqual(UISettings->ApplicationScale, 1.0f));

	struct FScaleExpectation
	{
		FIntPoint Size;
		float Scale;
	};
	const FScaleExpectation Expectations[] = {
		{ FIntPoint(1280, 720), 2.0f / 3.0f },
		{ FIntPoint(1366, 768), 768.0f / 1080.0f },
		{ FIntPoint(1600, 900), 5.0f / 6.0f },
		{ FIntPoint(1920, 1080), 1.0f },
		{ FIntPoint(2560, 1440), 1.0f },
		{ FIntPoint(3840, 2160), 1.0f },
		{ FIntPoint(1280, 800), 2.0f / 3.0f },
		{ FIntPoint(2560, 1080), 1.0f },
		{ FIntPoint(1600, 720), 2.0f / 3.0f }
	};
	for (const FScaleExpectation& Expectation : Expectations)
	{
		TestTrue(
			*FString::Printf(TEXT("Capped design scale resolves %d x %d"), Expectation.Size.X, Expectation.Size.Y),
			FMath::IsNearlyEqual(
				UISettings->GetDPIScaleBasedOnSize(Expectation.Size),
				Expectation.Scale,
				0.001f));
	}
	return true;
}

#endif
